# We try to conform to the earliest possible standard of C in order
# to ease running on older systems.  XCB uses C99 stdint, so we use that.
# For ease of debugging, use:
CFLAGS=-g -O0 -std=c99 -Wall -W -Werror
# For minimal binary size, use:
#CFLAGS=-Os -std=c99
PROG=wm
OBJS=${PROG}.o
INSTALL=install
LDFLAGS=-lxcb
DESTDIR=/usr/local
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
