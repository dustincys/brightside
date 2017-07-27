/* BRIGHTSIDE
 * Copyright (C) 2004 Ed Catmur <ed@catmur.co.uk>
 * Portions copyright (C) 2001 Bastien Nocera <hadess@hadess.net>
 *
 * brightside-util.c
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

#include <gnome.h>

#include "brightside-util.h"

void
brightside_error (char *msg)
{
	GtkWidget *error_dialog;

	error_dialog =
	    gtk_message_dialog_new (NULL,
			    GTK_DIALOG_MODAL,
			    GTK_MESSAGE_ERROR,
			    GTK_BUTTONS_OK,
			    "%s", msg);
	gtk_dialog_set_default_response (GTK_DIALOG (error_dialog),
			GTK_RESPONSE_OK);
	gtk_widget_show (error_dialog);
	gtk_dialog_run (GTK_DIALOG (error_dialog));
	gtk_widget_destroy (error_dialog);
}

gboolean 
spawn_command_line_async_pid (const gchar *command_line, GError **error, 
		gint *child_pid)
{
	gint argc;
	gchar **argv;
	gboolean retval;
	
	retval = g_shell_parse_argv (command_line, &argc, &argv, error);
	if (retval)
		g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, 
				NULL, NULL, child_pid, error);
	if (argv)
		g_strfreev (argv);
	return retval;
}

void
execute (char *cmd, gboolean sync, gint *child_pid)
{
	gboolean retval;
	g_assert (!sync || child_pid == NULL);


	if (cmd == NULL)
		return;
	if (strlen (cmd) == 0)
		return;

	if (sync != FALSE)
		retval = g_spawn_command_line_sync
			(cmd, NULL, NULL, NULL, NULL);
	else
		retval = spawn_command_line_async_pid (cmd, NULL, child_pid);

	if (retval == FALSE)
	{
		char *msg;

		msg = g_strdup_printf
			(_("Couldn't execute command: %s\n"
			   "Verify that this command exists."),
			 cmd);

		brightside_error (msg);
		g_free (msg);
	}
}

void
kill_child_pid (gint child_pid)
{
	if (child_pid > 1)
		kill (child_pid, 15);
}

#if 0
char*
permission_problem_string (const char *files)
{
	return g_strdup_printf (_("Permissions on the file %s are broken\n"
				"Please check Brightside's documentation, "
				"correct the problem and restart Brightside."), 
			files);
}
#endif


