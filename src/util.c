
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

#ifdef __sun
# include <errno.h>
# include <unistd.h>
# include <stropts.h>
# include <sys/stream.h>
# include <sys/termios.h>
# include <sys/ptem.h>
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

#ifdef __sun
/* Sun doesn't have getpt().  This function emulates it's behavior. */
int getpt (void);

int
getpt ()
{
	int fdm, fds;
	extern int errno;
	char *slavename;
	/* extern char *ptsname(); */

	/* open master */
	fdm = open("/dev/ptmx", O_RDWR);
	if ( fdm < 0 ) {
		/* Simulate getpt()'s only error condition. */
		errno = ENOENT;
		return -1;
	}
	if ( grantpt(fdm) < 0 )	{	/* change permission ofslave */
		/* Simulate getpt()'s only error condition. */
		errno = ENOENT;
		return -1;
	}
	if ( unlockpt(fdm) < 0 ) {	/* unlock slave */
		/* Simulate getpt()'s only error condition. */
		errno = ENOENT;
		return -1;
	}
	slavename = ptsname(fdm);	/* get name of slave */
	if ( slavename == NULL ) {
		/* Simulate getpt()'s only error condition. */
		errno = ENOENT;
		return -1;
	}
	fds = open(slavename, O_RDWR);        /* open slave */
	if ( fds < 0 ) {
		/* Simulate getpt()'s only error condition. */
		errno = ENOENT;
		return -1;
	}
	ioctl(fds, I_PUSH, "ptem");           /* push ptem * /
	ioctl(fds, I_PUSH, "ldterm");         /* push ldterm */

	return fds;
}

/* Handle the fact that solaris doesn't have an asprintf */
/* pinched from 
http://samba.anu.edu.au/rsync/doxygen/head/snprintf_8c-source.html */

#ifndef HAVE_VASPRINTF
int vasprintf(char **ptr, const char *format, va_list ap)
{
	int ret;
        
	ret = vsnprintf(NULL, 0, format, ap);
	if (ret <= 0) return ret;

	(*ptr) = (char *)malloc(ret+1);
	if (!*ptr) return -1;
	ret = vsnprintf(*ptr, ret+1, format, ap);

	return ret;
}
#endif


#ifndef HAVE_ASPRINTF
 int asprintf(char **ptr, const char *format, ...)
{
	va_list ap;
	int ret;
        
	va_start(ap, format);
	ret = vasprintf(ptr, format, ap);
	va_end(ap);

	return ret;
}
#endif
#endif

static int root = 0;			/* if we are root, no password is
                                           required */

static gint
exec_su (gchar *exec_path, gchar *user, gchar *pwd)
{
	gchar *exec_p, *user_p;  /* command to execute, user name */
	pid_t pid;
	int t_fd;
	
	exec_p = g_strdup (exec_path);

#if 0
	if (asprintf (&exec_p, "%s&", exec_path) < 0) {
		perror ("Unable to allocate memory chunk");
		g_free (exec_p);
		return 0;
	}
#endif

	user_p = (user ? user : "root");

	if ((pwd == NULL) || (*pwd == '\0')) {
		g_free (exec_p);
		return 0;
	}

	/*
	 * Make su think we're sending the password from a terminal:
	 */

	if (((t_fd = OPEN_TTY()) < 0) || (grantpt(t_fd) < 0) || (unlockpt(t_fd) < 0)) {
		fprintf (stderr, "Unable to open a terminal\n");
		ABORT (root);
	}

	if ((pid = fork()) < 0) {
		perror ("Unable to fork a new process");
		ABORT (root);
	}

	if (pid > 0) {			/* parent process */
		int status;

		sleep(1);

		/* su(1) won't want a password if we're already root.
		 */
		if (root == 0) {
			write (t_fd, pwd, strlen(pwd));

			/* Need the \n to flush the password */
			write (t_fd, "\n", 1);
		}

		waitpid (pid, &status, 0);

		if (WIFEXITED (status) && WEXITSTATUS (status) && (WEXITSTATUS(status) < 255)) {
/*			error_box (_("Incorrect password.")); */
			g_free (exec_p);
			return -1;
		}
		else {
			memset (pwd, 0, strlen (pwd));
		}
	}
	else {				/* child process */
		struct passwd *pw;
		char *env, *home, *pts;

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

		if(((pts = ptsname(t_fd)) == NULL) || ((t_fd = open(pts, O_RDWR | O_NOCTTY)) < 0)) {
			perror ("Unable to open pseudo slave terminal");
			_exit (-1);
		}
		dup2 (t_fd, 0);

#if 0
		freopen ("/dev/null", "w", stderr);
		freopen ("/dev/null", "w", stdout);
#endif

		execlp ("su", "su", "-m", user_p, "-c", exec_p, NULL);
		_exit (0);
		
	}
	
	g_free (exec_p);

	return 0;
}

gint
su_run_with_password (gchar *exec_path, gchar *password)
{
	return exec_su (exec_path, "root", password);
}

gchar *
get_size_string (gfloat fsize)
{
	
	if (fsize < 1024.0f)
		return g_strdup_printf (ngettext ("%d byte", "%d bytes", (int)fsize), (int)fsize);
	
	fsize /= 1024.0f;
	if (fsize < 1024.0f)
		return g_strdup_printf (_("%d KB"), (int)fsize);
		
	fsize /= 1024.0f;
	if (fsize < 100.0f)
		return g_strdup_printf (_("%.1f MB"), fsize);
	else if (fsize < 1024.0f)
		return g_strdup_printf (_("%.0f MB"), fsize);
	
	fsize /= 1024.0f;
	return g_strdup_printf (_("%.1f GB"), fsize);

}


