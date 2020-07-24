CFLAGS=-g -std=c99 # -Wall -W -Werror
OBJS=wm.o
LDFLAGS=-lxcb
all: wm
wm: ${OBJS}
	${CC} -o wm ${OBJS} ${LDFLAGS}
clean:
	rm -f wm *.o
