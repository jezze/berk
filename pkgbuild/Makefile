.PHONY: all clean install

NAME:=berk
VERSION:=0.0.1
TARPKG:=${NAME}-${VERSION}.tar.gz
PKG:=${NAME}-${VERSION}-1-any.pkg.tar.zst
DPKG:=${NAME}-debug-${VERSION}-1-any.pkg.tar.zst

all: $(PKG)

clean:
	rm -rf src pkg ${TARPKG} ${PKG} ${DPKG}

install:
	pacman -U ${PKG}

${TARPKG}:
	make -C .. dist
	cp ../${TARPKG} $@

${PKG}: ${TARPKG}
	makepkg --skipchecksum
