CC = clang
CFLAGS = -march=native -O3 -g -fno-omit-frame-pointer -DMG_TLS=MG_TLS_OPENSSL -lssl -lcrypto

flare_server:
	$(CC) $(CFLAGS) src/main.c src/mongoose.c -o target/flare_server