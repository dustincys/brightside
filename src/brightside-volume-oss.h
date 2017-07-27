/* brightside-volume-oss.h

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

#define BRIGHTSIDE_TYPE_VOLUME_OSS		(brightside_volume_get_type ())
#define BRIGHTSIDE_VOLUME_OSS(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BRIGHTSIDE_TYPE_VOLUME_OSS, BrightsideVolumeOss))
#define BRIGHTSIDE_VOLUME_OSS_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BRIGHTSIDE_TYPE_VOLUME_OSS, BrightsideVolumeOssClass))
#define BRIGHTSIDE_IS_VOLUME_OSS(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BRIGHTSIDE_TYPE_VOLUME_OSS))
#define BRIGHTSIDE_VOLUME_OSS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BRIGHTSIDE_TYPE_VOLUME_OSS, BrightsideVolumeOssClass))

typedef struct BrightsideVolumeOss BrightsideVolumeOss;
typedef struct BrightsideVolumeOssClass BrightsideVolumeOssClass;
typedef struct BrightsideVolumeOssPrivate BrightsideVolumeOssPrivate;

struct BrightsideVolumeOss {
	BrightsideVolume parent;
	BrightsideVolumeOssPrivate *_priv;
};

struct BrightsideVolumeOssClass {
	BrightsideVolumeClass parent;
};

GType brightside_volume_oss_get_type		(void);

