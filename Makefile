PROG = write-trend
CFLAGS += -g -O2 -Wall
CFLAGS += -std=gnu99
# CFLAGS += -pthread
# LDLIBS += -L/usr/local/lib -lmylib
# LDLIBS += -lrt
# LDFLAGS += -pthread

all: $(PROG)
OBJS += $(PROG).o
OBJS += get_num.o
OBJS += set_timer.o
OBJS += my_signal.o
$(PROG): $(OBJS)

clean:
	rm -f *.o $(PROG)
