# We try to conform to the earliest possible standard of C in order
# to ease running on older systems.  XCB uses C99 stdint, so we use that.
CFLAGS=-g -O0 -std=c99 -Wall -W -Werror
PROG=wm
OBJS=${PROG}.o
INSTALL=install
LDFLAGS=-lxcb
installpath=${DESTDIR}${PREFIX}
bindir=${installpath}/bin
docdir=${installpath}/share/doc/${PROG}
all: ${PROG}
wm: ${OBJS}
	${CC} -o ${PROG} ${OBJS} ${LDFLAGS}
clean:
	rm -f ${PROG} *.o
install:
	${INSTALL} -d ${bindir}
	${INSTALL} -s -m 755 ${PROG} ${bindir}
