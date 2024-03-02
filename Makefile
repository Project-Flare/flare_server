CC = clang
CFLAGS = -target amd64-unknown-openbsd7.4 -I/usr/local/include

flare_server:
	$(CC) $(CFLAGS) src/main.c -o target/flare_server