.PHONY: all clean

BIN:=berk
OBJ:=con.o con_ssh.o berk.o ini.o init.o job.o remote.o version.o
CFLAGS:=-Wall -Werror

all: $(BIN)

.c.o:
	$(CC) -c -o $@ $(CFLAGS) $<

$(BIN): $(OBJ)
	$(CC) -o $@ $^ -lssh2

clean:
	rm -rf $(BIN)
	rm -rf $(OBJ)

install: $(BIN)
	install -m 755 $(BIN) /usr/bin/

