CC = clang
CFLAGS = -march=native -O3 -g -fno-omit-frame-pointer -DMG_TLS=MG_TLS_BUILTIN -L/usr/local/lib -I/usr/local/include -lssl -lcrypto -Wall

flare_server:
	$(CC) $(CFLAGS) src/main.c src/mongoose.c -o target/flare_server