CC = clang
CFLAGS = -target amd64-unknown-openbsd7.4 -I/usr/local/include -L/usr/local/lib -l:libwebsockets.a -lssl -lcrypto

flare_server:
	$(CC) $(CFLAGS) src/main.c -o target/flare_server