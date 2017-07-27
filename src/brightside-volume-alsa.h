/* brightside-volume-alsa.h

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

#include <glib.h>
#include <glib-object.h>
#include "brightside-volume.h"

#define BRIGHTSIDE_TYPE_VOLUME_ALSA		(brightside_volume_get_type ())
#define BRIGHTSIDE_VOLUME_ALSA(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BRIGHTSIDE_TYPE_VOLUME_ALSA, BrightsideVolumeAlsa))
#define BRIGHTSIDE_VOLUME_ALSA_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BRIGHTSIDE_TYPE_VOLUME_ALSA, BrightsideVolumeAlsaClass))
#define BRIGHTSIDE_IS_VOLUME_ALSA(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BRIGHTSIDE_TYPE_VOLUME_ALSA))
#define BRIGHTSIDE_VOLUME_ALSA_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BRIGHTSIDE_TYPE_VOLUME_ALSA, BrightsideVolumeAlsaClass))

typedef struct BrightsideVolumeAlsa BrightsideVolumeAlsa;
typedef struct BrightsideVolumeAlsaClass BrightsideVolumeAlsaClass;
typedef struct BrightsideVolumeAlsaPrivate BrightsideVolumeAlsaPrivate;

struct BrightsideVolumeAlsa {
	BrightsideVolume parent;
	BrightsideVolumeAlsaPrivate *_priv;
};

struct BrightsideVolumeAlsaClass {
	BrightsideVolumeClass parent;
};

GType brightside_volume_alsa_get_type		(void);

