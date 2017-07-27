/* BRIGHTSIDE
 * Copyright (C) 2004 Ed Catmur <ed@catmur.co.uk>
 * Portions copyright (C) 2001 Bastien Nocera <hadess@hadess.net>
 *
 * brightside.c
 *
 * This file is part of Brightside.
 *
 * Brightside is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Brightside is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Brightside; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 * USA.
 */

#include <config.h>

#include <sys/file.h>
#include <signal.h>
#include <X11/X.h>
#include <X11/Xatom.h>

/* Gnome headers */
#include <gdk/gdkx.h>
#include <gnome.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include <libwnck/libwnck.h>
#include <libwnck/screen.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>

#include "brightside.h"
#include "brightside-volume.h"
#ifdef USE_FBLEVEL
#include "brightside-fb-level.h"
#endif
#ifdef ENABLE_TRAY_ICON
#include "brightside-tray.h"
#endif

typedef struct {
	BrightsideVolume *volobj;
#ifdef USE_FBLEVEL
	BrightsideFblevel *levobj;
#endif
	struct {
		struct {
			gboolean enabled;
			gchar *action;
			gchar *custom_in;
			gchar *custom_out;
			gboolean custom_kill;
			gint custom_kill_pid;
		} corners[4];
		gint corner_delay;
		gboolean corner_flip;
		gboolean edge_flip;
		gchar *after_flip_command;
		gint edge_delay;
		gboolean edge_wrap;
		gboolean orientable_workspaces;
	} settings;
	
	GladeXML *xml;
	GtkWidget *dialog;
	GtkWidget *dialog_image;
	GConfClient *conf_client;
	guint dialog_timeout;
	guint screensaver_prod_timeout;
	gint screensaver_start_pid;
	guint lock_screen_timeout;
	guint pointer_watch_timeout;
	GtkWidget *pager;
	WnckPager *pager_pager;
	guint pager_timeout;
	WnckScreen *screen;

	/* Multihead stuff */
	GdkDisplay *display;
	GdkScreen *current_screen;
	GList *screens;

	BrightsideCornerType corner_in;
	BrightsideRegionType region_at;
	GTimeVal time_region_entered;
	/* Set in future to get progress bar to count down rather than up */
	GTimeVal time_progress_bar;
	GTimeVal time_corner_activated;
	GTimeVal time_edge_flipped;
	struct {
		BrightsideRegionType region;
		GTimeVal time_region_entered;
	} gesture_history [REGION_GESTURE_HISTORY];
} Brightside;

enum {
	ICON_MUTED,
	ICON_LOUD,
	ICON_BRIGHT,
	ICON_SCREENSAVER,
	ICON_DPMS,
	ICON_LOCK,
	ICON_CUSTOM,
};

enum {
	ACTION_START,
	ACTION_STOP,
};

typedef struct {
	int rows;
	int cols;
	gboolean vertical_workspaces;
	gint starting_corner;
	int workspaces;
	int current;
} workspace_info;

/* forward definitions */

gboolean watch_mouse (gpointer data);
static void pager_show (Brightside *brightside, WnckWorkspace *workspace, 
		int n_rows, gboolean from_scroll);
gboolean pager_enter_leave (GtkWidget *widget, GdkEventCrossing *event, 
		gpointer data);

/* main code */

static Window
my_wnck_screen_get_root (WnckScreen *screen, Display *display)
{
	XWindowAttributes attributes;
	WnckWindow *window = wnck_screen_get_active_window (screen);
	GList *list;
	if (window) {
		XGetWindowAttributes (display, wnck_window_get_xid (window), 
				&attributes);
		return attributes.root;
	}
	if (list = wnck_screen_get_windows (screen)) {
		window = g_list_first (list)->data;
		XGetWindowAttributes (display, wnck_window_get_xid (window), 
				&attributes);
		return attributes.root;
	}
	return None;
}

static Window
get_root_window_hint_window (Brightside *brightside, char *atom_name)
{
	Window xwindow = None;
	Display *xdisplay = gdk_x11_display_get_xdisplay (brightside->display);
	Window xroot = my_wnck_screen_get_root (brightside->screen, xdisplay);
	Atom xatom = XInternAtom (xdisplay, atom_name, False);
	gulong n_items = 0;
	int format = 0;
	guchar *prop = NULL;
	Atom type = None;
	gulong bytes_after = 0;
	if (XGetWindowProperty (xdisplay, xroot, xatom,
				0, G_MAXLONG, False, 
				XA_WINDOW, &type, &format,
				&n_items,
				&bytes_after,
				(guchar **)&prop) != Success ||
			type == None) {
		if (prop)
			XFree (prop);
		return None;
	}
	if (type == XA_WINDOW && n_items > 0) {
		xwindow = *((Window *) prop);
	}
	XFree (prop);
	return xwindow;
}

static gboolean
get_workspace_info (workspace_info *w_info, Brightside *brightside)
{
	int workspaces = wnck_screen_get_workspace_count (brightside->screen);
	int current = wnck_workspace_get_number (
			wnck_screen_get_active_workspace (brightside->screen));	
	gboolean vertical_workspaces = FALSE;
	gint columns_of_workspaces = 0, rows_of_workspaces = 0;
	gint starting_corner = NW;
	int cols = 0, rows = 0;

	/* Start EWMH communication */

	/* This code hacked out in complete desparation. Is there really no
	 * Gnome/GTK library that can tell me the neighbouring workspace? */
	Display *xdisplay = gdk_x11_display_get_xdisplay (brightside->display);
	Window xroot = my_wnck_screen_get_root (brightside->screen, xdisplay);
	Atom xatom;
	gulong n_items = 0;
	int format = 0;
	gulong *list = NULL;
	guchar *prop = NULL;
	Atom type = None;
	gulong bytes_after = 0;
	xatom = XInternAtom (xdisplay, DESKTOP_LAYOUT_ATOM, False);
	if (XGetWindowProperty (xdisplay, xroot, xatom,
				0, G_MAXLONG, False, 
				XA_CARDINAL, &type, &format,
				&n_items,
				&bytes_after,
				(guchar **)&prop) != Success ||
			type == None) {
		if (prop)
			XFree (prop);
		return FALSE;
	}
	list = (gulong *) prop;
	prop = NULL;
	if (n_items == 3 || n_items == 4) {

		switch (list[0]) {
			case 0:
				vertical_workspaces = FALSE;
				break;
			case 1:
				vertical_workspaces = TRUE;
				break;
			default:
				g_assert_not_reached();
		}
		cols = list[1];
		rows = list[2];
		g_assert (rows > 0 || cols > 0);
		rows_of_workspaces = (rows > 0) ? rows : -1;
		columns_of_workspaces = (cols > 0) ? cols : -1;
		if (n_items == 4) {
			switch (list[3]) {
				case 0:
					starting_corner = NW;
					break;
				case 1:
					starting_corner = NE;
					break;
				case 2:
					starting_corner = SE;
					break;
				case 3:
					starting_corner = SW;
					break;
				default:
					g_assert_not_reached();
			}
		}
	} else {
		g_assert_not_reached();
	}
	XFree (list);

	if (rows_of_workspaces == -1 && columns_of_workspaces == -1) {
		rows = 1; cols = workspaces;
	} else if (rows_of_workspaces == -1)
		rows = (workspaces + cols - 1) / cols;
	else if (columns_of_workspaces == -1)
		cols = (workspaces + rows - 1) / rows;
	else if (columns_of_workspaces * rows_of_workspaces < workspaces)
		cols = (workspaces + rows - 1) / rows;
	else if ((columns_of_workspaces - 1) * rows_of_workspaces >= workspaces)
		cols = (workspaces + rows - 1) / rows;
	else if ((rows_of_workspaces - 1) * columns_of_workspaces >= workspaces)
		rows = (workspaces + cols - 1) / cols;

	w_info->workspaces = workspaces;
	w_info->current = current;
	w_info->vertical_workspaces = vertical_workspaces;
	w_info->starting_corner = starting_corner;
	w_info->cols = cols;
	w_info->rows = rows;
	return TRUE;
}

static void
selection_get_func (GtkClipboard *clipboard, GtkSelectionData *selection_data,
		guint info, gpointer data)
{
	/* Hackiness: this is used for interprocess communication to request 
	 * the pager be shown. By extending the list of formats we can support 
	 * a small number of messages. */
#ifdef DEBUG
	g_print ("selection_get\n");
#endif
	Brightside *brightside = (Brightside *) data;
	workspace_info *w_info = g_new (workspace_info, 1);
	if (get_workspace_info (w_info, brightside) == TRUE)
		pager_show (brightside, wnck_screen_get_active_workspace (
					brightside->screen), 
				w_info->rows, FALSE);
	g_free (w_info);
	return;
}

static void
selection_clear_func (GtkClipboard *clipboard, gpointer data)
{       
#ifdef DEBUG
	g_print ("selection_clear\n");
#endif
	return;
}

static gboolean
brightside_get_lock (Brightside *brightside)
{
	gboolean result = FALSE;
	GtkClipboard *clipboard;
	Atom clipboard_atom = gdk_x11_get_xatom_by_name (SELECTION_NAME);
	static const GtkTargetEntry targets[] = {
		{ SELECTION_NAME, 0, 0 }
	};

	XGrabServer (GDK_DISPLAY());

	if (XGetSelectionOwner (GDK_DISPLAY(), clipboard_atom) != None)
		goto out;

	clipboard = gtk_clipboard_get (gdk_atom_intern (SELECTION_NAME, FALSE));

	if (!gtk_clipboard_set_with_data (clipboard, targets,
				G_N_ELEMENTS (targets),
				selection_get_func,
				selection_clear_func, (gpointer) brightside))
		goto out;

	result = TRUE;

out:
	XUngrabServer (GDK_DISPLAY());
	gdk_flush();

	return result;
}

static void
clipboard_received_func (GtkClipboard *clipboard, 
		GtkSelectionData *selection_data, gpointer data)
{
#ifdef DEBUG
	g_print ("received\n");
#endif
	if (data == NULL)
		gtk_main_quit ();
	return;
}

static gboolean
brightside_send_show_pager (gpointer data)
{
#ifdef DEBUG
	g_print ("sending show_pager\n");
#endif
	GdkAtom clip_atom = gdk_atom_intern (SELECTION_NAME, FALSE);
	GtkClipboard *clipboard = gtk_clipboard_get (clip_atom);
	gtk_clipboard_request_contents (clipboard, clip_atom, clipboard_received_func, data);
	/* let clipboard_received_func do the quitting */
	gtk_main ();
	return FALSE;
}

static void
brightside_exit (Brightside *brightside)
{
	g_free (brightside);
	exit (0);
}

#ifdef USE_FBLEVEL
static void
fb_problem_cb (void)
{
	char *msg;

	if (brightside_fblevel_is_powerbook () == FALSE)
		return;

	msg = permission_problem_string ("/dev/pmu");
	brightside_error (msg);
	g_free (msg);

	return;
}
#endif

static void
update_use_pcm_cb (GConfClient *client, guint id, GConfEntry *entry,
		gpointer data)
{
	Brightside *brightside = (Brightside *)data;
	gboolean use_pcm = FALSE;

	use_pcm = gconf_client_get_bool (brightside->conf_client,
			USE_PCM_KEY,
			NULL);
	brightside_volume_set_use_pcm (brightside->volobj, use_pcm);
}

static void
brightside_image_set_custom (Brightside *brightside, const gchar *cmd, 
		gboolean sensitive)
{
	GtkWidget *image = NULL;
	GtkWidget *vbox;

	vbox = glade_xml_get_widget (brightside->xml, "vbox2");
	g_return_if_fail (vbox != NULL);

	if (cmd && strlen(cmd) != 0) {
		gchar **argv;
		gint argc;
		if (g_shell_parse_argv (cmd, &argc, &argv, NULL)) {
			char *path;
			GnomeIconTheme *icon_theme;
			icon_theme = gnome_icon_theme_new ();
			path = gnome_icon_theme_lookup_icon (icon_theme,
					argv[0], 48, NULL, NULL);
			if (path != NULL) {
				image = gtk_image_new_from_file (path);
				g_free (path);
			}
			g_object_unref (icon_theme);
			g_strfreev (argv);
		}
	}
	if (!image) {
		image = gtk_image_new_from_stock (GTK_STOCK_EXECUTE, 
				GTK_ICON_SIZE_DIALOG);
	}
	if (brightside->dialog_image != NULL)
		gtk_widget_destroy (brightside->dialog_image);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (vbox), image, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (image, sensitive);
	brightside->dialog_image = image;
}

static void
brightside_image_set (Brightside *brightside, int icon, gboolean sensitive)
{
	GtkWidget *image = NULL;
	GtkWidget *vbox;

	vbox = glade_xml_get_widget (brightside->xml, "vbox2");
	g_return_if_fail (vbox != NULL);

	switch (icon) {
	case ICON_LOUD:
		image = gtk_image_new_from_file (
				BRIGHTSIDE_DATA "gnome-speakernotes.png");
		break;
	case ICON_MUTED:
		image = gtk_image_new_from_file (
				BRIGHTSIDE_DATA "gnome-speakernotes-muted.png");
		break;
	case ICON_BRIGHT:
		image = gtk_image_new_from_file (
				BRIGHTSIDE_DATA "brightside-brightness.png");
		break;
	case ICON_SCREENSAVER:
		image = gtk_image_new_from_file (
				BRIGHTSIDE_DATA "xscreensaver.png");
		break;
	case ICON_DPMS:
		image = gtk_image_new_from_file (
				BRIGHTSIDE_DATA "dpms.png");
		break;
	case ICON_LOCK:
		image = gtk_image_new_from_file (
				BRIGHTSIDE_DATA "gnome-lockscreen.png");
		break;
	default:
		g_assert_not_reached ();
	}
	
	if (brightside->dialog_image != NULL)
		gtk_widget_destroy (brightside->dialog_image);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (vbox), image, TRUE, TRUE, 0);
	gtk_widget_set_sensitive (image, sensitive);
	brightside->dialog_image = image;
}

static void
update_settings_cb (GConfClient *client, guint id, 
		GConfEntry *entry, gpointer data)
{
	Brightside *brightside = (Brightside *) data;
	gint i;

	g_return_if_fail (entry->key != NULL);

	/* Find the key that was modified */
	for (i = REGION_FIRST_CORNER; REGION_IS_CORNER (i); ++i) {
		if (!strcmp (entry->key, corners[i].enabled_key)) {
			brightside->settings.corners[i].enabled 
				= gconf_client_get_bool (
						brightside->conf_client,
						entry->key, NULL);
			return;
		} else if (!strcmp (entry->key, corners[i].action_key)) {
			brightside->settings.corners[i].action
				= gconf_client_get_string (
						brightside->conf_client,
						entry->key, NULL);
			return;
		} else if (!strcmp (entry->key, corners[i].custom_in_key)) {
			brightside->settings.corners[i].custom_in
				= gconf_client_get_string (
						brightside->conf_client,
						entry->key, NULL);
			return;
		} else if (!strcmp (entry->key, corners[i].custom_out_key)) {
			brightside->settings.corners[i].custom_out
				= gconf_client_get_string (
						brightside->conf_client,
						entry->key, NULL);
			return;
		} else if (!strcmp (entry->key, corners[i].custom_kill_key)) {
			brightside->settings.corners[i].custom_kill
				= gconf_client_get_bool (
						brightside->conf_client,
						entry->key, NULL);
			return;
		}
	}
	if (!strcmp (entry->key, CORNER_DELAY_KEY)) {
		brightside->settings.corner_delay = gconf_client_get_int (
				brightside->conf_client, entry->key, NULL);
		if (brightside->pointer_watch_timeout) {
			gtk_timeout_remove (brightside->pointer_watch_timeout);
			brightside->pointer_watch_timeout = gtk_timeout_add (
				CLAMP(MIN(brightside->settings.edge_delay, 
					brightside->settings.corner_delay) / 5,
				20, 200), watch_mouse, brightside);
		}
	} else if (!strcmp (entry->key, CORNER_FLIP_KEY)) {
		brightside->settings.corner_flip = gconf_client_get_bool (
				brightside->conf_client, entry->key, NULL);
	} else if (!strcmp (entry->key, ENABLE_EDGE_FLIP_KEY)) {
		brightside->settings.edge_flip = gconf_client_get_bool (
				brightside->conf_client, entry->key, NULL);
	} else if (!strcmp (entry->key, AFTER_FLIP_COMMAND_KEY)) {
		brightside->settings.after_flip_command = 
			gconf_client_get_string (
				brightside->conf_client, entry->key, NULL);
	} else if (!strcmp (entry->key, EDGE_DELAY_KEY)) {
		brightside->settings.edge_delay = gconf_client_get_int (
				brightside->conf_client, entry->key, NULL);
		if (brightside->pointer_watch_timeout) {
			gtk_timeout_remove (brightside->pointer_watch_timeout);
			brightside->pointer_watch_timeout = gtk_timeout_add (
				CLAMP (MIN (brightside->settings.edge_delay, 
					brightside->settings.corner_delay) / 5,
				20, 200), watch_mouse, brightside);
		}
	} else if (!strcmp (entry->key, EDGE_WRAP_KEY)) {
		brightside->settings.edge_wrap = gconf_client_get_bool (
				brightside->conf_client, entry->key, NULL);
	} else if (!strcmp (entry->key, ORIENTABLE_WORKSPACES_KEY)) {
		brightside->settings.orientable_workspaces = 
			gconf_client_get_bool (brightside->conf_client, 
					entry->key, NULL);
	}
}

static void
init_mouse (Brightside *brightside)
{
	int i;

	brightside->display = gdk_display_get_default ();
	brightside->screens = NULL;

	if (gdk_display_get_n_screens (brightside->display) == 1) {
		brightside->screens = g_list_append (brightside->screens,
				gdk_screen_get_default ());
	} else {
		for (i = 0; i < gdk_display_get_n_screens (brightside->display);
				i++) {
			GdkScreen *screen;
			screen = gdk_display_get_screen (
					brightside->display, i);
			if (screen != NULL)
				brightside->screens = g_list_append (
						brightside->screens, screen);
		}
	}

	gconf_client_notify_add (brightside->conf_client, BRIGHTSIDE_KEY_ROOT,
			update_settings_cb, brightside, NULL, NULL);
	for (i = REGION_FIRST_CORNER; REGION_IS_CORNER (i); ++i) {
		brightside->settings.corners[i].enabled = 
			gconf_client_get_bool (brightside->conf_client, 
				corners[i].enabled_key, NULL);
		brightside->settings.corners[i].action = 
			gconf_client_get_string (brightside->conf_client, 
					corners[i].action_key, NULL);
		brightside->settings.corners[i].custom_in = 
			gconf_client_get_string (brightside->conf_client, 
				corners[i].custom_in_key, NULL);
		brightside->settings.corners[i].custom_out	= 
			gconf_client_get_string (brightside->conf_client, 
				corners[i].custom_out_key, NULL);
		brightside->settings.corners[i].custom_kill = 
			gconf_client_get_bool (brightside->conf_client, 
					corners[i].custom_kill_key, NULL);
	}
	brightside->settings.corner_delay = 
		gconf_client_get_int (brightside->conf_client, 
			CORNER_DELAY_KEY, NULL);
	brightside->settings.corner_flip = 
		gconf_client_get_bool (brightside->conf_client, 
			CORNER_FLIP_KEY, NULL);
	brightside->settings.edge_flip = 
		gconf_client_get_bool (brightside->conf_client, 
			ENABLE_EDGE_FLIP_KEY, NULL);
	brightside->settings.after_flip_command = 
		gconf_client_get_string (brightside->conf_client, 
			AFTER_FLIP_COMMAND_KEY, NULL);
	brightside->settings.edge_delay = 
		gconf_client_get_int (brightside->conf_client, 
			EDGE_DELAY_KEY, NULL);
	brightside->settings.edge_wrap = 
		gconf_client_get_bool (brightside->conf_client,
			EDGE_WRAP_KEY, NULL);
	brightside->settings.orientable_workspaces = 
		gconf_client_get_bool (brightside->conf_client,
			ORIENTABLE_WORKSPACES_KEY, NULL);
}

static void
init_sm (Brightside *brightside)
{
	GnomeClient *master;
	GnomeClientFlags flags;

	master = gnome_master_client ();
	flags = gnome_client_get_flags (master);
	if (flags & GNOME_CLIENT_IS_CONNECTED) {
#ifdef DEBUG
		gnome_client_set_restart_style (master,	GNOME_RESTART_NEVER);
#else
		gnome_client_set_restart_style (master,
				GNOME_RESTART_IMMEDIATELY);
#endif
		gnome_client_flush (master);
	}

	g_signal_connect (GTK_OBJECT (master), "die",
			G_CALLBACK (brightside_exit), brightside);
}

static void 
dialog_move_to_screen_position (Brightside *brightside, GtkWidget *dialog,
		gfloat x_loc, gfloat y_loc, gboolean show_first)
{
	int orig_x, orig_y, orig_w, orig_h, orig_d;
	int screen_w, screen_h;
	int x, y;
	int pointer_x, pointer_y;
	GdkScreen *pointer_screen;
	GdkRectangle geometry;
	int monitor;

	gtk_window_set_screen (GTK_WINDOW (dialog), brightside->current_screen);
	gtk_widget_realize (GTK_WIDGET (dialog));

	pointer_screen = NULL;
	gdk_display_get_pointer (
			gdk_screen_get_display (brightside->current_screen),
			&pointer_screen, &pointer_x,
			&pointer_y, NULL);
	if (pointer_screen != brightside->current_screen) {
		/* The pointer isn't on the current screen, so just
		 * assume the default monitor
		 */
		monitor = 0;
	} else {
		monitor = gdk_screen_get_monitor_at_point (
				brightside->current_screen,
				pointer_x, pointer_y);
	}
		
	gdk_screen_get_monitor_geometry (brightside->current_screen, monitor,
					 &geometry);

	screen_w = geometry.width;
	screen_h = geometry.height;

	if (show_first)
		gtk_widget_show (dialog);

	gdk_window_get_geometry (GTK_WIDGET (dialog)->window,
				 &orig_x, &orig_y,
				 &orig_w, &orig_h, &orig_d);

	x = geometry.x + (screen_w * x_loc) - (orig_w / 2);
	y = geometry.y + (screen_h * y_loc) - (orig_h / 2);

	gdk_window_move (GTK_WIDGET (dialog)->window, x, y);

	/* this makes sure the dialog is actually shown */
	while (gtk_events_pending())
		gtk_main_iteration();
}

static gboolean
dialog_hide (Brightside *brightside)
{
	gtk_widget_hide (brightside->dialog);
	brightside->dialog_timeout = 0;
	return FALSE;
}

static void
dialog_show (Brightside *brightside, glong timeout_msec)
{
	dialog_move_to_screen_position (brightside, brightside->dialog,
			0.5, 0.75, TRUE);
	brightside->dialog_timeout = gtk_timeout_add (timeout_msec,
			(GtkFunction) dialog_hide, brightside);
}

static gboolean
update_bar_volume_cb (Brightside *brightside)
{
	gint vol;
	GtkWidget *progress;
	
	if (brightside->dialog_timeout == 0)
		return FALSE;
	vol = brightside_volume_get_volume (brightside->volobj);
	progress = glade_xml_get_widget (brightside->xml, "progressbar");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
			(double) vol / 100);
	return TRUE;
}

static gboolean
lock_screen_cb (gpointer data)
{
	g_spawn_command_line_async ("xdg-screensaver lock", NULL);
	return FALSE;
}

static gboolean
update_bar_timed_cb (Brightside *brightside)
{
	GTimeVal time_now;
	glong elapsed_msec;
	GtkWidget *progress;
	
	if (brightside->dialog_timeout == 0)
		return FALSE;
	g_get_current_time (&time_now);
	elapsed_msec = ABS (TIMEVAL_ELAPSED_MSEC (time_now, 
				brightside->time_progress_bar));
	if (elapsed_msec > DIALOG_TIMEOUT)
		return FALSE;
	progress = glade_xml_get_widget (brightside->xml, "progressbar");
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
			(double) elapsed_msec / DIALOG_TIMEOUT);
	return TRUE;
}

static void
show_dialog_timed_bar (Brightside *brightside, gint icon, gboolean is_enabled)
{
	GtkWidget *progress;
	glong timeout_msec = DIALOG_TIMEOUT; 
	progress = glade_xml_get_widget (brightside->xml, "progressbar");
	gtk_widget_set_sensitive (progress, is_enabled);
	if (icon != ICON_CUSTOM)
		brightside_image_set (brightside, icon, is_enabled);
	if (is_enabled) {
		GTimeVal time_now;
		GTimeVal time_start;
		g_get_current_time (&time_now);
		timeout_msec = TIMEVAL_ELAPSED_MSEC (
				brightside->time_progress_bar, time_now);
		timeout_msec = CLAMP (timeout_msec, 0, DIALOG_TIMEOUT);
		g_get_current_time (&time_start);
		time_start.tv_sec -= timeout_msec / 1000;
		time_start.tv_usec -= (timeout_msec % 1000) * 1000;
		brightside->time_progress_bar = time_start;
	} else {
		GTimeVal time_now;
		GTimeVal time_end;
		g_get_current_time (&time_now);
		timeout_msec = TIMEVAL_ELAPSED_MSEC (time_now, 
					brightside->time_progress_bar);
		timeout_msec = CLAMP (timeout_msec, 0, DIALOG_TIMEOUT);
		g_get_current_time (&time_end);
		time_end.tv_sec += timeout_msec / 1000;
		time_end.tv_usec += (timeout_msec % 1000) * 1000;
		brightside->time_progress_bar = time_end;
	}
	dialog_show (brightside, is_enabled ? DIALOG_TIMEOUT - timeout_msec : 
			timeout_msec);
	gtk_timeout_add (DIALOG_TIMEOUT / 100, 
			(GtkFunction) update_bar_timed_cb, brightside);
}

static void
cancel_lock_screen_action (Brightside *brightside)
{
	if (brightside->lock_screen_timeout != 0) {
		gtk_timeout_remove (brightside->lock_screen_timeout);
		brightside->lock_screen_timeout = 0;
	}
	if (brightside->screensaver_start_pid) {
		kill_child_pid (brightside->screensaver_start_pid);
		brightside->screensaver_start_pid = 0;
	}
}

/**
 * Begin do_..._actions
 **/

static void
do_mute_action (Brightside *brightside, gint action)
{
	GtkWidget *progress;
	gboolean muted;
	gint vol;

	if (brightside->dialog_timeout != 0) {
		gtk_timeout_remove (brightside->dialog_timeout);
		brightside->dialog_timeout = 0;
	}

	muted = brightside_volume_get_mute(brightside->volobj);
	if (muted != (action == ACTION_START)) {
		brightside_volume_mute_toggle_fade (brightside->volobj, 
				DIALOG_TIMEOUT);
		muted = !muted;
	}
	
	brightside_image_set (brightside, 
			muted ? ICON_MUTED : ICON_LOUD, !muted);

	vol = brightside_volume_get_volume (brightside->volobj);
	progress = glade_xml_get_widget (brightside->xml, "progressbar");
	gtk_widget_set_sensitive (progress, !muted);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
			(double) vol / 100);

	dialog_show (brightside, DIALOG_TIMEOUT);
	gtk_timeout_add (DIALOG_TIMEOUT / 100, 
			(GtkFunction) update_bar_volume_cb, brightside);
}

gboolean
disable_screensaver_cb (gpointer data)
{
	g_spawn_command_line_async ("xdg-screensaver reset", NULL);
	return TRUE;
}

static void
do_no_screensaver_action (Brightside *brightside, gint action)
{
	gboolean is_enabled = (action == ACTION_STOP);

	if (brightside->dialog_timeout != 0) {
		gtk_timeout_remove (brightside->dialog_timeout);
		brightside->dialog_timeout = 0;
	}
	if (brightside->screensaver_prod_timeout != 0) {
		gtk_timeout_remove (brightside->screensaver_prod_timeout);
		brightside->screensaver_prod_timeout = 0;
	}

	if (!is_enabled)
		brightside->screensaver_prod_timeout = gtk_timeout_add (30000,
				disable_screensaver_cb, brightside);

	/* oops - should de-overload time_corner_activated 
	GTimeVal time_now;
	g_get_current_time (&time_now);
	glong timeout_msec = TIMEVAL_ELAPSED_MSEC (time_now, 
				brightside->time_corner_activated); */
	show_dialog_timed_bar (brightside, ICON_SCREENSAVER, is_enabled);
}

static void
do_screensaver_action (Brightside *brightside, gint action)
{
	if (brightside->dialog_timeout != 0) {
		gtk_timeout_remove (brightside->dialog_timeout);
		brightside->dialog_timeout = 0;
	}

	show_dialog_timed_bar (brightside, (action == ACTION_START) ? 
			ICON_SCREENSAVER : ICON_LOCK, (action == ACTION_START));

	if (action == ACTION_START) {
		execute ("xdg-screensaver activate", FALSE,
				&brightside->screensaver_start_pid);
		brightside->lock_screen_timeout = gtk_timeout_add 
			(DIALOG_TIMEOUT, 
			 (GtkFunction) lock_screen_cb, brightside);
	} else {
		cancel_lock_screen_action (brightside);
	}
}

static void
do_dpms_action (Brightside *brightside, gint type, gint action)
{
	if (brightside->dialog_timeout != 0) {
		gtk_timeout_remove (brightside->dialog_timeout);
		brightside->dialog_timeout = 0;
	}

	show_dialog_timed_bar (brightside, (action == ACTION_START) ? 
			ICON_DPMS : ICON_LOCK, (action == ACTION_START));

	if (action == ACTION_START) {
		switch (type) {
		case DPMS_STANDBY_ACTION:
			g_spawn_command_line_async ("xset dpms force standby", 
					NULL);
			break;
		case DPMS_SUSPEND_ACTION:
			g_spawn_command_line_async ("xset dpms force suspend", 
					NULL);
			break;
		case DPMS_OFF_ACTION:
			g_spawn_command_line_async ("xset dpms force off", 
					NULL);
			break;
		default:
			g_assert_not_reached ();
		}
		brightside->lock_screen_timeout = gtk_timeout_add (
				DIALOG_TIMEOUT, 
				(GtkFunction) lock_screen_cb, brightside);
	} else {
		cancel_lock_screen_action (brightside);
	}
}

static void
do_showdesktop_action (Brightside *brightside, gint action)
{
       if (action == ACTION_START) {
               WnckScreen *screen = brightside->screen;
               
               if (wnck_screen_get_showing_desktop(screen)) {
                       wnck_screen_toggle_showing_desktop(screen, FALSE);
               } else {
                       wnck_screen_toggle_showing_desktop(screen, TRUE);
               }
       }
}

#ifdef USE_FBLEVEL
static void
do_dimbacklight_action (Brightside *brightside, gint action)
{
	GtkWidget *progress;
	gboolean is_dim = (action == ACTION_START);
	gint level;

	if (brightside->dialog_timeout != 0) {
		gtk_timeout_remove (brightside->dialog_timeout);
		brightside->dialog_timeout = 0;
	}
	
	brightside_fblevel_set_dim (brightside->levobj, is_dim);

	brightside_image_set (brightside, ICON_BRIGHT, !is_dim);

	progress = glade_xml_get_widget (brightside->xml, "progressbar");
	gtk_widget_set_sensitive (progress, !is_dim);
	if (is_dim)
		level = 0;
	else
		level = brightside_fblevel_get_level (brightside->levobj);
	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress),
			(double) level / 15);

	dialog_show (brightside, DIALOG_TIMEOUT);
}
#endif

static void
do_custom_action (Brightside *brightside, gint corner, gint action)
{
	gboolean is_enabled = (action == ACTION_START);

	if (is_enabled) {
		brightside_image_set_custom (brightside, 
				brightside->settings.corners[corner].custom_in,
				TRUE);
		execute (brightside->settings.corners[corner].custom_in, FALSE, 
			&brightside->settings.corners[corner].custom_kill_pid
			);
	} else if (brightside->settings.corners[corner].custom_kill) {
		brightside_image_set_custom (brightside, 
				brightside->settings.corners[corner].custom_in,
				FALSE);
		kill_child_pid (
			brightside->settings.corners[corner].custom_kill_pid);
	} else {
		brightside_image_set_custom (brightside, 
				brightside->settings.corners[corner].custom_in,
				TRUE);
		execute (brightside->settings.corners[corner].custom_out, 
				FALSE, NULL);
	}
	
	dialog_hide (brightside);
}

static void
do_action (Brightside *brightside, gint corner, gint type, gint action)
{
#ifdef DEBUG
	g_print ("do_action, type is: %d\n", type);
#endif
	switch (type) {
	case MUTE_VOLUME_ACTION:
		if (brightside->volobj != NULL)
			do_mute_action (brightside, action);
		break;
	case NO_SCREENSAVER_ACTION:
		do_no_screensaver_action (brightside, action);
		break;
	case SCREENSAVER_ACTION:
		do_screensaver_action (brightside, action);
		break;
	case DPMS_STANDBY_ACTION:
	case DPMS_SUSPEND_ACTION:
	case DPMS_OFF_ACTION:
		do_dpms_action (brightside, type, action);
		break;
	case SHOWDESKTOP_ACTION:
		do_showdesktop_action (brightside, action);
		break;
#ifdef USE_FBLEVEL
	case DIM_BACKLIGHT_ACTION:
		if (brightside->levobj != NULL)
			do_brightness_action (brightside, action);
		break;
#endif
	case CUSTOM_ACTION:
		do_custom_action (brightside, corner, action);
		break;
	default:
		g_assert_not_reached ();
	}
}

/* *
 * deorient_workspace: when orientable_workspaces is false and edge_wrap is 
 * true, the workspace layout becomes in effect a Klein bottle rather than
 * a torus (yes: although the standard model for a Klein bottle is a square 
 * with vertical edges identified parallel and horizontal edges identified 
 * antiparallel, we get the same surface identifying both sets of edges 
 * antiparallel; think about it, does the resulting surface have any handles? 
 * - no, so it's either a sphere or a Klein bottle. Question: how do we add a
 * handle? Answer: leave a hole (not at the edge), identify opposite edges
 * (parallel for a handle, antiparallel for a cross-cap).
 * Thus when we wrap around across an antiparallel identified edge, we reenter
 * the workspace grid in effect rotated around the axis perpendicular to the
 * edge wrapped across. We choose to implement this by keeping windows on the
 * same numbered workspaces, entering the antiparallel opposite workspace,
 * warping the pointer to the antiparallel opposite point, and rotating all
 * workspaces around the axis indicated. We do not invert the pointer 
 * direction, amusing as that would be.
 * To rotate a workspace around an axis, we simply reverse the stacking order
 * and move all non-sticky, non-toplevel, non-bottomlevel windows such that
 * their new and original window rectangles (not including window decorations)
 * form mirror images around the axis of rotation. We do not show the reverse 
 * side of windows (though see Sun's Java Desktop System); nor do we flip the 
 * contents of the windows themselves.
 * Unfortunately, Metacity keeps its own list of the `default' window on each 
 * workspace, which we do not have access to; it reactivates that window when 
 * switching workspace; this messes up the window reordering but there's
 * nothing we can do about it short of switching to each workspace in turn and 
 * giving focus to the lowest window there. This would be far too much effort, 
 * especially as giving focus doesn't "take" without doing a gdk_display_sync 
 * to give the window manager time to catch up - which would display each 
 * desktop in turn, wasting time and looking ugly. Instead, one needs to 
 * reconceptualise the active window as having its own stacking layer.
 * */
static void
deorient_workspaces (Brightside *brightside, Window avoid, gint edge)
{							/* or corner */
	WnckScreen *screen = brightside->screen;
	Display *xdisplay = gdk_x11_display_get_xdisplay (brightside->display);
	GList *list = wnck_screen_get_windows (screen);
	GList *tmp;
	int i, j;
	WnckWindow *windows [g_list_length (list)];

	/* in bottom-to-top order */
	list = wnck_screen_get_windows_stacked (screen);
	/* Loop through windows, flipping their position and adding them 
	 * to the restacking array */
	for (i = 0, tmp = g_list_first (list); tmp != NULL; 
			tmp = g_list_next (tmp)) {
		WnckWindow *window = tmp->data;
		WnckWindowType type;
		g_assert (WNCK_IS_WINDOW (window));
		type = wnck_window_get_window_type (window);
		if (type == WNCK_WINDOW_NORMAL || type == WNCK_WINDOW_DIALOG
				|| type == WNCK_WINDOW_TOOLBAR
				|| type == WNCK_WINDOW_MENU
				|| type == WNCK_WINDOW_UTILITY
				|| type == WNCK_WINDOW_SPLASHSCREEN) {
			WnckWorkspace *workspace = wnck_window_get_workspace (
					window);
			Window xwindow = wnck_window_get_xid (window);
			int wwidth, wheight, x, y, width, height;
			XWindowAttributes attributes;
			if (xwindow == avoid)
				continue;
			wwidth = wnck_workspace_get_width (workspace);
			wheight = wnck_workspace_get_height (workspace);
			/* Trouble: this gets position of internal window,
			 * not including decorations. However, XMoveWindow
			 * moves the decorated window. So, use 
			 * XGetWindowAttributes as x, y are offsets of
			 * internal window from decorations. */
			wnck_window_get_geometry (window, 
					&x, &y, &width, &height);
			XGetWindowAttributes (xdisplay, xwindow, &attributes);
			if ((edge == LEFT || edge == RIGHT) &&
					!wnck_window_is_maximized (window) &&
					!wnck_window_is_maximized_horizontally
					(window))
				XMoveWindow (xdisplay, xwindow,
						x - attributes.x, 
						wheight - y - height
						- attributes.y);
			else if ((edge == TOP || edge == BOTTOM) &&
					!wnck_window_is_maximized (window) &&
					!wnck_window_is_maximized_vertically
					(window))
				XMoveWindow (xdisplay, xwindow,	
						wwidth - x - width
						- attributes.x, 
						y - attributes.y);
			else if (REGION_IS_CORNER (edge) && 
					!wnck_window_is_maximized (window) &&
					!wnck_window_is_maximized_vertically
					(window) &&
					!wnck_window_is_maximized_horizontally
					(window))
				XMoveWindow (xdisplay, xwindow,	
						wwidth - x - width
						- attributes.x, 
						wheight - y - height
						- attributes.y);
			windows [i++] = window;
		}
	}
	/* Restack from top to bottom - XRestackWindows doesn't seem to work */
	if (REGION_IS_EDGE (edge))
		for (j = 0; j < i; ++j)
			XLowerWindow (xdisplay, 
					wnck_window_get_xid(windows[j]));
}

static gboolean
applet_scroll (GtkWidget *pager, GdkEventScroll *event, gpointer data)
{
	Brightside *brightside = (Brightside *) data;

	WnckWorkspace *active_workspace =
		wnck_screen_get_active_workspace (brightside->screen);
	int workspace_index = wnck_workspace_get_number (active_workspace);
	int workspace_count = wnck_screen_get_workspace_count (brightside->screen);
	int n_columns, n_rows, new_index;
	WnckWorkspace *new_workspace;

	workspace_info *w_info = g_new (workspace_info, 1);
	if (get_workspace_info (w_info, brightside) == FALSE) {
		g_free (w_info);
		return FALSE;
	}
	n_columns = w_info->cols;
	n_rows = w_info->rows;
	g_free (w_info);

	new_index = workspace_index;

	if (event->type != GDK_SCROLL)
		return FALSE;
	
	switch (event->direction) {
	case GDK_SCROLL_DOWN:
		if (workspace_index + n_columns < workspace_count)
			new_index = workspace_index + n_columns;
		else if (workspace_index < workspace_count - 1)
			new_index = workspace_index % n_columns + 1;
		else
			new_index = 0;
		break;
		
	case GDK_SCROLL_RIGHT:
		if (workspace_index < workspace_count)
			new_index = workspace_index + 1;
		break;
		
	case GDK_SCROLL_UP:
		if (workspace_index - n_columns >= 0)
			new_index = workspace_index - n_columns;
		else if (workspace_index > 0) {
			new_index = (n_rows - 1) * n_columns;
			new_index += workspace_index % n_columns - 1;
		}
		else
			new_index = workspace_count - 1;
		break;

	case GDK_SCROLL_LEFT:
		if (workspace_index > 0)
			new_index = workspace_index - 1;
		break;
	}

#ifdef DEBUG
	g_print ("new index %d\n", new_index);
#endif

	new_workspace = wnck_screen_get_workspace (brightside->screen, new_index);
	if (new_workspace) {
		wnck_workspace_activate (new_workspace, event->time);
		pager_show (brightside, new_workspace, n_rows, TRUE);
	}
	
	return TRUE;
}

static void
pager_setup (Brightside *brightside)
{
	/* From test-pager.c */
	GtkWidget *pager;
	GtkWidget *vbox;
	vbox = glade_xml_get_widget (brightside->xml, "vbox3");
	g_return_if_fail (vbox != NULL);

	pager = wnck_pager_new (brightside->screen);
	wnck_pager_set_orientation (WNCK_PAGER (pager), 
			GTK_ORIENTATION_HORIZONTAL);
	wnck_pager_set_shadow_type (WNCK_PAGER (pager), GTK_SHADOW_NONE);
	gtk_box_pack_start (GTK_BOX (vbox), pager, TRUE, TRUE, 0);
	brightside->pager_pager = WNCK_PAGER (pager);
	gtk_widget_show (pager);
	g_signal_connect (G_OBJECT (pager), "scroll-event",
			G_CALLBACK (applet_scroll), brightside);
}

static gboolean
pager_hide (Brightside *brightside)
{
	gtk_widget_hide (brightside->pager);
	brightside->pager_timeout = 0;
	return FALSE;
}

static void
pager_show (Brightside *brightside, WnckWorkspace *workspace, 
		int n_rows, gboolean from_scroll)
{
	GtkWidget *label;
#ifdef DEBUG
	g_print("pager_show %d %d\n", n_rows, from_scroll);
#endif
	label = glade_xml_get_widget (brightside->xml, 
			"workspace_label");
	gtk_label_set_text (GTK_LABEL (label), 
			wnck_workspace_get_name (workspace));

	wnck_pager_set_n_rows (brightside->pager_pager, n_rows);

	if (brightside->pager_timeout)
		gtk_timeout_remove (brightside->pager_timeout);
	else
		dialog_move_to_screen_position (brightside, brightside->pager,
				0.5, 0.5, TRUE);
	
	if (!from_scroll)
		brightside->pager_timeout = gtk_timeout_add (3 * MAX (
					brightside->settings.corner_flip ?
					brightside->settings.corner_delay : 0,
					brightside->settings.edge_flip ?
					brightside->settings.edge_delay : 0),
				(GtkFunction) pager_hide, brightside);
}

gboolean
pager_enter_leave (GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	Brightside *brightside;
#ifdef DEBUG
	g_print("pager_enter_leave %d %d\n", event->type, event->detail);
#endif
	brightside = (Brightside *) data;
	if (event->type == GDK_LEAVE_NOTIFY && 
			event->detail != GDK_NOTIFY_INFERIOR)
		gtk_widget_hide (brightside->pager);
	if (brightside->pager_timeout) {
		gtk_timeout_remove (brightside->pager_timeout);
		brightside->pager_timeout = 0;
	}
	return FALSE;
}

static void 
do_edge_flip (Brightside *brightside, gint edge) /* or corner flip, now */
{
	int i, workspaces, current, cols, rows, original, new_space = 0;
	gint starting_corner, wrapped_point = 0, oldx, oldy, x, y;
	gboolean vertical_workspaces, have_wrapped = FALSE;
	Display *xdisplay;
	WnckWorkspace *new_workspace;
	GdkScreen *screen;

#ifdef DEBUG
	g_print ("Request edge flip: edge is %d\n", edge);
#endif
	workspace_info *w_info = g_new (workspace_info, 1);
	if (get_workspace_info (w_info, brightside) == FALSE) {
		g_free (w_info);
		return;
	}
	workspaces = w_info->workspaces;
	current = w_info->current;
	vertical_workspaces = w_info->vertical_workspaces;
	starting_corner = w_info->starting_corner;
	cols = w_info->cols;
	rows = w_info->rows;
	g_free (w_info);
	g_return_if_fail (workspaces > 0);
#ifdef DEBUG
	g_print ("Workspace layout: %d rows, %d cols, orientation: %d, "
			"starting corner: %d, %d workspaces, current is %d, "
			"are%s orientable, are%s wrapping\n",
                	rows, cols,
			vertical_workspaces, starting_corner,
			workspaces, current, 
			brightside->settings.orientable_workspaces ?"":" not",
			brightside->settings.edge_wrap ? "" : " not");
#endif
	original = current;

	new_space = 0;
	have_wrapped = FALSE;
	wrapped_point = 0;
	xdisplay = gdk_x11_display_get_xdisplay (brightside->display);

	Window xmoving = None;
	WnckWindow *moving = NULL;
	Window xresizing = None;

	/* Originally removed pursuant to bug 131630 on bugzilla.gnome.org,
	   re-enabled by ari@debian.org since this is fixed as of metacity 2.8 */
	/* Now, this is where it gets messy.
	 * If a window is being moved, we want it to come with us. 
	 * If a window is being resized, we don't want to change workspace at
	 * all. */
	xmoving = get_root_window_hint_window (brightside, MOVING_WINDOW_ATOM);
	if (xmoving != None)
		moving = wnck_window_get (xmoving);
	
	xresizing = get_root_window_hint_window (brightside, 
			RESIZING_WINDOW_ATOM);
	if (xresizing != None) {
#ifdef DEBUG
		g_print ("Window %lx is being resized: abort\n", xresizing);
#endif
		return;
	} 
	
	/* Special case: a 1-dimensional layout is a cylinder (or Moebius 
	 * strip), 0- and 2- dimensional layouts have closed topologies. */
	if (((edge == TOP || edge == BOTTOM) && (rows == 1 && cols != 1))
	|| ((edge == LEFT || edge == RIGHT) && (rows != 1 && cols == 1)))
		return;
	
	/* In order to handle gaps in the workspace layout: loop at most 
	 * workspaces times */
	for (i = 0; i < workspaces; ++i) {
		gint current_col, current_row;

		current_col = vertical_workspaces
			? current / rows : current % cols;
		if (starting_corner == NE || starting_corner == SE)
			current_col = (cols - 1) - current_col;
		current_row = vertical_workspaces
			? current % rows : current / cols;
		if (starting_corner == SW || starting_corner == SE)
			current_row = rows - 1 - current_row;
#ifdef DEBUG
		g_print ("current cell is (%d, %d); ", current_col, current_row);
#endif
		switch (edge) {
			case TOP: 	--current_row; break;
			case BOTTOM:	++current_row; break;
			case LEFT:	--current_col; break;
			case RIGHT:	++current_col; break;
			case NW: --current_row; --current_col; break;
			case SW: ++current_row; --current_col; break;
			case NE: --current_row; ++current_col; break;
			case SE: ++current_row; ++current_col; break;
		}
		if (brightside->settings.edge_wrap) {
			if (current_col < 0 || current_col >= cols ||
					current_row < 0 || current_row >= rows)
				have_wrapped = !have_wrapped;
			if (!brightside->settings.orientable_workspaces) {
				if (current_col < 0) {
					if (current_row < 0) 
						wrapped_point = NW;
					else if (current_row >= rows)
						wrapped_point = SW;
					else
						wrapped_point = LEFT;
				} else if (current_col >= cols) {
					if (current_row < 0) 
						wrapped_point = NE;
					else if (current_row >= rows)
						wrapped_point = SE;
					else
						wrapped_point = RIGHT;
				} else {
					if (current_row < 0) 
						wrapped_point = TOP;
					else if (current_row >= rows)
						wrapped_point = BOTTOM;
					else
						wrapped_point = NONE;
				}
				if (current_col < 0 )
					current_row = rows - 1 - current_row;
				if (current_col >= cols)
					current_row = rows - 1 - current_row;
				if (current_row < 0)
					current_col = cols - 1 - current_col;
				if (current_row >= rows)
					current_col = cols - 1 - current_col;
			}
			if (current_col < 0) current_col += cols;
			if (current_col >= cols) current_col -= cols;
			if (current_row < 0) current_row += rows;
			if (current_row >= rows) current_row -= rows;
		} else {
			if (current_col < 0) current_col = 0;
			if (current_col >= cols) current_col = cols - 1;
			if (current_row < 0) current_row = 0;
			if (current_row >= rows) current_row = rows - 1;
		}
#ifdef DEBUG
		g_print ("new cell is (%d, %d); ", current_col, current_row);
#endif
		new_space = vertical_workspaces ?
			( ( (starting_corner == NW || starting_corner == SW) ?
			    current_col : (cols - 1 - current_col) ) * rows +
			  ( (starting_corner == NW || starting_corner == NE) ?
			    current_row : (rows - 1 - current_row) ) ) :
			( ( (starting_corner == NW || starting_corner == NE) ?
			    current_row : (rows - 1 - current_row) ) * cols +
			  ( (starting_corner == NW || starting_corner == SW) ?
			    current_col : (cols - 1 - current_col) ) );
#ifdef DEBUG
		g_print ("proposed new workspace is %d; ", new_space);
#endif
		/* if the new workspace is valid: brilliant! */
		if (new_space < workspaces) break;
#ifdef DEBUG
		g_print ("invalid; ");
#endif
		/* if on a gap and not edge_wrapping, don't change workspace */
		if (!brightside->settings.edge_wrap) {
			new_space = current;
#ifdef DEBUG
			g_print ("hit a gap, aborting; ");
#endif
			break;
		}
		/* if edge_wrapping, keep looping till we find a workspace -
		 * this may be the original one, of course! */
		current = new_space;
#ifdef DEBUG
		g_print ("keeping going! \n");
#endif
	}
#ifdef DEBUG
	g_print ("new workspace is %d\n", new_space);
#endif

	if (new_space == original && !have_wrapped)
		return;
	
	if (have_wrapped && !brightside->settings.orientable_workspaces) {
#ifdef DEBUG
		g_print ("Deorienting workspaces around the %s axis\n", 
				REGION_IS_CORNER (wrapped_point) ? "z" :
				((wrapped_point == TOP
				  || wrapped_point == BOTTOM)
				 ? "north-south" : "east-west"));
#endif
#if 0
		deorient_workspaces (brightside, xmoving, wrapped_point);
#else
		deorient_workspaces (brightside, None, wrapped_point);
#endif
	}

	new_workspace = wnck_screen_get_workspace (
			brightside->screen, new_space);
	wnck_workspace_activate (new_workspace, gtk_get_current_event_time());

	pager_show (brightside, new_workspace, rows, FALSE);
	
	gdk_display_get_pointer (brightside->display, &screen, 
			&oldx, &oldy, NULL);
	x = gdk_screen_get_width(screen);
	y = gdk_screen_get_height(screen);
	if (edge == TOP || edge == BOTTOM) {
		y -= 10;
		if (edge == BOTTOM)
			y = -y;
		x = 0;
	} else if (edge == LEFT || edge == RIGHT) {
		x -= 10;
		if (edge == RIGHT)
			x = -x;
		y = 0;
	} else {
		x -= 10; y -= 10;
		if (edge == NE || edge == SE)
			x = -x;
		if (edge == SW || edge == SE)
			y = -y;
	}
	if (have_wrapped && !brightside->settings.orientable_workspaces) {
		if (edge == TOP || edge == BOTTOM)
			x = gdk_screen_get_width(screen) - 2 * oldx;
		else if (edge == LEFT || edge == RIGHT)
			y = gdk_screen_get_height(screen) - 2 * oldy;
		else if (wrapped_point == TOP || wrapped_point == BOTTOM) {
			y = gdk_screen_get_height(screen) - 2 * oldy;
			x = (edge == NW || edge == SW) ? 10 : -10;
		} else if (wrapped_point == LEFT || wrapped_point == RIGHT) {
			x = gdk_screen_get_width(screen) - 2 * oldx;
			y = (edge == NW || edge == NE) ? 10 : -10;
		} else {
			x = (edge == NW || edge == SW) ? 10 : -10;
			y = (edge == NW || edge == NE) ? 10 : -10;
		}
	}
#ifdef DEBUG
	g_print ("Warp pointer by: %d, %d ", x, y);
#endif
	XWarpPointer (xdisplay, None, None, 0, 0, 0, 0, x, y);
	gdk_display_get_pointer (brightside->display, &screen, &x, &y, NULL);
	brightside->current_screen = screen;
#ifdef DEBUG
	g_print ("gives %d, %d\n", x, y);
#endif
	execute (brightside->settings.after_flip_command, FALSE, NULL);
#if 0
	if (moving != NULL) {
		int moving_dx, moving_dy; /* offset of pointer from window */
		/* Use XGetWindowAttributes to get offset of window from 
		 * decorations; FIXME assumes right and bottom decorations are
		 * same width as left decoration! */
		XWindowAttributes attributes;
		XGetWindowAttributes (xdisplay, xmoving, &attributes);
#ifdef DEBUG
		g_print ("Attributes: %d, %d, %d x %d\n", 
				attributes.x, attributes.y, 
				attributes.width, attributes.height);
#endif
		int mx, my, mwidth, mheight;
		wnck_window_get_geometry (moving, &mx, &my, &mwidth, &mheight);
		moving_dx = oldx + attributes.x - mx;
		if (moving_dx < 0) 
			moving_dx = 0;
		if (moving_dx >= mwidth + attributes.x)
			moving_dx = mwidth + attributes.x - 1;
		moving_dy = oldy + attributes.y - my;
		if (moving_dy < 0) 
			moving_dy = 0;
		if (moving_dy >= mheight + attributes.x) 
			moving_dy = mheight + attributes.x - 1;
#ifdef DEBUG
		g_print ("Window %lx is being moved: bringing it along, moving "
				"it to %d, %d (pointer offset is %d, %d, was "
				"%d, %d)\n", 
				xmoving, x - moving_dx, y - moving_dy,
				moving_dx, moving_dy, oldx + attributes.x - mx, 
				oldy + attributes.y - my);
#endif
		wnck_window_move_to_workspace (moving, new_workspace);
		wnck_window_activate (moving);
		XMoveWindow (xdisplay, xmoving, x - moving_dx, y - moving_dy);
		/* Send _NET_WM_MOVERESIZE to ensure the WM keeps the window
		 * in the 'moving' state. */
		XEvent xev;
		xev.xclient.type = ClientMessage;
		xev.xclient.serial = 0;
		xev.xclient.send_event = True;
		xev.xclient.display = xdisplay;
		xev.xclient.window = xmoving;
		xev.xclient.message_type = XInternAtom (xdisplay, 
				WM_MOVERESIZE_ATOM, False);
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = x;
		xev.xclient.data.l[1] = y;
		xev.xclient.data.l[2] = _NET_WM_MOVERESIZE_MOVE;
		xev.xclient.data.l[3] = 0;

		XSendEvent (xdisplay, xroot, False,
			SubstructureRedirectMask | SubstructureNotifyMask,
			&xev); 
	}
#endif
}

gboolean
watch_mouse (gpointer data)
{
	Brightside *brightside = (Brightside *) data;
	GdkScreen *screen;
	gint x, y;
	gint i;
	gint region, corner, action;
	
	gdk_display_get_pointer (brightside->display, &screen,
			&x, &y, NULL);
	brightside->current_screen = screen;
	if (x > 0 && y > 0 && x < gdk_screen_get_width (screen) - 1
			&& y < gdk_screen_get_height (screen) - 1) {
		if (brightside->corner_in != NONE) {
			corner = brightside->corner_in;
			if (brightside->settings.corners[corner].enabled && 
					gconf_string_to_enum (
						actions_lookup_table,
						brightside->settings
						.corners[corner].action,
						&action))
				do_action (brightside, corner, 
						action, ACTION_STOP);
			brightside->corner_in = NONE;
		}
		brightside->region_at = NONE;
		return TRUE;
	}
	if (x == 0) {
		if (y == 0) 
			region = NW;
		else if (y == gdk_screen_get_height (screen) - 1)
			region = SW;
		else
			region = LEFT;
	} else if (x == gdk_screen_get_width (screen) - 1) {
		if (y == 0)
			region = NE;
		else if (y == gdk_screen_get_height (screen) - 1) 
			region = SE;
		else
			region = RIGHT;
	} else if (y == 0) 
		region = TOP; 
	else
		region = BOTTOM;
	if (brightside->corner_in != NONE && brightside->corner_in != region) {
		/* Just moved out of a corner we had triggered */
		corner = brightside->corner_in;
		if (brightside->settings.corners[corner].enabled && 
				gconf_string_to_enum (actions_lookup_table,
				brightside->settings.corners[corner].action,
				&action))
			do_action (brightside, corner, 
					action, ACTION_STOP);
		brightside->corner_in = NONE;
	} else if (brightside->region_at != NONE
			&& brightside->region_at != region) {
		/* Just moved out of a region not yet triggered */
		brightside->corner_in = NONE;
	} else if (brightside->corner_in == region) {
		/* Still in the triggered region */
		;
	} else if (brightside->region_at == region) {
		/* Still in a region not yet triggered */
		GTimeVal time_now;
		glong elapsed_msec, delay;
		g_get_current_time (&time_now);
		elapsed_msec = TIMEVAL_ELAPSED_MSEC (time_now,
				brightside->time_region_entered);
		delay = REGION_IS_CORNER (region)
			? brightside->settings.corner_delay
			: brightside->settings.edge_delay;
		/* Check if it's time to trigger the region */
		if (elapsed_msec > delay) {
			if (
(REGION_IS_CORNER (region) && brightside->settings.corner_flip) ||
  (REGION_IS_EDGE (region) && brightside->settings.edge_flip)) {
				brightside->time_edge_flipped = time_now;
				do_edge_flip (brightside, region);
			} else if (REGION_IS_CORNER (region)) {
				brightside->time_corner_activated = time_now;
				if (brightside->settings.corners[region].enabled
						&& gconf_string_to_enum (
							actions_lookup_table,
							brightside->settings
							.corners[region].action,
							&action))
					do_action (brightside, region, 
							action, ACTION_START);
				brightside->corner_in = region;
			} 
			for (i = 0; i < REGION_GESTURE_HISTORY; ++i)
				brightside->gesture_history[i].region = NONE;
		}
	} 
	if (brightside->region_at != region) {
		/* Moved into a new region */
		/* More edge flips come without resistance for a time of
		 * MAX (flip delay) ? 2 - this allows moving back, corner 
		 * flipping even when corner_flip is false, and, for the 
		 * nimble, covering multiple screens in one action. 
		 * Allow corner flipping even when corner_flip is false as 
		 * it's near impossible to hit the corner when edge flipping 
		 * is resistance-less and we don't want to confuse users. */
		gboolean have_just_flipped = FALSE;
		g_get_current_time (&brightside->time_region_entered);
		if ((REGION_IS_CORNER (region)
					&& brightside->settings.corner_flip) ||
				brightside->settings.edge_flip) {
			GTimeVal time_now;
			glong elapsed_msec, delay;
			g_get_current_time (&time_now);
			elapsed_msec = TIMEVAL_ELAPSED_MSEC (time_now,
					brightside->time_edge_flipped);
			delay = 2 * MAX (
					brightside->settings.corner_flip ?
					brightside->settings.corner_delay : 0,
					brightside->settings.edge_flip ?
					brightside->settings.edge_delay : 0);
			if (elapsed_msec <= delay) {
				brightside->time_edge_flipped = time_now;
				do_edge_flip (brightside, region);
				have_just_flipped = TRUE;
			} else { /* Too slow */
				glong delay2 = REGION_IS_CORNER (region)
					? brightside->settings.corner_delay
					: brightside->settings.edge_delay;
				if (elapsed_msec <= delay + delay2) {
/* They weren't that slow - make the trigger time the same distance into the
 * future as they missed the extra-flip cutoff time by */
				brightside->time_region_entered.tv_sec -= 
					(delay + delay2 - elapsed_msec)	/ 1000;
				brightside->time_region_entered.tv_usec -= 
				((delay + delay2 - elapsed_msec) % 1000) * 1000;
				}
			}
		} /* cop-out to avoid extra nesting */
#if 0
		if (!have_just_flipped && REGION_IS_CORNER (region) && 
			((get_root_window_hint_window 
			 (brightside, MOVING_WINDOW_ATOM) != None &&
			 !brightside->settings.corner_flip) ||
			 get_root_window_hint_window
			 (brightside, RESIZING_WINDOW_ATOM) != None)) {
			/* A window is being moved (and we aren't corner 
			 * flipping) or resized into a corner: pretend that we 
			 * are not in that corner */
			region = NONE;
		} /* cop-out to avoid extra nesting */
#endif
		if (!have_just_flipped && 
				!REGION_CORNER_TO_ADJACENT_EDGE (
					brightside->region_at, region)) {
			gchar gesture [REGION_GESTURE_HISTORY + 1];
			gchar *gesture_found;

			for (i = 1; i < REGION_GESTURE_HISTORY; ++i)
				brightside->gesture_history[i-1] = 
					brightside->gesture_history[i];
			brightside->gesture_history[i-1].region = region;
			brightside->gesture_history[i-1].time_region_entered =
					brightside->time_region_entered;
			for (i = 0; i < REGION_GESTURE_HISTORY; ++i)
				gesture[i] = 
					(brightside->gesture_history[i].region
					 == NONE)
					? ' '
					: REGION_GESTURE_CODES [
					brightside->gesture_history[i].region];
			gesture[i] = '\0';
#ifdef DEBUG
			g_print ("Testing gesture: [%s]", gesture);
#endif
			if ((gesture_found = g_strrstr (gesture, 
						EASTER_EGG_GESTURE))) {
				glong elapsed_msec, delay;
				
				i = (gint) (gesture_found - gesture);
				elapsed_msec = TIMEVAL_ELAPSED_MSEC (
						brightside->time_region_entered,
						brightside->gesture_history[i]
						.time_region_entered);
				delay = MAX (
					brightside->settings.corner_delay,
					brightside->settings.edge_delay)
					* 6;
				if (elapsed_msec < delay) {
					/* Not quite gegls from outer space */
					gchar *url = g_strdup_printf (
"%s%La", BRIGHTSIDE_URL, *(long double *) PACKAGE_STRING BRIGHTSIDE_URL);
					gnome_url_show (url, NULL);
					g_print ("\nNot quite gegls from outer"
							"space...\n");
					g_free (url);
					for (i = 0; i < REGION_GESTURE_HISTORY;
							++i)
						brightside->gesture_history[i]
							.region = NONE;
				} else {
					g_print (
"Timed out! You took: %ldms Target was: %ldms YOU FAIL IT!!! :p\n",
							elapsed_msec, delay);
				}
			}
		}
	}
	brightside->region_at = region;
	
	return TRUE;
}

/* Any extra code to initialise the Brightside struct */
static void
init_brightside_struct (Brightside *brightside)
{
	gint i;
	for (i = 0; i < REGION_GESTURE_HISTORY; ++i)
		brightside->gesture_history[i].region = NONE;
}

int
main (int argc, char *argv[])
{
	Brightside *brightside;

	GnomeProgram *brightside_program = NULL;
	gboolean show_pager = FALSE, show_version = FALSE;
	struct poptOption cmd_options_table[] = {
		{"pager", 'p', POPT_ARG_NONE, &show_pager, 0, 
			_("Show the pager"), NULL},
		{"version", 'v', POPT_ARG_NONE, &show_version, 0, 
			_("Show the version of brightside"), NULL},
		{0}};

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
	
	brightside_program = gnome_program_init ("brightside", VERSION,
			LIBGNOMEUI_MODULE,
			argc, argv, GNOME_PARAM_POPT_TABLE, cmd_options_table,
			NULL);
	gnome_program_parse_args (brightside_program);
	if (show_version == TRUE) {
		g_print ("%s\n", VERSION);
		return 0;
	}
			
	/* Stuff relies on the struct being initialised to 0 */
	brightside = g_new0 (Brightside, 1);
	init_brightside_struct (brightside);

	if (brightside_get_lock (brightside) == FALSE) {
		if (show_pager == TRUE)
			brightside_send_show_pager (NULL);
		else
			g_print (_("Daemon already running, exiting...\n"));
		brightside_exit (brightside);
	}

	glade_gnome_init ();
	brightside->xml = glade_xml_new (BRIGHTSIDE_DATA "brightside.glade", 
			NULL, NULL);

	if (brightside->xml == NULL) {
		brightside_error (_("Couldn't load the Glade file.\n"
				"Make sure that this daemon is properly"
				" installed."));
		exit (1);
	}

	brightside->dialog = glade_xml_get_widget (brightside->xml, "dialog");
	brightside_image_set (brightside, ICON_LOUD, TRUE);
	brightside->pager = glade_xml_get_widget (brightside->xml, "pager");
	g_signal_connect (G_OBJECT (brightside->pager), "enter-notify-event",
			G_CALLBACK (pager_enter_leave), (gpointer) brightside);
	g_signal_connect (G_OBJECT (brightside->pager), "leave-notify-event",
			G_CALLBACK (pager_enter_leave), (gpointer) brightside);

	brightside->conf_client = gconf_client_get_default ();
	gconf_client_add_dir (brightside->conf_client,
			BRIGHTSIDE_KEY_ROOT,
			GCONF_CLIENT_PRELOAD_ONELEVEL,
			NULL);

	init_mouse (brightside);
	init_sm (brightside);
#ifdef ENABLE_TRAY_ICON
	init_tray ();
#endif
	brightside->current_screen = gdk_screen_get_default ();
	brightside->screen = wnck_screen_get_default ();
	gtk_widget_realize (brightside->dialog);
	dialog_move_to_screen_position (brightside, brightside->dialog,
			0.5, 0.75, FALSE);
	pager_setup (brightside);
	gtk_widget_realize (brightside->pager);
	dialog_move_to_screen_position (brightside, brightside->pager,
			0.5, 0.5, FALSE);

	/* initialise Volume handler */
	brightside->volobj = brightside_volume_new();
	if (brightside->volobj != NULL) {
		brightside_volume_set_use_pcm (brightside->volobj,
				gconf_client_get_bool (brightside->conf_client,
					USE_PCM_KEY, NULL));
		gconf_client_notify_add (brightside->conf_client,
				USE_PCM_KEY, update_use_pcm_cb,
				brightside, NULL, NULL);
	}

#ifdef USE_FBLEVEL
	/* initialise Frame Buffer level handler */
	brightside->levobj = brightside_fblevel_new();
	if (brightside->levobj == NULL)
		fb_problem_cb ();
#endif

	/* Start filtering the events */
	brightside->corner_in = brightside->region_at = NONE;
	brightside->pointer_watch_timeout = gtk_timeout_add (
			CLAMP(MIN(brightside->settings.edge_delay, 
				brightside->settings.corner_delay) / 5,
			20, 200), watch_mouse, brightside);
	
	if (show_pager == TRUE)
		gtk_timeout_add (10, brightside_send_show_pager, brightside);

	gtk_main ();
	
	g_free (brightside);
	
	return 0;
}

