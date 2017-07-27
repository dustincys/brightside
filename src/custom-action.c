/* BRIGHTSIDE
 * Copyright (C) 2004 Ed Catmur <ed@catmur.co.uk>
 *
 * custom-action.c
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

#include <config.h>

#include <sys/file.h>
#include <gnome.h>
#include <glade/glade.h>
#include <gconf/gconf-client.h>
#include <gdk/gdkx.h>

/* from gnome-control-center common */
#include "gconf-property-editor.h"
#include "capplet-util.h"

#include "custom-action.h"
#include "brightside.h"
#include "brightside-properties-shared.h"

static void
run_toggled_cb (GtkToggleButton *toggle, GladeXML *dialog)
{
	gtk_widget_set_sensitive (WID ("out_entry"), 
			gtk_toggle_button_get_active (toggle));
}

static void
dialog_button_clicked_cb (GtkDialog *dialog, gint response_id, 
		GConfChangeSet *changeset)
{
	if (response_id == GTK_RESPONSE_HELP)
		gnome_url_show (HELP_URL, NULL);
	else
		gtk_widget_destroy (GTK_WIDGET (dialog));
}

static GladeXML *
create_dialog (void)
{
	GladeXML *dialog;

	dialog = glade_xml_new (BRIGHTSIDE_DATA "custom-action.glade", 
			"action_widget", NULL);
	g_object_set_data (G_OBJECT (WID ("action_widget")), 
			"glade-data", dialog);

	return dialog;
}

static void
setup_dialog (gint corner, GladeXML *dialog, 
		GConfClient *client, GConfChangeSet *changeset)
{
	GObject *peditor;
	GtkWidget *entry;
	gboolean b;
	
	gconf_peditor_new_string (NULL, 
			(gchar *) corners[corner].custom_in_key,
			WID ("in_entry"), NULL);
	gconf_peditor_new_boolean (NULL,
			(gchar *) corners[corner].custom_kill_key,
			WID ("terminate_radio"), NULL);
	g_signal_connect (GTK_OBJECT (WID ("run_radio")), "toggled",
			G_CALLBACK (run_toggled_cb), dialog);
	b = !gconf_client_get_bool (client, 
			corners[corner].custom_kill_key, NULL);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (WID ("run_radio")), b);
	gtk_widget_set_sensitive (WID ("out_entry"), b);
	gconf_peditor_new_string (NULL, 
			(gchar *) corners[corner].custom_out_key,
			WID ("out_entry"), NULL);
}

void
show_custom_action_dialog (GtkWindow *parent, gint corner, 
		GConfClient *client, GConfChangeSet *changeset)
{
	GladeXML *dialog = NULL;
	GtkWidget *dialog_win;

	dialog = create_dialog ();
	setup_dialog (corner, dialog, client, changeset);

	dialog_win = gtk_dialog_new_with_buttons(
			_("Custom action"), parent, 
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT
				| GTK_DIALOG_NO_SEPARATOR,
			GTK_STOCK_HELP, GTK_RESPONSE_HELP,
			GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			NULL);
	gtk_container_set_border_width (GTK_CONTAINER (dialog_win), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog_win)->vbox), 2);
	gtk_dialog_set_default_response (GTK_DIALOG (dialog_win), 
			GTK_RESPONSE_CLOSE);
	g_signal_connect (G_OBJECT (dialog_win), "response", 
			(GCallback) dialog_button_clicked_cb, changeset);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog_win)->vbox), 
			WID ("action_widget"), TRUE, TRUE, 0);
	gtk_window_set_resizable (GTK_WINDOW (dialog_win), FALSE);
	gtk_window_set_icon (GTK_WINDOW (dialog_win), 
			gtk_widget_render_icon (dialog_win, 
				GTK_STOCK_EXECUTE, GTK_ICON_SIZE_DIALOG, NULL));
	gtk_widget_show_all (dialog_win);
}

