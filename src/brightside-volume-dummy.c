/* brightside-volume-dummy.c

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
#include "brightside-volume-dummy.h"

static GObjectClass *parent_class = NULL;

static int brightside_volume_dummy_get_volume (BrightsideVolume *self);
static void brightside_volume_dummy_set_volume (BrightsideVolume *self, int val);
//static gboolean brightside_volume_dummy_mixer_check (BrightsideVolumeDummy *self, int fd);

static void
brightside_volume_dummy_finalize (GObject *object)
{
	BrightsideVolumeDummy *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (BRIGHTSIDE_IS_VOLUME_DUMMY (object));

	self = BRIGHTSIDE_VOLUME_DUMMY (object);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
brightside_volume_dummy_set_use_pcm (BrightsideVolume *vol, gboolean val)
{
}

static gboolean
brightside_volume_dummy_get_use_pcm (BrightsideVolume *vol)
{
	return FALSE;
}

static void
brightside_volume_dummy_set_mute (BrightsideVolume *vol, gboolean val)
{
}

static void
brightside_volume_dummy_set_mute_fade (BrightsideVolume *vol, gboolean val,
		guint duration)
{
}

static gboolean
brightside_volume_dummy_get_mute (BrightsideVolume *vol)
{
	return FALSE;
}

static int
brightside_volume_dummy_get_volume (BrightsideVolume *vol)
{
	return 0;
}

static void
brightside_volume_dummy_set_volume (BrightsideVolume *vol, int val)
{
}

static void
brightside_volume_dummy_init (BrightsideVolume *vol)
{
}

static void
brightside_volume_dummy_class_init (BrightsideVolumeDummyClass *klass)
{
	BrightsideVolumeClass *volume_class = BRIGHTSIDE_VOLUME_CLASS (klass);
	G_OBJECT_CLASS (klass)->finalize = brightside_volume_dummy_finalize;

	parent_class = g_type_class_peek_parent (klass);

	volume_class->set_volume = brightside_volume_dummy_set_volume;
	volume_class->get_volume = brightside_volume_dummy_get_volume;
	volume_class->set_mute = brightside_volume_dummy_set_mute;
	volume_class->set_mute_fade = brightside_volume_dummy_set_mute_fade;
	volume_class->get_mute = brightside_volume_dummy_get_mute;
	volume_class->set_use_pcm = brightside_volume_dummy_set_use_pcm;
	volume_class->get_use_pcm = brightside_volume_dummy_get_use_pcm;
}

GType brightside_volume_dummy_get_type (void)
{
	static GType object_type = 0;

	if (!object_type)
	{
		static const GTypeInfo object_info =
		{
			sizeof (BrightsideVolumeDummyClass),
			NULL,         /* base_init */
			NULL,         /* base_finalize */
			(GClassInitFunc) brightside_volume_dummy_class_init,
			NULL,         /* class_finalize */
			NULL,         /* class_data */
			sizeof (BrightsideVolumeDummy),
			0,            /* n_preallocs */
			(GInstanceInitFunc) brightside_volume_dummy_init
		};

		object_type = g_type_register_static (BRIGHTSIDE_TYPE_VOLUME,
				"BrightsideVolumeDummy", &object_info, 0);
	}

	return object_type;
}

