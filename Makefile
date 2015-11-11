DEB_ZLIB_STATIC=/usr/lib/x86_64-linux-gnu/libz.a
DEB_LIBZIP_STATIC=/usr/lib/libzip.a

all:
	gcc java.c main.c util.c -lz -lzip -o jfind

static-deb:
	gcc java.c main.c util.c ${DEB_LIBZIP_STATIC} ${DEB_ZLIB_STATIC} -o jfind

static-cwd:
	gcc java.c main.c util.c libz.a libzip.a -o jfind

semi-static:
	gcc java.c main.c util.c -lz libzip.a -o jfind

clean:
	rm -f jfind

