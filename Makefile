CFLAGS+= -g -std=c99 -pedantic -Wall
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

	$(CC) $(LDFLAGS) -s -o $@ ${OBJ} $(LDADD)

install: all
	install -Dm 755 larrythewindowmanager $(DESTDIR)$(BINDIR)/larrythewindowmanager

clean:
	rm -fv larrythewindowmanager *.o

