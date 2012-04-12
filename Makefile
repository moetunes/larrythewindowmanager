CFLAGS+= -Wall
LDADD+= -lX11
LDFLAGS=
EXEC=larry

PREFIX?= /usr/local
BINDIR?= $(PREFIX)/bin

CC=gcc

all: $(EXEC)

larry: larry.o
	$(CC) $(LDFLAGS) -s -Os -o $@ $+ $(LDADD)

install: all
	install -Dm 755 larry $(DESTDIR)$(BINDIR)/larry

clean:
	rm -fv larry *.o

