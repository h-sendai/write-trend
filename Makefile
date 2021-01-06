PROG = write-trend read-trend
CFLAGS += -g -O2 -Wall
CFLAGS += -std=gnu99
# CFLAGS += -pthread
# LDLIBS += -L/usr/local/lib -lmylib
# LDLIBS += -lrt
# LDFLAGS += -pthread

all: $(PROG)

OBJS0 += write-trend.o
OBJS0 += get_num.o
OBJS0 += set_timer.o
OBJS0 += my_signal.o
write-trend: $(OBJS0)

OBJS1 += read-trend.o
OBJS1 += get_num.o
OBJS1 += set_timer.o
OBJS1 += my_signal.o
read-trend: $(OBJS1)

clean:
	rm -f *.o $(PROG)
