#define _GNU_SOURCE /* for O_DIRECT */
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
    char msg[] = "Usage: write-trend [-d] [-i interval] [-s usec] [-C] [-D] [-S] filename buffer_size total_size\n"
                 "suffix m for mega, g for giga\n"
                 "Options:\n"
                 "-d debug\n"
                 "-i interval (default: 1 seconds): print interval (may decimal such as 0.1)\n"
                 "-s usec (default: none): sleep usec micro seconds between each write\n"
                 "-C : drop page cache after all write() done\n"
                 "-D : Use direct IO (O_DIRECT)\n"
                 "-S : Use synchronized IO (O_SYNC)\n"
                 "-t : Print timestamp from Epoch\n";

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
    int usleep_usec     = 0;
    int do_drop_cache   = 0;
    int use_direct_io   = 0;
    int use_sync_io     = 0;
    int print_timestamp = 0;
    struct timeval tv_interval = { 1, 0 };

    prctl(PR_SET_TIMERSLACK, 1);

    while ( (c = getopt(argc, argv, "dhi:s:tCDS")) != -1) {
        switch (c) {
            case 'd':
                debug = 1;
                break;
            case 'h':
                usage();
                exit(0);
                break;
            case 'i':
                tv_interval = str2timeval(optarg);
                break;
            case 's':
                usleep_usec = strtol(optarg, NULL, 0);
                break;
            case 't':
                print_timestamp = 1;
                break;
            case 'C':
                do_drop_cache = 1;
                break;
            case 'D':
                use_direct_io = 1;
                break;
            case 'S':
                use_sync_io = 1;
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
    long prev_file_size    = 0;

    int open_flags = O_CREAT|O_WRONLY;
    if (use_direct_io) {
        open_flags |= O_DIRECT;
    }
    if (use_sync_io) {
        open_flags |= O_SYNC;
    }

    int fd = open(filename, open_flags, 0644);
    if (fd < 0) {
        err(1, "open");
    }

    unsigned char *buf;
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
    memset(buf, 0, bufsize);

    gettimeofday(&start, NULL);
    prev = start;
    
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
            printf("%ld.%06ld %.3f MB/s %.3f MB", 
                elapsed.tv_sec, elapsed.tv_usec,
                write_rate,
                (double) current_file_size/1024.0/1024.0);
            if (print_timestamp) {
                printf(" %ld.%06ld", now.tv_sec, now.tv_usec);
            }
            printf("\n");
            fflush(stdout);
            prev_file_size = current_file_size;
        }
        int n = write(fd, buf, bufsize);
        if (n < 0) {
            err(1, "write");
        }
        current_file_size += n;
        if (debug) {
            fprintf(stderr, "write done: %ld bytes\n", current_file_size);
        }
        if (total_size <= current_file_size) {
            break;
        }
        if (usleep_usec > 0) {
            mysleep(usleep_usec);
        }
    }

    if (do_drop_cache) {
        drop_page_cache(filename);
    }

    return 0;
}
