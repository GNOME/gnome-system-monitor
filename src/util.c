
#define _GNU_SOURCE
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/wait.h>


#ifdef __FreeBSD__
# include <errno.h>
# include <libutil.h>
#endif

#include "util.h"

/* ABORT() kills GTK if we're not root, else it just exits.
 */
#define ABORT(root)			\
		if (root == 0)		\
			GTK_ABORT();	\
		else			\
			_exit(-1)

/* GTK_ABORT() is supposed to kill GTK and exit.
 */
#define GTK_ABORT() do {			\
			gtk_main_quit();	\
			_exit(0);		\
		    } while (0)

/* OPEN_TTY() is supposed to return a file descriptor to a pseudo-terminal.
 */
#define OPEN_TTY() getpt()

#ifdef __FreeBSD__
/* FreeBSD doesn't have getpt(). This function emulates it's behaviour. */
int getpt (void);

int
getpt ()
{
	int master, slave;

	if (openpty (&master, &slave, NULL, NULL, NULL) < 0) {
		/* Simulate getpt()'s only error condition. */
		errno = ENOENT;
		return -1;
	}
	return master;
}
#endif

static int root;			/* if we are root, no password is
                                           required */

static gint
exec_su (gchar *exec_path, gchar *user, gchar *pwd)
{
	gchar *exec_p, *user_p;  /* command to execute, user name */
	pid_t pid;
	int t_fd;
	gint error;

	exec_p = g_strdup (exec_path);

#if 0
	if (asprintf (&exec_p, "%s&", exec_path) < 0) {
		perror ("Unable to allocate memory chunk");
		return 0;
	}
#endif

	user_p = (user ? user : "root");

	if ((pwd == NULL) || (*pwd == '\0'))
		return 0;

	/*
	 * Make su think we're sending the password from a terminal:
	 */

	if ((t_fd = OPEN_TTY()) < 0) {
		fprintf (stderr, "Unable to open a terminal\n");
		ABORT (root);
	}

	if ((pid = fork()) < 0) {
		perror ("Unable to fork a new process");
		ABORT (root);
	}

	if (pid > 0) {			/* parent process */
		int status;

		/* su(1) won't want a password if we're already root.
		 */
		if (root == 0)
			write (t_fd, pwd, strlen(pwd));

		waitpid (pid, &status, 0);

		if (WIFEXITED (status) && WEXITSTATUS (status) && (WEXITSTATUS(status) < 255)) {
/*			error_box (_("Incorrect password.")); */
			return 0;
		}
		else {
			memset (pwd, 0, strlen (pwd));
			_exit (0);
		}
	}
	else {				/* child process */
		struct passwd *pw;
		char *env, *home;

		/* We have rights to run X (obviously).  We need to ensure the
		 * destination user has the right stuff in the environment
		 * to be able to continue to run X.
		 * su will change $HOME to the new users home, so we will
		 * need an XAUTHORITY / ICEAUTHORITY pointing to the
		 * authorization files.
		 */

		if ((home = getenv ("HOME")) == NULL) {
			if ((env = getenv ("USER")) == NULL)
				pw = getpwuid(getuid());
			else
				pw = getpwnam(env);
			if (pw)
				home = pw->pw_dir;
			else {
				perror ("Unable to find home directory");
				_exit (-1);
			}
		}

		if ((env = getenv ("XAUTHORITY")) == NULL) {
			if (asprintf (&env, "XAUTHORITY=%s/.Xauthority", home) > 0)
				putenv (env);
			else {
				perror ("Unable to allocate memory chunk");
				_exit (-1);
			}
		}

		if ((env = getenv ("ICEAUTHORITY")) == NULL) {
			if (asprintf (&env, "ICEAUTHORITY=%s/.ICEauthority", home) > 0)
				putenv (env);
			else {
				perror ("Unable to allocate memory chunk");
				_exit (-1);
			}
		}

		dup2 (t_fd, 0);

#if 0
		freopen ("/dev/null", "w", stderr);
		freopen ("/dev/null", "w", stdout);
#endif

		sleep (1);
		g_print ("begin \n");
		error = fork ();
		if (error != -1)
			error = execlp ("su", "su", "-m", user_p, "-c", exec_p, NULL);
		g_print ("end \n");
		/*_exit (0);*/
	}

	return 0;
}

void
su_run_with_password (gchar *exec_path, gchar *password)
{
	exec_su (exec_path, "root", password);
}

