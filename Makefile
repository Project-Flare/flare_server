CC = clang
CFLAGS = -target amd64-unknown-openbsd7.4

flare_server:
	cc src/main.c -o target/flare_server