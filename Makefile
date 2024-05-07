CC = cc
TARGET = beacon
SOURCE = *.c
DESTDIR = /usr/bin
CFLAGS ?= -Wall -g -ljansson -O2 -pedantic

all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) $(DESTDIR)
