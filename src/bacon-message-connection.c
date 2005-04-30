/*
 * Copyright (C) 2003 Bastien Nocera <hadess@hadess.net>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "bacon-message-connection.h"

#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

struct BaconMessageConnection {
	/* A server accepts connections */
	gboolean is_server;

	/* The socket path itself */
	char *path;

	/* FD is for the connection, serverfd is for the server socket,
	 * it accepts incoming connections. */
	int fd, serverfd, server_conn_id, conn_id;
	GIOChannel *chan;

	/* callback */
	void (*func) (const char *message, gpointer user_data);
	gpointer data;
};

static gboolean
test_is_socket (const char *path)
{
	struct stat s;

	if (stat (path, &s) == -1)
		return FALSE;

	if (S_ISSOCK (s.st_mode))
		return TRUE;

	return FALSE;
}

static gboolean server_cb (GIOChannel *source,
			   GIOCondition condition, gpointer data);

static gboolean
setup_connection (BaconMessageConnection *conn)
{
	conn->chan = g_io_channel_unix_new (conn->fd);
	if (!conn->chan) {
		return FALSE;
	}
	g_io_channel_set_line_term (conn->chan, "\n", 1);
	conn->conn_id = g_io_add_watch (conn->chan, G_IO_IN, server_cb, conn);

	return TRUE;
}

static gboolean
server_cb (GIOChannel *source, GIOCondition condition, gpointer data)
{
	BaconMessageConnection *conn = (BaconMessageConnection *)data;
	char *message, *subs, buf;
	int cd, alen, rc, offset;
	gboolean finished;

	offset = 0;
	if (conn->serverfd == g_io_channel_unix_get_fd (source)) {
		cd = accept (conn->serverfd, NULL, (guint *)&alen);
		conn->fd = cd;
		setup_connection (conn);
		return TRUE;
	}
	message = g_malloc (1);
	cd = conn->fd;
	rc = read (cd, &buf, 1);
	while (rc > 0 && buf != '\n')
	{
		message = g_realloc (message, rc + offset + 1);
		message[offset] = buf;
		offset = offset + rc;
		rc = read (cd, &buf, 1);
	}
	if (rc <= 0) {
		g_io_channel_shutdown (conn->chan, FALSE, NULL);
		g_io_channel_unref (conn->chan);
		conn->chan = NULL;
		close (conn->fd);
		conn->fd = -1;
		g_free (message);
		conn->conn_id = 0;

		return FALSE;
	}
	message[offset] = '\0';

	subs = message;
	finished = FALSE;

	while (*subs != '\0' && finished == FALSE)
	{
		if (message != NULL && conn->func != NULL)
			(*conn->func) (subs, conn->data);

		subs += strlen (subs) + 1;
		if (subs - message >= offset)
			finished = TRUE;
	}

	g_free (message);

	return TRUE;
}

static char *
socket_filename (const char *prefix)
{
	char *filename, *path;
	const char *dir;

	filename = g_strdup_printf (".%s.%s", prefix, g_get_user_name ());

	dir = g_getenv ("BACON_SOCKET_DIR");
	if (dir == NULL)
	{
		path = g_build_filename (g_get_home_dir (), filename, NULL);
	} else {
		path = g_build_filename (dir, filename, NULL);
	}

	g_free (filename);
	return path;
}

static gboolean
try_server (BaconMessageConnection *conn)
{
	struct sockaddr_un uaddr;

	uaddr.sun_family = AF_UNIX;
	strncpy (uaddr.sun_path, conn->path,
			MIN (strlen(conn->path)+1, UNIX_PATH_MAX));
	conn->serverfd = conn->fd = socket (PF_UNIX, SOCK_STREAM, 0);
	if (bind (conn->fd, (struct sockaddr *) &uaddr, sizeof (uaddr)) == -1)
	{
		conn->fd = -1;
		return FALSE;
	}
	listen (conn->fd, 5);

	if (!setup_connection (conn))
		return FALSE;
	conn->server_conn_id = conn->conn_id;
	return TRUE;
}

static gboolean
try_client (BaconMessageConnection *conn)
{
	struct sockaddr_un uaddr;

	uaddr.sun_family = AF_UNIX;
	strncpy (uaddr.sun_path, conn->path,
			MIN(strlen(conn->path)+1, UNIX_PATH_MAX));
	conn->fd = socket (PF_UNIX, SOCK_STREAM, 0);
	conn->serverfd = -1;
	if (connect (conn->fd, (struct sockaddr *) &uaddr,
				sizeof (uaddr)) == -1)
	{
		conn->fd = -1;
		return FALSE;
	}

	return setup_connection (conn);
}

BaconMessageConnection *
bacon_message_connection_new (const char *prefix)
{
	BaconMessageConnection *conn;

	g_return_val_if_fail (prefix != NULL, NULL);

	conn = g_new0 (BaconMessageConnection, 1);
	conn->path = socket_filename (prefix);

	if (test_is_socket (conn->path) == FALSE)
	{
		if (!try_server (conn))
		{
			bacon_message_connection_free (conn);
			return NULL;
		}

		conn->is_server = TRUE;
		return conn;
	}

	if (try_client (conn) == FALSE)
	{
		unlink (conn->path);
		try_server (conn);
		if (conn->fd == -1)
		{
			bacon_message_connection_free (conn);
			return NULL;
		}

		conn->is_server = TRUE;
		return conn;
	}

	conn->is_server = FALSE;
	return conn;
}

void
bacon_message_connection_free (BaconMessageConnection *conn)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (conn->path != NULL);

	if (conn->server_conn_id) {
		g_source_remove (conn->server_conn_id);
		conn->server_conn_id = 0;
	}
	if (conn->conn_id) {
		g_source_remove (conn->conn_id);
		conn->conn_id = 0;
	}
	if (conn->chan) {
		g_io_channel_shutdown (conn->chan, FALSE, NULL);
		g_io_channel_unref (conn->chan);
	}

	if (conn->is_server != FALSE) {
		unlink (conn->path);
		close (conn->serverfd);
	} else if (conn->fd != -1) {
		close (conn->fd);
	}

	g_free (conn->path);
	g_free (conn);
}

void
bacon_message_connection_set_callback (BaconMessageConnection *conn,
				       BaconMessageReceivedFunc func,
				       gpointer user_data)
{
	g_return_if_fail (conn != NULL);

	conn->func = func;
	conn->data = user_data;
}

void
bacon_message_connection_send (BaconMessageConnection *conn,
			       const char *message)
{
	g_return_if_fail (conn != NULL);
	g_return_if_fail (message != NULL);

	g_io_channel_write_chars (conn->chan, message, strlen (message),
				  NULL, NULL);
	g_io_channel_write_chars (conn->chan, "\n", 1, NULL, NULL);
	g_io_channel_flush (conn->chan, NULL);
}

gboolean
bacon_message_connection_get_is_server (BaconMessageConnection *conn)
{
	g_return_val_if_fail (conn != NULL, FALSE);

	return conn->is_server;
}

