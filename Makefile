.PHONY: all clean

BIN:=berk
OBJ:=berk.o con.o con_ssh.o command.o error.o event.o ini.o remote.o
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

