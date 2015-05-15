LIBXML2_INCLUDE_PATH = /usr/include/libxml2
INCLUDES = -I/opt/dev/sysrepod -I$(LIBXML2_INCLUDE_PATH)
LIBRARIES = -L/opt/dev/sysrepod/
DEBUG = -ggdb -O0 -v -da -Q -Wstack-protector -fstack-check
CC_FLAGS = -Wall -g 
CC = gcc
PREFIX = /usr

all: \
	cli


cli:	cli.c
	$(CC) $(CC_FLAGS) $(INCLUDES) $(LIBRARIES) cli.c -lsrd -lxml2 -lreadline -o sysrepo

clean:
	rm -f *.o sysrepo

install: cli
	install -d $(DESTDIR)/$(PREFIX)/bin/
	cp sysrepo $(DESTDIR)/$(PREFIX)/bin
