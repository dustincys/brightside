/* BRIGHTSIDE
 * Copyright (C) 2004 Ed Catmur <ed@catmur.co.uk>
 * Portions copyright (C) 2001 Bastien Nocera <hadess@hadess.net>
 *
 * brightside-util.h
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

#ifndef __BRIGHTSIDE_UTIL_H__
#define __BRIGHTSIDE_UTIL_H__

void brightside_error (char *msg);
gboolean spawn_command_line_async_pid (const gchar *command_line, 
		GError **error, gint *child_pid);
void execute (char *cmd, gboolean sync, gint *child_pid);
void kill_child_pid (gint child_pid);

#endif /* __BRIGHTSIDE_UTIL_H */
