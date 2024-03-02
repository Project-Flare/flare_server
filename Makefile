CC = clang
CFLAGS = -target amd64-unknown-openbsd7.4 -march=native

flare_server:
	$(CC) $(CFLAGS) src/main.c src/mongoose.c -o target/flare_server