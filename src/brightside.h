/* BRIGHTSIDE
 * Copyright (C) 2004 Ed Catmur <ed@catmur.co.uk>
 *
 * brightside.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 * USA.
 */

#ifndef __BRIGHTSIDE_H__
#define __BRIGHTSIDE_H__

#include <gnome.h>
#include <gconf/gconf-client.h>

#define DIALOG_TIMEOUT 1000	/* timeout in ms for action icon dialog */

#define SELECTION_NAME "_BRIGHTSIDE_SELECTION" /* X selection to communicate
					       between daemon and capplet */

#define BRIGHTSIDE_KEY_ROOT "/apps/brightside"

#define TIMEVAL_ELAPSED_MSEC(a, b) ((((a).tv_sec - (b).tv_sec) * 1000) \
			+ (((a).tv_usec - (b).tv_usec) / 1000))

#if 0

#if defined(__powerpc__) && defined (__linux__)
#define USE_FBLEVEL
#else
#undef USE_FBLEVEL
#endif

#endif

enum {
	MUTE_VOLUME_ACTION,
	NO_SCREENSAVER_ACTION,
	SCREENSAVER_ACTION,
	DPMS_STANDBY_ACTION,
	DPMS_SUSPEND_ACTION,
	DPMS_OFF_ACTION,
	SHOWDESKTOP_ACTION,
#ifdef USE_FBLEVEL
	DIM_BACKLIGHT_ACTION,
#endif
	CUSTOM_ACTION,
	HANDLED_ACTIONS,
};

static const gchar *action_descriptions[HANDLED_ACTIONS] = {
	N_("Mute volume"),
	N_("Prevent screensaver starting"),
	N_("Start screensaver"),
	N_("Enter DPMS standby mode"),
	N_("Enter DPMS suspend mode"),
	N_("Enter DPMS off mode"),
	N_("Toggle showing desktop"),
#ifdef USE_FBLEVEL
	N_("Dim laptop backlight"),
#endif
	N_("Custom action..."),
};

static GConfEnumStringPair actions_lookup_table[HANDLED_ACTIONS + 1] = {
	{ MUTE_VOLUME_ACTION, "mute", },
	{ NO_SCREENSAVER_ACTION, "noscreensaver", },
	{ SCREENSAVER_ACTION, "screensaver", },
	{ DPMS_STANDBY_ACTION, "standby", },
	{ DPMS_SUSPEND_ACTION, "suspend", },
	{ DPMS_OFF_ACTION, "off", },
	{ SHOWDESKTOP_ACTION, "showdesktop", },
#ifdef USE_FBLEVEL
	{ DIM_BACKLIGHT_ACTION, "dimbacklight", },
#endif
	{ CUSTOM_ACTION, "custom", },
	{ -1, NULL },
};

/* Can change ordering as long as whole contents of this file are updated */
typedef enum { NW, SW, NE, SE, 
	TOP, BOTTOM, LEFT, RIGHT, NONE = -1 } BrightsideRegionType;
typedef BrightsideRegionType BrightsideCornerType;
#define REGION_FIRST_CORNER NW
#define REGION_IS_CORNER(r) ((r) >= NW && (r) <= SE)
#define REGION_IS_EDGE(r) ((r) >= TOP && (r) <= RIGHT)
#define REGION_ADJACENT_REGIONS(r, s) ( \
 ((r) == NW && (s) == TOP) 	|| ((r) == TOP && 	(s) == NE) || \
 ((r) == NE && (s) == RIGHT) 	|| ((r) == RIGHT && 	(s) == SE) || \
 ((r) == SE && (s) == BOTTOM) 	|| ((r) == BOTTOM && 	(s) == SW) || \
 ((r) == SW && (s) == LEFT) 	|| ((r) == LEFT && 	(s) == NW) || \
 ((s) == NW && (r) == TOP) 	|| ((s) == TOP && 	(r) == NE) || \
 ((s) == NE && (r) == RIGHT) 	|| ((s) == RIGHT && 	(r) == SE) || \
 ((s) == SE && (r) == BOTTOM) 	|| ((s) == BOTTOM && 	(r) == SW) || \
 ((s) == SW && (r) == LEFT) 	|| ((s) == LEFT && 	(r) == NW) || \
 0 )
#define REGION_CORNER_TO_ADJACENT_EDGE(r, s) ( \
 ((r) == NW && (s) == TOP)	|| ((r) == NE && (s) == RIGHT) || \
 ((r) == SE && (s) == BOTTOM)	|| ((r) == SW && (s) == LEFT) || \
 ((r) == NW && (s) == LEFT)	|| ((r) == NE && (s) == TOP) || \
 ((r) == SE && (s) == RIGHT)	|| ((r) == SW && (s) == BOTTOM) || \
 0 )
#define REGION_GESTURE_CODES "FLTJNSWE" /* corners are Graffiti strokes */
#define REGION_GESTURE_HISTORY 8

static struct {
	BrightsideCornerType id;
	const gchar *enabled_key;
	const char *enabled_toggle_id;
	const gchar *action_key;
	const char *action_menu_id;
	const gchar *custom_in_key;
	const gchar *custom_out_key;
	const gchar *custom_kill_key;
	gint default_action; /* see also brightside.schemas.in */
} corners[4] = {
	{ NW, "/apps/brightside/nw_enabled", "nw_enabled",
		"/apps/brightside/nw_action", "nw_action",
		"/apps/brightside/nw_custom_in", 
		"/apps/brightside/nw_custom_out",
		"/apps/brightside/nw_custom_kill", MUTE_VOLUME_ACTION, },
	{ SW, "/apps/brightside/sw_enabled", "sw_enabled",
		"/apps/brightside/sw_action", "sw_action",
		"/apps/brightside/sw_custom_in", 
		"/apps/brightside/sw_custom_out",
		"/apps/brightside/sw_custom_kill", NO_SCREENSAVER_ACTION, },
	{ NE, "/apps/brightside/ne_enabled", "ne_enabled",
		"/apps/brightside/ne_action", "ne_action",
		"/apps/brightside/ne_custom_in", 
		"/apps/brightside/ne_custom_out",
		"/apps/brightside/ne_custom_kill", SCREENSAVER_ACTION, },
	{ SE, "/apps/brightside/se_enabled", "se_enabled",
		"/apps/brightside/se_action", "se_action",
		"/apps/brightside/se_custom_in", 
		"/apps/brightside/se_custom_out",
		"/apps/brightside/se_custom_kill", DPMS_STANDBY_ACTION },
};

#define CORNER_DELAY_KEY "/apps/brightside/corner_delay"
#define CORNER_FLIP_KEY "/apps/brightside/corner_flip"
#define ENABLE_EDGE_FLIP_KEY "/apps/brightside/enable_edge_flip"
#define AFTER_FLIP_COMMAND_KEY "/apps/brightside/after_flip_command"
#define EDGE_DELAY_KEY "/apps/brightside/edge_delay"
#define EDGE_WRAP_KEY "/apps/brightside/edge_wrap"
#define ORIENTABLE_WORKSPACES_KEY "/apps/brightside/orientable_workspaces"
#define USE_PCM_KEY "/apps/brightside/use_pcm"

#define CUSTOM_SHOW_LIST_KEY \
	"/apps/panel/profiles/default/general/show_program_list"

#define MOVING_WINDOW_ATOM "_NET_MOVING_WINDOW"
#define RESIZING_WINDOW_ATOM "_NET_RESIZING_WINDOW"
#define DESKTOP_LAYOUT_ATOM "_NET_DESKTOP_LAYOUT"
#define WM_MOVERESIZE_ATOM "_NET_WM_MOVERESIZE"
#define ACTIVE_WINDOW_ATOM "_NET_ACTIVE_WINDOW"
/* fragment of EWMH spec */
#define _NET_WM_MOVERESIZE_MOVE              8   /* movement only */

#define BRIGHTSIDE_URL PACKAGE_BUGREPORT

#define EASTER_EGG_GESTURE "TFLJWEWE"	/* Euro symbol */

#endif /* __BRIGHTSIDE_H__ */

