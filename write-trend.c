#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/prctl.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "get_num.h"
#include "set_timer.h"
#include "my_signal.h"

int debug = 0;
volatile sig_atomic_t got_alrm = 0;

int usage()
{
    char msg[] = "Usage: write-trend [-d] [-i interval] [-s usec] filename buffer_size total_size\n"
                 "suffix m for mega, g for giga\n"
                 "Options:\n"
                 "-d debug\n"
                 "-i interval (default: 1 seconds): print interval (may decimal such as 0.1)\n"
                 "-s usec (default: none): sleep usec micro seconds between each write\n";

    fprintf(stderr, "%s\n", msg);

    return 0;
}

void sig_alrm(int signo)
{
    got_alrm = 1;
    return;
}

/*
 * anti-signal sleep
 * if received signal during nanosleep(), then try nanosleep() again in remainint time
 */
int mysleep(int usec)
{
    struct timespec ts, rem;
    ts.tv_sec = usec/1000000;
    ts.tv_nsec = (usec - ts.tv_sec*1000000)*1000;

    if (debug) {
        fprintf(stderr, "%d usec: ts.tv_sec: %ld, ts.tv_nsec: %ld\n",
            usec, ts.tv_sec, ts.tv_nsec);
    }

    rem = ts;
again:
    if (nanosleep(&rem, &rem) < 0) {
        if (errno == EINTR) {
            goto again;
        }
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int c;
    int usleep_usec = 0;
    struct timeval tv_interval = { 1, 0 };

    prctl(PR_SET_TIMERSLACK, 1);

    while ( (c = getopt(argc, argv, "di:s:")) != -1) {
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case 'i':
                tv_interval = str2timeval(optarg);
                break;
            case 's':
                usleep_usec = strtol(optarg, NULL, 0);
                break;
            default:
                break;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 3) {
        usage();
        exit(1);
    }

    char *filename  = argv[0];
    long bufsize    = get_num(argv[1]);
    long total_size = get_num(argv[2]);

    struct timeval start, now, prev, elapsed;
    long current_file_size = 0;
    long prev_file_size = 0;

    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        err(1, "fopen");
    }

    unsigned char *buf = malloc(bufsize);
    if (buf == NULL) {
        err(1, "malloc");
    }
    memset(buf, 0, bufsize);

    gettimeofday(&start, NULL);
    gettimeofday(&prev, NULL);
    
    my_signal(SIGALRM, sig_alrm);
    set_timer(tv_interval.tv_sec, tv_interval.tv_usec, tv_interval.tv_sec, tv_interval.tv_usec);
    for ( ; ; ) {
        if (got_alrm) {
            got_alrm = 0;
            gettimeofday(&now, NULL);
            timersub(&now, &start, &elapsed);
            struct timeval interval;
            timersub(&now, &prev, &interval);
            double interval_usec = interval.tv_sec + 0.000001*interval.tv_usec;
            prev = now;
            long write_bytes_in_this_period = current_file_size - prev_file_size;
            double write_rate = (double) write_bytes_in_this_period / interval_usec / 1024.0 / 1024.0;
            printf("%ld.%06ld %.3f MB/s %.3f MB\n", 
                elapsed.tv_sec, elapsed.tv_usec,
                write_rate,
                (double) current_file_size/1024.0/1024.0);
            fflush(stdout);
            prev_file_size = current_file_size;
        }
        int n = fwrite(buf, 1 /* 1 byte */, bufsize, fp);
        if (n == 0) {
            if (ferror(fp)) {
                err(1, "fwrite");
            }
        }
        current_file_size += n;
        if (total_size < current_file_size) {
            exit(0);
        }
        if (usleep_usec > 0) {
            mysleep(usleep_usec);
        }
    }
    return 0;
}
