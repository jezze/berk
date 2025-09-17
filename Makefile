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
TESTSCRIPT:=${CURDIR}/${SCRIPTDIR}/test.sh

all: ${BIN}

test: ${TESTSCRIPT}

clean:
	rm -rf ${BIN} ${OBJ} ${TARDIR} ${TARPKG}

dist: ${TARPKG}

install: ${BIN}
	install -Dm 755 ${BIN} ${DESTDIR}/usr/bin/${BIN}

.c.o:
	${CC} -c -o $@ ${CFLAGS} $<

${BIN}: ${OBJ}
	${CC} -o $@ $^ -lssh2

${TARDIR}:
	mkdir -p $@
	cp -r Makefile ${SRC} ${HEADERS} ${SCRIPTDIR} $@

${TARPKG}: ${TARDIR}
	tar czf $@ $^

${TESTSCRIPT}: ${BIN}
	${TESTSCRIPT}
