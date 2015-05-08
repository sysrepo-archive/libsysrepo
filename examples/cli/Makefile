LIBXML2_INCLUDE_PATH = /usr/include/libxml2
INCLUDES = /opt/dev/sysrepod -I$(LIBXML2_INCLUDE_PATH)
LIBRARIES = -L/opt/dev/libsysrepo/bin -L/opt/dev/sysrepod/
DEBUG = -ggdb -O0 -v -da -Q -Wstack-protector -fstack-check
CC_FLAGS = -Wall -g 
CC = gcc

all: \
	cli


cli:	cli.c
	$(CC) $(CC_FLAGS) -I$(INCLUDES) $(LIBRARIES) cli.c -lsrd -lxml2 -lreadline -o sysrepo

clean:
	rm -f *.o sysrepo
