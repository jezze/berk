.PHONY: all clean dist install

BIN:=berk
SRC:=berk.c config.c event.c ini.c log.c remote.c run.c ssh.c util.c
HEADERS:=config.h event.h ini.h log.h remote.h run.h ssh.h util.h
OBJ:=berk.o config.o event.o ini.o log.o remote.o run.o ssh.o util.o
CFLAGS:=-Wall -Werror -pedantic
TARNAME:=berk
TARVER:=0.0.1
TARDIR:=${TARNAME}-${TARVER}
TARPKG:=${TARDIR}.tar.gz

all: ${BIN}

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
	cp ${SRC} ${HEADERS} Makefile $@

${TARPKG}: ${TARDIR}
	tar czf $@ $^
