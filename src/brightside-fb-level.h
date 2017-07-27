/* brightside-fb-level.h

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

#define BRIGHTSIDE_TYPE_FBLEVEL		(brightside_fblevel_get_type ())
#define BRIGHTSIDE_FBLEVEL(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), BRIGHTSIDE_TYPE_FBLEVEL, BrightsideFblevel))
#define BRIGHTSIDE_FBLEVEL_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), BRIGHTSIDE_TYPE_FBLEVEL, BrightsideFblevelClass))
#define BRIGHTSIDE_IS_FBLEVEL(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BRIGHTSIDE_TYPE_FBLEVEL))
#define BRIGHTSIDE_FBLEVEL_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), BRIGHTSIDE_TYPE_FBLEVEL, BrightsideFblevelClass))

typedef struct BrightsideFblevelPrivate BrightsideFblevelPrivate;
typedef struct BrightsideFblevel BrightsideFblevel;
typedef struct BrightsideFblevelClass BrightsideFblevelClass;

struct BrightsideFblevel {
	GObject parent;
	int level;
	gboolean dim;
	BrightsideFblevelPrivate *_priv;
};

struct BrightsideFblevelClass {
	GObjectClass parent;
};

GType brightside_fblevel_get_type			(void);
int brightside_fblevel_get_level			(BrightsideFblevel *self);
void brightside_fblevel_set_level			(BrightsideFblevel *self, int val);
gboolean brightside_fblevel_get_dim			(BrightsideFblevel *self);
void brightside_fblevel_set_dim			(BrightsideFblevel *self,
						 gboolean val);
BrightsideFblevel *brightside_fblevel_new			(void);
gboolean brightside_fblevel_is_powerbook		(void);

