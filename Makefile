.PHONY: all clean

BIN:=berk
OBJ:=con.o con_ssh.o berk.o error.o ini.o init.o job.o remote.o run.o
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

