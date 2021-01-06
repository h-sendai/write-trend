#define _GNU_SOURCE
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

#include "my_signal.h"
#include "set_timer.h"
#include "get_num.h"

int debug = 0;

volatile sig_atomic_t got_alrm = 0;

void sig_alrm(int signo)
{
    got_alrm = 1;
    return;
}

int usage()
{
    char msg[] = "Usage: read-trend [-i interval] [-b bufsize] filename\n"
                 "-i interval (allow decimal) sec (default 1 second)\n"
                 "-b bufsize  buffer size (default 64kB)\n"
                 "-D          use O_DIRECT\n";
    fprintf(stderr, "%s", msg);

    return 0;
}

int drop_page_cache(char *filename)
{
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        warn("open in drop_page_cache");
        return -1;
    }
    if (fdatasync(fd) < 0) {
        warn("fdatasync in drop_page_cache");
    }

    int n = posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
    if (n != 0) {
        warnx("posix_fadvise POSIX_FADV_DONTNEED fail");
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    int c;
    struct timeval tv_interval = { 1, 0 };
    int bufsize = 64*1024;
    int use_direct_io = 0;

    while ( (c = getopt(argc, argv, "b:hi:dD")) != -1) {
        switch (c) {
            case 'b':
                bufsize = get_num(optarg);
                break;
            case 'd':
                debug = 1;
                break;
            case 'h':
                usage();
                exit(0);
            case 'i':
                tv_interval = str2timeval(optarg);
                break;
            case 'D':
                use_direct_io = 1;
                break;
            default:
                break;
        }
    }

    argc -= optind;
    argv += optind;

    if (argc != 1) {
        usage();
        exit(1);
    }

    char *filename = argv[0];
    int mode = O_RDONLY;
    if (use_direct_io) {
        mode = (mode | O_DIRECT);
    }
    int fd = open(filename, mode);
    if (fd < 0) {
        err(1, "open");
    }

    char *buf;
    if (use_direct_io) {
        buf = aligned_alloc(512, bufsize);
        if (buf == NULL) {
            errx(1, "aligned_alloc");
        }
    }
    else {
        buf = malloc(bufsize);
        if (buf == NULL) {
            err(1, "malloc");
        }
    }

    my_signal(SIGALRM, sig_alrm);
    set_timer(tv_interval.tv_sec, tv_interval.tv_usec,
        tv_interval.tv_sec, tv_interval.tv_usec);

    long total_read_size      = 0;
    long prev_total_read_size = 0;

    struct timeval start, prev;
    gettimeofday(&start, NULL);
    prev = start;
    for ( ; ; ) {
        if (got_alrm) {
            got_alrm = 0;
            long interval_read_size = total_read_size - prev_total_read_size;
            prev_total_read_size = total_read_size;
            struct timeval now, elapsed, interval;
            gettimeofday(&now, NULL);
            timersub(&now, &start, &elapsed);
            timersub(&now, &prev, &interval);
            double interval_usec = interval.tv_sec + 0.000001*interval.tv_usec;
            double read_rate = interval_read_size / interval_usec / 1024.0 / 1024.0;
            printf("%ld.%06ld %.3f MB/s %.3f MB\n",
                elapsed.tv_sec, elapsed.tv_usec, read_rate, total_read_size / 1024.0 / 1024.0);
            fflush(stdout);
            prev = now;
        }

        int n = read(fd, buf, bufsize);
        if (n < 0) {
            err(1, "read");
        }
        if (n == 0) {
            break;
        }
        total_read_size += n;
    }

    close(fd);

    drop_page_cache(filename);
    return 0;
}
