/* brightside-volume.h

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

#ifndef _BRIGHTSIDE_VOLUME_H
#define _BRIGHTSIDE_VOLUME_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define BRIGHTSIDE_TYPE_VOLUME		(brightside_volume_get_type ())
#define BRIGHTSIDE_VOLUME(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BRIGHTSIDE_TYPE_VOLUME, BrightsideVolume))
#define BRIGHTSIDE_VOLUME_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BRIGHTSIDE_TYPE_VOLUME, BrightsideVolumeClass))
#define BRIGHTSIDE_IS_VOLUME(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BRIGHTSIDE_TYPE_VOLUME))
#define BRIGHTSIDE_VOLUME_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BRIGHTSIDE_TYPE_VOLUME, BrightsideVolumeClass))

typedef struct {
	GObject parent;
} BrightsideVolume;

typedef struct {
	GObjectClass parent;

	void (* set_volume) (BrightsideVolume *self, int val);
	int (* get_volume) (BrightsideVolume *self);
	void (* set_mute) (BrightsideVolume *self, gboolean val);
	void (* set_mute_fade) (BrightsideVolume *self, gboolean val, 
			guint duration);
	int (* get_mute) (BrightsideVolume *self);
	void (* set_use_pcm) (BrightsideVolume *self, gboolean val);
	gboolean (* get_use_pcm) (BrightsideVolume *self);
} BrightsideVolumeClass;

GType brightside_volume_get_type	(void);
int brightside_volume_get_volume	(BrightsideVolume *self);
void brightside_volume_set_volume	(BrightsideVolume *self, int val);
gboolean brightside_volume_get_mute	(BrightsideVolume *self);
void brightside_volume_set_mute		(BrightsideVolume *self, gboolean val);
void brightside_volume_set_mute_fade	(
		BrightsideVolume *self, gboolean val, guint duration);
gboolean brightside_volume_get_use_pcm	(BrightsideVolume *self);
void brightside_volume_set_use_pcm	(BrightsideVolume *self, gboolean val);
void brightside_volume_mute_toggle	(BrightsideVolume * self);
void brightside_volume_mute_toggle_fade	(
		BrightsideVolume * self, guint duration);
BrightsideVolume *brightside_volume_new	(void);

G_END_DECLS

#endif /* _BRIGHTSIDE_VOLUME_H */
