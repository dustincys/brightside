/* BRIGHTSIDE
 * Copyright (C) 2004 Ed Catmur <ed@catmur.co.uk>
 * Portions copyright (C) 2001 Bastien Nocera <hadess@hadess.net>
 *
 * brightside-tray.c
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

#include <gnome.h>
#include "eggtrayicon.h"

#include "brightside-tray.h"

#define DESCRIPTION "Screen Actions daemon"

typedef struct {
	EggTrayIcon *icon;
	GtkTooltips *icon_tooltip;
	GtkWidget *popup_menu;
} BrightsideTray;

static void
prefs_activated (GtkMenuItem *menuitem, gpointer user_data)
{
	execute ("brightside-properties", FALSE, NULL);
}

static void
about_activated (GtkMenuItem *menuitem, gpointer user_data)
{
	static GtkWidget *about = NULL;
	GdkPixbuf *pixbuf = NULL;
	const gchar *authors[] = { "Ed Catmur <ed@catmur.co.uk>", NULL };
	const gchar *documenters[] = { NULL };
	const gchar *translator_credits = _("translator_credits");
	gpointer about_as_gpointer;

	if (about != NULL) {
		gdk_window_raise (about->window);
		gdk_window_show (about->window);
		return;
	}

	pixbuf = gdk_pixbuf_new_from_file (
			BRIGHTSIDE_DATA "brightside.svg", NULL);

	about = gnome_about_new(PACKAGE_NAME, VERSION,
			"Copyright \xc2\xa9 2004 Ed Catmur", _(DESCRIPTION),
			(const char **)authors,
			(const char **)documenters,
			strcmp (translator_credits, "translator_credits") != 0
			 ? translator_credits : NULL,
			pixbuf);
	
	if (pixbuf != NULL)
		gdk_pixbuf_unref (pixbuf);
	
	g_signal_connect (G_OBJECT (about), "destroy",
			G_CALLBACK (gtk_widget_destroyed), &about);
	about_as_gpointer = (gpointer) about;
	g_object_add_weak_pointer (G_OBJECT (about), &about_as_gpointer);

	gtk_widget_show(about);
}

static void
quit_activated (GtkMenuItem *menuitem, gpointer user_data)
{
	GnomeClient *master;
	GnomeClientFlags flags;

	/* User initiated quit: remove from session management */
	master = gnome_master_client ();
	flags = gnome_client_get_flags (master);
	if (flags & GNOME_CLIENT_IS_CONNECTED) {
		gnome_client_set_restart_style (master,	GNOME_RESTART_NEVER);
		gnome_client_flush (master);
	}

	g_free (user_data);

	gtk_main_quit();
}

static gboolean
tray_icon_release (GtkWidget *widget, GdkEventButton *event, 
		BrightsideTray *tray)
{
	if (event->button == 3) {
		gtk_menu_popdown (GTK_MENU (tray->popup_menu));
		return FALSE;
	}

	return TRUE;
}

static gboolean
tray_icon_press (GtkWidget *widget, GdkEventButton *event, 
		BrightsideTray *tray)
{
	if (event->button == 3) {
		gtk_menu_popup (GTK_MENU (tray->popup_menu), NULL, NULL, 
				NULL, NULL, event->button, event->time);
		return TRUE;
	}

	return FALSE;
}

static gboolean
tray_destroyed (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	g_free (user_data);
	init_tray ();
	return TRUE;
}

void
init_tray (void)
{
	GtkWidget *image, *evbox, *item;

	BrightsideTray *tray = g_new0 (BrightsideTray, 1);
	tray->icon = egg_tray_icon_new (_(DESCRIPTION));
	image = gtk_image_new_from_file (BRIGHTSIDE_DATA "brightside-16.png");

	tray->icon_tooltip = gtk_tooltips_new ();
	gtk_tooltips_set_tip (tray->icon_tooltip, GTK_WIDGET (tray->icon),
			_(DESCRIPTION " active"), NULL);

	/* Event box */
	evbox = gtk_event_box_new ();
	g_signal_connect (G_OBJECT (evbox), "button_press_event",
			G_CALLBACK (tray_icon_press), (gpointer) tray);
	g_signal_connect (G_OBJECT (evbox), "button_release_event",
			G_CALLBACK (tray_icon_release), (gpointer) tray);

	/* Popup menu */
	tray->popup_menu = gtk_menu_new ();
	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES,
			NULL);
	g_signal_connect (G_OBJECT (item), "activate",
			G_CALLBACK (prefs_activated), (gpointer) tray);
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (tray->popup_menu), item);

	item = gtk_image_menu_item_new_from_stock (GNOME_STOCK_ABOUT,
			NULL);
	g_signal_connect (G_OBJECT (item), "activate",
			G_CALLBACK (about_activated), (gpointer) tray);
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (tray->popup_menu), item);

	item = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT,
			NULL);
	g_signal_connect (G_OBJECT (item), "activate",
			G_CALLBACK (quit_activated), (gpointer) tray);
	gtk_widget_show (item);
	gtk_menu_shell_append (GTK_MENU_SHELL (tray->popup_menu), item);

	gtk_container_add (GTK_CONTAINER (evbox), image);
	gtk_container_add (GTK_CONTAINER (tray->icon), evbox);
	gtk_container_set_border_width (GTK_CONTAINER (tray->icon), 2);
	gtk_widget_show_all (GTK_WIDGET (tray->icon));

	g_signal_connect (G_OBJECT (tray->icon), "destroy-event",
			G_CALLBACK (tray_destroyed), (gpointer) tray);
}


