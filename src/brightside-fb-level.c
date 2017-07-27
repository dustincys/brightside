/* brightside-fb-level.c

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
#include "brightside-fb-level.h"

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <linux/pmu.h>
#include <errno.h>

#ifndef FBIOBLANK
#define FBIOBLANK      0x4611          /* 0 or vesa-level+1 */
#endif
                                                                                
#ifndef PMU_IOC_GRAB_BACKLIGHT
#define PMU_IOC_GRAB_BACKLIGHT  _IOR('B', 6, 0)
#endif

struct BrightsideFblevelPrivate {
	int pmu_fd;
	int saved_level;
};

static GObjectClass *parent_class = NULL;

static void
brightside_fblevel_finalize (GObject *obj_self)
{
	BrightsideFblevel *self;// = BRIGHTSIDE_FBLEVEL (obj_self);
	gpointer priv = self->_priv;

	if (G_OBJECT_CLASS(parent_class)->finalize)
		(* G_OBJECT_CLASS(parent_class)->finalize)(obj_self);
	g_free (priv);

	return;
}

static void
brightside_fblevel_class_init (BrightsideFblevelClass *klass)
{
	GObjectClass *g_object_class = (GObjectClass*) klass;
	parent_class = g_type_class_ref (G_TYPE_OBJECT);
	g_object_class->finalize = brightside_fblevel_finalize;

	return;
}

static void
brightside_fblevel_init (BrightsideFblevel *fblevel)
{
	fblevel->_priv = g_new0 (BrightsideFblevelPrivate, 1);
	fblevel->level = 0;
	fblevel->dim = FALSE;
	fblevel->_priv->pmu_fd = -1;
	fblevel->_priv->saved_level = 0;

	return;
}

GType brightside_fblevel_get_type (void)
{
	static GType type = 0;
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (BrightsideFblevelClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) brightside_fblevel_class_init,
			(GClassFinalizeFunc) NULL,
			NULL /* class_data */,
			sizeof (BrightsideFblevel),
			0 /* n_preallocs */,
			(GInstanceInitFunc) brightside_fblevel_init,
		};

		type = g_type_register_static (G_TYPE_OBJECT, "BrightsideFblevel",
				&info, (GTypeFlags)0);
	}
	return type;
}

int
brightside_fblevel_get_level (BrightsideFblevel *self)
{
	int level;
	ioctl (self->_priv->pmu_fd,
			PMU_IOC_GET_BACKLIGHT, &level);
	return level;
}

void
brightside_fblevel_set_level (BrightsideFblevel *self, int val)
{
	int level;

	level = CLAMP (val, 0, 15);

	ioctl (self->_priv->pmu_fd,
			PMU_IOC_SET_BACKLIGHT, &level);
	self->level = level;
}

gboolean
brightside_fblevel_get_dim (BrightsideFblevel *self)
{
	return self->dim;
}

void
brightside_fblevel_set_dim (BrightsideFblevel *self, gboolean val)
{
	if (self->dim == FALSE && val == TRUE)
	{
		self->_priv->saved_level = brightside_fblevel_get_level(self);
		brightside_fblevel_set_level (self, 1);
		self->dim = TRUE;
	} else if (self->dim == TRUE && val == FALSE) {
		brightside_fblevel_set_level (self, self->_priv->saved_level);
		self->dim = FALSE;
	}
}

BrightsideFblevel *
brightside_fblevel_new (void)
{
	BrightsideFblevel *self;
	int fd, foo;

	if (g_file_test ("/dev/pmu", G_FILE_TEST_EXISTS) == FALSE)
		return NULL;

	if (brightside_fblevel_is_powerbook () == FALSE)
		return NULL;

	self = BRIGHTSIDE_FBLEVEL (g_object_new (BRIGHTSIDE_TYPE_FBLEVEL, NULL));
	/* This function switches the kernel backlight control off.
	 * This is part of the PPC kernel branch since version
	 * 2.4.18-rc2-benh. It does nothing with older kernels.
	 * For those kernels a separate kernel patch is nessecary to
	 * get backlight control in user space.
	 *
	 * Notice nicked from pbbuttons*/
	fd  = open ("/dev/pmu", O_RDWR);
	/* We can't emit the signal yet, the signal isn't connected! */
	if (fd < 0)
		return NULL;

	foo = ioctl(fd, PMU_IOC_GRAB_BACKLIGHT, 0);
	self->_priv->pmu_fd = fd;
	return self;
}

gboolean
brightside_fblevel_is_powerbook (void)
{
	FILE *fd;
	char str[2048];
	gboolean found = FALSE;

	fd = fopen ("/proc/cpuinfo", "r");
	while (!feof (fd) && found == FALSE)
	{
		fread (str, 1, 2048, fd);
		if (strstr (str, "PowerBook") != NULL)
			found = TRUE;
	}

	fclose (fd);

	return found;
}

