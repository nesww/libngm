CC=gcc
CFLAGS= -Wall -Werror -Wpedantic

LINKLIBS= -ldrm
INCLUDESLIBS= -I/usr/include/libdrm

SOURCES=src
INCLUDES=include
BUILDS=build

all: clean build

clean:
	rm -rf $(BUILDS)/libngm.so

build: $(SOURCES)/ngm.c $(INCLUDES)/libngm.h
	$(CC) -fPIC $(CFLAGS) $(INCLUDESLIBS) -I$(INCLUDES) -c $(SOURCES)/* -o $(BUILDS)/libngm.o
	$(CC) -shared -o $(BUILDS)/libngm.so $(BUILDS)/libngm.o $(LINKLIBS)
	rm -f $(BUILDS)/libngm.o

install: build
	sudo cp $(BUILDS)/libngm.so /usr/local/lib
	sudo cp $(INCLUDES)/libngm.h /usr/local/include
	sudo ldconfig

uninstall:
	sudo rm -f /usr/local/lib/libngm.so
	sudo rm -f /usr/local/include/libngm.h

test: install
	$(CC) $(CFLAGS) test.c -o $(BUILDS)/test -lngm -ldrm $(INCLUDESLIBS)
