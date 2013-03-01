CFLAGS+= -g -std=c99 -pedantic -Wall -march=i686 -mtune=generic -O2 -pipe -fstack-protector --param=ssp-buffer-size=4 -D_FORTIFY_SOURCE=2
LDADD+= -lX11
LDFLAGS=
EXEC=larrythewindowmanager

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin

CC=gcc

SRC = larry.c
OBJ = ${SRC:.c=.o}

all: $(EXEC)
${EXEC}: ${OBJ}

	$(CC) $(LDFLAGS) -s -ffast-math -fno-unit-at-a-time -o $@ ${OBJ} $(LDADD)

install: all
	install -Dm 755 larrythewindowmanager $(DESTDIR)$(BINDIR)/larrythewindowmanager

clean:
	rm -fv larrythewindowmanager *.o

