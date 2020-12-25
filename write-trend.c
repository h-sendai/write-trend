#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

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
    char msg[] = "Usage: write-trend filename buffer_size total_size\n"
                 "suffix m for mega, g for giga\n";
    fprintf(stderr, "%s\n", msg);

    return 0;
}

void sig_alrm(int signo)
{
    got_alrm = 1;
    return;
}

int main(int argc, char *argv[])
{
    int c;
    while ( (c = getopt(argc, argv, "d")) != -1) {
        switch (c) {
            case 'd':
                debug = 1;
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
    set_timer(1, 0, 1, 0);
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
    }
    return 0;
}
