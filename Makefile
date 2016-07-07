.PHONY: all clean

BIN:=berk
OBJ:=berk.o con.o config.o error.o event.o ini.o remote.o util.o
CFLAGS:=-Wall -Werror
PREFIX:=/usr/local/bin

all: $(BIN)

.c.o:
	$(CC) -c -o $@ $(CFLAGS) $<

$(BIN): $(OBJ)
	$(CC) -o $@ $^ -lssh2

clean:
	rm -rf $(BIN)
	rm -rf $(OBJ)

install: $(BIN)
	install -m 755 $(BIN) $(PREFIX)

