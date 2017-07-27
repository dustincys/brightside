/* brightside-volume-oss.c

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
#include "brightside-volume-oss.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifdef __NetBSD__
#include <soundcard.h>
#else
#include <sys/soundcard.h>
#endif /* __NetBSD__ */

struct BrightsideVolumeOssPrivate
{
	gboolean use_pcm;
	gboolean mixerpb;
	int volume;
	int saved_volume;
	gboolean pcm_avail;
	gboolean mute;
	gboolean fading;
};

static GObjectClass *parent_class = NULL;

static int brightside_volume_oss_get_volume (BrightsideVolume *self);
static void brightside_volume_oss_set_volume (BrightsideVolume *self, int val);
static gboolean brightside_volume_oss_mixer_check (BrightsideVolumeOss *self, int fd);

static void
brightside_volume_oss_finalize (GObject *object)
{
	BrightsideVolumeOss *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (BRIGHTSIDE_IS_VOLUME_OSS (object));

	self = BRIGHTSIDE_VOLUME_OSS (object);

	g_return_if_fail (self->_priv != NULL);
	g_free (self->_priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static int
brightside_volume_oss_vol_check (int volume)
{
	return CLAMP (volume, 0, 100);
}

static void
brightside_volume_oss_set_use_pcm (BrightsideVolume *vol, gboolean val)
{
	BrightsideVolumeOss *self = (BrightsideVolumeOss *) vol;
	self->_priv->use_pcm = val;
}

static gboolean
brightside_volume_oss_get_use_pcm (BrightsideVolume *vol)
{
	BrightsideVolumeOss *self = (BrightsideVolumeOss *) vol;
	return self->_priv->use_pcm;
}

static void
brightside_volume_oss_set_mute (BrightsideVolume *vol, gboolean val)
{
	BrightsideVolumeOss *self = (BrightsideVolumeOss *) vol;

	if (self->_priv->mute == FALSE)
	{
		self->_priv->saved_volume =
			brightside_volume_oss_get_volume (vol);
		brightside_volume_oss_set_volume (vol, 0);
		self->_priv->mute = TRUE;
	} else {
		brightside_volume_oss_set_volume (vol, self->_priv->saved_volume);
		self->_priv->mute = FALSE;
	}
}

static gboolean
brightside_volume_oss_mute_fade_cb (gpointer data)
{
	BrightsideVolume *vol = (BrightsideVolume *) data;
	BrightsideVolumeOss *self = (BrightsideVolumeOss *) vol;
	int volume = brightside_volume_oss_get_volume (vol);
	if (volume <= 0 && self->_priv->mute) {
		self->_priv->fading = FALSE;
		return FALSE;
	}
	if (volume >= self->_priv->saved_volume && !self->_priv->mute) {
		self->_priv->fading = FALSE;
		return FALSE;
	}
	if (self->_priv->mute) --volume;
	else ++volume;
	brightside_volume_oss_set_volume (vol, volume);
	if (volume <= 0 && self->_priv->mute) {
		self->_priv->fading = FALSE;
		return FALSE;
	}
	if (volume >= self->_priv->saved_volume && !self->_priv->mute) {
		self->_priv->fading = FALSE;
		return FALSE;
	}
	return TRUE;
}

static void
brightside_volume_oss_set_mute_fade (BrightsideVolume *vol, gboolean val, 
		guint duration)
{
	BrightsideVolumeOss *self = (BrightsideVolumeOss *) vol;

	if (!self->_priv->fading) {
		if (!self->_priv->mute)
			self->_priv->saved_volume = 
				brightside_volume_oss_get_volume (vol);
		if (self->_priv->saved_volume > 0) {
			g_timeout_add (duration / self->_priv->saved_volume,
				brightside_volume_oss_mute_fade_cb, self);
			self->_priv->fading = TRUE;
		}
	}
	self->_priv->mute = !self->_priv->mute;
}

static gboolean
brightside_volume_oss_get_mute (BrightsideVolume *vol)
{
	BrightsideVolumeOss *self = (BrightsideVolumeOss *) vol;

	/* somebody else might have changed the volume */
	if ((self->_priv->mute == TRUE) && (self->_priv->volume != 0))
	{
		self->_priv->mute = FALSE;
	}

	return self->_priv->mute;
}

static int
brightside_volume_oss_get_volume (BrightsideVolume *vol)
{
	gint volume, r, l, fd;
	BrightsideVolumeOss *self = (BrightsideVolumeOss *) vol;

	fd  = open ("/dev/mixer", O_RDONLY);
	if (brightside_volume_oss_mixer_check(self, fd) == FALSE)
	{
		volume = 0;
	} else {
		if (self->_priv->use_pcm && self->_priv->pcm_avail)
			ioctl (fd, MIXER_READ (SOUND_MIXER_PCM), &volume);
		else
			ioctl (fd, MIXER_READ (SOUND_MIXER_VOLUME), &volume);
		close (fd);

		r = (volume & 0xff);
		l = (volume & 0xff00) >> 8;
		volume = (r + l) / 2;
		volume = brightside_volume_oss_vol_check (volume);
	}

	return volume;
}

static void
brightside_volume_oss_set_volume (BrightsideVolume *vol, int val)
{
	int fd, tvol, volume;
	BrightsideVolumeOss *self = (BrightsideVolumeOss *) vol;

	volume = brightside_volume_oss_vol_check (val);

	fd = open ("/dev/mixer", O_RDONLY);
	if (brightside_volume_oss_mixer_check (self, fd) == FALSE)
	{
		return;
	} else {
		tvol = (volume << 8) + volume;
		if (self->_priv->use_pcm && self->_priv->pcm_avail)
			ioctl (fd, MIXER_WRITE (SOUND_MIXER_PCM), &tvol);
		else
			ioctl (fd, MIXER_WRITE (SOUND_MIXER_VOLUME), &tvol);
		close (fd);
		self->_priv->volume = volume;
	}
}

static void
brightside_volume_oss_init (BrightsideVolume *vol)
{
	BrightsideVolumeOss *self = (BrightsideVolumeOss *) vol;
	int fd;

	self->_priv = g_new0 (BrightsideVolumeOssPrivate, 1);

	fd  = open ("/dev/mixer", O_RDONLY);
	if (brightside_volume_oss_mixer_check(self, fd) == FALSE)
	{
		self->_priv->pcm_avail = FALSE;
	} else {
		int mask = 0;

		ioctl (fd, SOUND_MIXER_READ_DEVMASK, &mask);
		if (mask & ( 1 << SOUND_MIXER_PCM))
			self->_priv->pcm_avail = TRUE;
		else
			self->_priv->pcm_avail = FALSE;
		if (!(mask & ( 1 << SOUND_MIXER_VOLUME)))
			self->_priv->use_pcm = TRUE;

		close (fd);
	}
}

static void
brightside_volume_oss_class_init (BrightsideVolumeOssClass *klass)
{
	BrightsideVolumeClass *volume_class = BRIGHTSIDE_VOLUME_CLASS (klass);
	G_OBJECT_CLASS (klass)->finalize = brightside_volume_oss_finalize;

	parent_class = g_type_class_peek_parent (klass);

	volume_class->set_volume = brightside_volume_oss_set_volume;
	volume_class->get_volume = brightside_volume_oss_get_volume;
	volume_class->set_mute = brightside_volume_oss_set_mute;
	volume_class->set_mute_fade = brightside_volume_oss_set_mute_fade;
	volume_class->get_mute = brightside_volume_oss_get_mute;
	volume_class->set_use_pcm = brightside_volume_oss_set_use_pcm;
	volume_class->get_use_pcm = brightside_volume_oss_get_use_pcm;
}

GType brightside_volume_oss_get_type (void)
{
	static GType object_type = 0;

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (BrightsideVolumeOssClass),
			NULL,         /* base_init */
			NULL,         /* base_finalize */
			(GClassInitFunc) brightside_volume_oss_class_init,
			NULL,         /* class_finalize */
			NULL,         /* class_data */
			sizeof (BrightsideVolumeOss),
			0,            /* n_preallocs */
			(GInstanceInitFunc) brightside_volume_oss_init
		};

		object_type = g_type_register_static (BRIGHTSIDE_TYPE_VOLUME,
				"BrightsideVolumeOss", &object_info, 0);
	}

	return object_type;
}

static gboolean
brightside_volume_oss_mixer_check (BrightsideVolumeOss *self, int fd)
{
	gboolean retval;

	if (fd <0) {
		if (self->_priv->mixerpb == FALSE) {
			self->_priv->mixerpb = TRUE;
			//FIXME
			//volume_oss_fd_problem(self);
		}
	}
	retval = (!self->_priv->mixerpb);
	return retval;
}

