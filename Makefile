CC = gcc
CFLAGS = -g -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/sysprof-6
LDFLAGS = -pthread -lgio-2.0 -lgobject-2.0 -lglib-2.0 -lcurl
TARGET = build/main
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
DEPS=$(wildcard *.h)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $<;

$(TARGET): $(OBJECTS)
	$(CC) $^ -o $@ $(LDFLAGS);

.PHONY: all clean

clean:
	rm -f *.o
