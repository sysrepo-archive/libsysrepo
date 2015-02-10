INCLUDES = -I/opt/dev/libsysrepo/include/sysrepo -I/opt/dev/sysrepod
LIBRARIES = -L/opt/dev/libsysrepo/bin -L../../../../build/ -L/opt/dev/sysrepod/
DEBUG = -ggdb -O0 -v -da -Q -Wstack-protector -fstack-check
FLAGS = -Wall  -g 
all: \
	sysrepo

sysrepo:
	gcc $(FLAGS) -c  -I/opt/dev/libsysrepo/include/sysrepo if.c
	gcc $(FLAGS)  $(INCLUDES) -I/usr/include/libxml2 $(LIBRARIES) ifstatus.c -l roxml -l sysrepo -l srd -l xml2 if.o -o ifstat

clean:
	rm ifstat *.o
