/* brightside-volume-alsa.c

   Copyright (C) 2002, 2003 Bastien Nocera

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Bastien Nocera <hadess@hadess.net>
 */

#include "config.h"
#include "brightside-volume-alsa.h"

#include <alsa/asoundlib.h>

#ifndef DEFAULT_CARD
#define DEFAULT_CARD "default"
#endif

#undef LOG
#ifdef LOG
#define D(x...) g_message (x)
#else
#define D(x...)
#endif

struct BrightsideVolumeAlsaPrivate
{
	gboolean use_pcm;
	long pmin, pmax;
	snd_mixer_t *handle;
	snd_mixer_elem_t *elem;
	gboolean mixerpb;
	int saved_volume;
	gboolean fading;
};

static GObjectClass *parent_class = NULL;

static int brightside_volume_alsa_get_volume (BrightsideVolume *self);
static void brightside_volume_alsa_set_volume (BrightsideVolume *self, int val);
static gboolean brightside_volume_alsa_mixer_check (BrightsideVolumeAlsa *self, int fd);
static gboolean brightside_volume_alsa_get_mute (BrightsideVolume *vol);

static void
brightside_volume_alsa_finalize (GObject *object)
{
	BrightsideVolumeAlsa *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (BRIGHTSIDE_IS_VOLUME_ALSA (object));

	self = BRIGHTSIDE_VOLUME_ALSA (object);

	if (self->_priv)
		g_free (self->_priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static int
brightside_volume_alsa_vol_check (BrightsideVolumeAlsa *self, int volume)
{
	return CLAMP (volume, self->_priv->pmin, self->_priv->pmax);
}

static void
brightside_volume_alsa_set_use_pcm (BrightsideVolume *vol, gboolean val)
{
	BrightsideVolumeAlsa *self = (BrightsideVolumeAlsa *) vol;
	self->_priv->use_pcm = val;
}

static gboolean
brightside_volume_alsa_get_use_pcm (BrightsideVolume *vol)
{
	BrightsideVolumeAlsa *self = (BrightsideVolumeAlsa *) vol;
	return self->_priv->use_pcm;
}

static void
brightside_volume_alsa_set_mute (BrightsideVolume *vol, gboolean val)
{
	BrightsideVolumeAlsa *self = (BrightsideVolumeAlsa *) vol;

	snd_mixer_selem_set_playback_switch(self->_priv->elem,
			SND_MIXER_SCHN_FRONT_LEFT, !val);
	if (val == TRUE) {
		self->_priv->saved_volume = brightside_volume_alsa_get_volume (vol);
		brightside_volume_alsa_set_volume (vol, 0);
	} else if (self->_priv->saved_volume != -1) {
		brightside_volume_alsa_set_volume (vol,
				self->_priv->saved_volume);
	}
}

static gboolean
brightside_volume_alsa_mute_fade_cb (gpointer data)
{
	BrightsideVolume *vol = (BrightsideVolume *) data;
	BrightsideVolumeAlsa *self = (BrightsideVolumeAlsa *) vol;
	int volume = brightside_volume_alsa_get_volume (vol);
	if (volume <= 0 && brightside_volume_alsa_get_mute (vol)) {
		self->_priv->fading = FALSE;
		return FALSE;
	}
	if (volume >= self->_priv->saved_volume
			&& !brightside_volume_alsa_get_mute (vol)) {
		self->_priv->fading = FALSE;
		return FALSE;
	}
	if (brightside_volume_alsa_get_mute (vol)) --volume;
	else ++volume;
	brightside_volume_alsa_set_volume (vol, volume);
	if (volume <= 0 && brightside_volume_alsa_get_mute (vol)) {
		self->_priv->fading = FALSE;
		return FALSE;
	}
	if (volume >= self->_priv->saved_volume
			&& !brightside_volume_alsa_get_mute (vol)) {
		self->_priv->fading = FALSE;
		return FALSE;
	}
	return TRUE;
}

static void
brightside_volume_alsa_set_mute_fade (BrightsideVolume *vol, gboolean val, 
		guint duration)
{
	BrightsideVolumeAlsa *self = (BrightsideVolumeAlsa *) vol;

	snd_mixer_selem_set_playback_switch(self->_priv->elem,
			SND_MIXER_SCHN_FRONT_LEFT, !val);
	if (!self->_priv->fading) {
		if (val == TRUE)
			self->_priv->saved_volume = 
				brightside_volume_alsa_get_volume (vol);
		if (self->_priv->saved_volume > 0) {
			g_timeout_add (duration / self->_priv->saved_volume,
				brightside_volume_alsa_mute_fade_cb, self);
			self->_priv->fading = TRUE;
		}
	}
}

static gboolean
brightside_volume_alsa_get_mute (BrightsideVolume *vol)
{
	BrightsideVolumeAlsa *self = (BrightsideVolumeAlsa *) vol;
	int ival;

	snd_mixer_selem_get_playback_switch(self->_priv->elem,
			SND_MIXER_SCHN_FRONT_LEFT, &ival);

	return !ival;
}

static int
brightside_volume_alsa_get_volume (BrightsideVolume *vol)
{
	BrightsideVolumeAlsa *self = (BrightsideVolumeAlsa *) vol;
	long lval, rval;
	int tmp;
	
	snd_mixer_selem_get_playback_volume(self->_priv->elem,
			SND_MIXER_SCHN_FRONT_LEFT, &lval);
	snd_mixer_selem_get_playback_volume(self->_priv->elem,
			SND_MIXER_SCHN_FRONT_RIGHT, &rval);

	tmp = brightside_volume_alsa_vol_check (self, (int) (lval + rval) / 2);
	return (tmp * 100 / self->_priv->pmax);
}

static void
brightside_volume_alsa_set_volume (BrightsideVolume *vol, int val)
{
	BrightsideVolumeAlsa *self = (BrightsideVolumeAlsa *) vol;

	val = (long) brightside_volume_alsa_vol_check (self,
			val * self->_priv->pmax / 100);
	snd_mixer_selem_set_playback_volume(self->_priv->elem,
			SND_MIXER_SCHN_FRONT_LEFT, val);
	snd_mixer_selem_set_playback_volume(self->_priv->elem,
			SND_MIXER_SCHN_FRONT_RIGHT, val);
}

static void
brightside_volume_alsa_init (BrightsideVolume *vol)
{
	BrightsideVolumeAlsa *self = (BrightsideVolumeAlsa *) vol;
	snd_mixer_selem_id_t *sid;
	snd_mixer_t *handle;
	snd_mixer_elem_t *elem;

	self->_priv = g_new0 (BrightsideVolumeAlsaPrivate, 1);

	 /* open the mixer */
	if (snd_mixer_open (&handle, 0) < 0)
	{
		D("snd_mixer_open");
		return;
	}
	/* attach the handle to the default card */
	if (snd_mixer_attach(handle, DEFAULT_CARD) <0)
	{
		D("snd_mixer_attach");
		goto bail;
	}
	/* ? */
	if (snd_mixer_selem_register(handle, NULL, NULL) < 0)
	{
		D("snd_mixer_selem_register");
		goto bail;
	}
	if (snd_mixer_load(handle) < 0)
	{
		D("snd_mixer_load");
		goto bail;
	}

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_name (sid, "Master");
	elem = snd_mixer_find_selem(handle, sid);
	if (!elem)
	{
		D("snd_mixer_find_selem");
		goto bail;
	}

	if (!snd_mixer_selem_has_playback_volume(elem))
	{
		D("snd_mixer_selem_has_capture_volume");
		goto bail;
	}

	snd_mixer_selem_get_playback_volume_range (elem,
			&(self->_priv->pmin),
			&(self->_priv->pmax));

	self->_priv->handle = handle;
	self->_priv->elem = elem;

	return;
bail:
	g_free (self->_priv);
	self->_priv = NULL;
}

static void
brightside_volume_alsa_class_init (BrightsideVolumeAlsaClass *klass)
{
	BrightsideVolumeClass *volume_class = BRIGHTSIDE_VOLUME_CLASS (klass);
	G_OBJECT_CLASS (klass)->finalize = brightside_volume_alsa_finalize;

	parent_class = g_type_class_peek_parent (klass);

	volume_class->set_volume = brightside_volume_alsa_set_volume;
	volume_class->get_volume = brightside_volume_alsa_get_volume;
	volume_class->set_mute = brightside_volume_alsa_set_mute;
	volume_class->set_mute_fade = brightside_volume_alsa_set_mute_fade;
	volume_class->get_mute = brightside_volume_alsa_get_mute;
	volume_class->set_use_pcm = brightside_volume_alsa_set_use_pcm;
	volume_class->get_use_pcm = brightside_volume_alsa_get_use_pcm;
}

GType brightside_volume_alsa_get_type (void)
{
	static GType object_type = 0;

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (BrightsideVolumeAlsaClass),
			NULL,         /* base_init */
			NULL,         /* base_finalize */
			(GClassInitFunc) brightside_volume_alsa_class_init,
			NULL,         /* class_finalize */
			NULL,         /* class_data */
			sizeof (BrightsideVolumeAlsa),
			0,            /* n_preallocs */
			(GInstanceInitFunc) brightside_volume_alsa_init
		};

		object_type = g_type_register_static (BRIGHTSIDE_TYPE_VOLUME,
				"BrightsideVolumeAlsa", &object_info, 0);
	}

	return object_type;
}

