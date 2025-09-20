.PHONY: all clean dist install test

BIN:=berk
SRC:=args.c berk.c config.c event.c ini.c log.c remote.c run.c util.c
HEADERS:=args.h config.h event.h ini.h log.h remote.h run.h util.h
OBJ:=args.o berk.o config.o event.o ini.o log.o remote.o run.o util.o
CFLAGS:=-Wall -Werror -pedantic
TARNAME:=berk
TARVER:=0.0.1
TARDIR:=${TARNAME}-${TARVER}
TARPKG:=${TARDIR}.tar.gz
SCRIPTDIR:=script
TESTDIR:=testdir
TESTSCRIPT:=${SCRIPTDIR}/test.sh
MAN:=berk.1
MANGZ:=berk.1.gz

all: ${BIN} ${MANGZ}

test: ${TESTDIR}

clean:
	rm -rf ${BIN} ${OBJ} ${MANGZ} ${TARDIR} ${TARPKG} ${TESTDIR}

dist: ${TARPKG}

install: ${BIN}
	install -Dm 755 ${BIN} ${DESTDIR}/usr/bin/${BIN}
	install -Dm 644 ${MANGZ} ${DESTDIR}/usr/share/man//man1/${MANGZ}

.c.o:
	${CC} -c -o $@ ${CFLAGS} $<

${BIN}: ${OBJ}
	${CC} -o $@ $^ -lssh2

${MANGZ}: ${MAN}
	gzip -c $^ > $@

${TARDIR}:
	mkdir -p $@
	cp -r Makefile ${SRC} ${HEADERS} ${MAN} ${SCRIPTDIR} $@

${TARPKG}: ${TARDIR}
	tar czf $@ $^

${TESTDIR}: ${BIN}
	mkdir -p $@
	${TESTSCRIPT} ${CURDIR}/${BIN} $@
