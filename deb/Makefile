.PHONY: all clean install

NAME:=berk
VERSION:=0.0.1
TARPKG:=${NAME}-${VERSION}.tar.gz
PKGDIR:=${NAME}-${VERSION}
PKG:=${NAME}_${VERSION}-1_amd64.deb

all: ${PKG}

clean:
	rm -rf ${TARPKG} ${PKGDIR} ${PKG} *.ddeb *.buildinfo *.changes

install:
	dpkg -i ${PKG}

${TARPKG}:
	make -C .. dist
	cp ../${TARPKG} $@

${PKGDIR}: ${TARPKG}
	tar xf $^

${PKGDIR}/debian: ${PKGDIR}
	cp -r debian $@

${PKG}: ${PKGDIR}/debian
	cd ${PKGDIR} && dpkg-buildpackage -rfakeroot -b
