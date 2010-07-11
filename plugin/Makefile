VERSION=unknown
STAGING_DIR=/usr/local

CC=arm-none-linux-gnueabi-gcc

CFLAGS := -g -DVERSION=\"${VERSION}\" -I${STAGING_DIR}/include/glib-2.0 -I${STAGING_DIR}/lib/glib-2.0/include -I${STAGING_DIR}/include/gstreamer-0.10 -I${STAGING_DIR}/include/libxml2 -I${STAGING_DIR}/include/SDL -I${STAGING_DIR}/include -I/usr/include
LDFLAGS  := -g -L${STAGING_DIR}/lib --rpath-link --noinhibit-exec -Wl,--allow-shlib-undefined -llunaservice -lmjson -lglib-2.0 -lgmodule-2.0 -lgthread-2.0 -lgobject-2.0 -lgstreamer-0.10 -lz -lxml2 -lcjson -ldbus-1 -lgdbus -lpdl -lSDL

zcorder: zcorder.o lunaservice.o main.o misc.o

clobber:
		rm -rf *.o zcorder
