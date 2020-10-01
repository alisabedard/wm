# We try to conform to the earliest possible standard of C in order
# to ease running on older systems.  XCB uses C99 stdint, so we use that.
CFLAGS=-g -O0 -std=c99 -Wall -W -Werror
OBJS=wm.o
LDFLAGS=-lxcb
all: wm
wm: ${OBJS}
	${CC} -o wm ${OBJS} ${LDFLAGS}
clean:
	rm -f wm *.o
