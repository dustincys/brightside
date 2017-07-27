/* brightside-volume-dummy.h

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

#define BRIGHTSIDE_TYPE_VOLUME_DUMMY		(brightside_volume_get_type ())
#define BRIGHTSIDE_VOLUME_DUMMY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BRIGHTSIDE_TYPE_VOLUME_DUMMY, BrightsideVolumeDummy))
#define BRIGHTSIDE_VOLUME_DUMMY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BRIGHTSIDE_TYPE_VOLUME_DUMMY, BrightsideVolumeDummyClass))
#define BRIGHTSIDE_IS_VOLUME_DUMMY(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BRIGHTSIDE_TYPE_VOLUME_DUMMY))
#define BRIGHTSIDE_VOLUME_DUMMY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BRIGHTSIDE_TYPE_VOLUME_DUMMY, BrightsideVolumeDummyClass))

typedef struct BrightsideVolumeDummy BrightsideVolumeDummy;
typedef struct BrightsideVolumeDummyClass BrightsideVolumeDummyClass;

struct BrightsideVolumeDummy {
	BrightsideVolume parent;
};

struct BrightsideVolumeDummyClass {
	BrightsideVolumeClass parent;
};

GType brightside_volume_dummy_get_type		(void);
BrightsideVolume* brightside_volume_dummy_new		(void);

