PROGRAM = mantis-wm
VERSION = 0.1.0

PREFIX?= $(DESTDIR)/usr
BINDIR?= $(PREFIX)/bin
ETCDIR?= $(DESTDIR)/etc
RCDIR  = $(ETCDIR)/$(PROGRAM)

SRC = main.c
OBJ = $(SRC:.c=.o)
RCFILE = mantisrc

CC = cc
LD = $(CC)

INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lc -lX11

CPPFLAGS+= -DVERSION=\"$(VERSION)\" -DPROGRAM=\"$(PROGRAM)\"
CPPFLAGS+= -DRCSAMPLE=\"$(RCDIR)/$(RCFILE)\"
CFLAGS   = -std=c99 -W -Wall -Wextra -Wshadow -pedantic $(CPPFLAGS) $(INCS)
LDFLAGS  = $(LIBS)

DEBUG = FALSE
ifeq ($(DEBUG), TRUE)
	CFLAGS += -g
	LDFLAGS+= -g
else
	CFLAGS += -Os
	LDFLAGS+= -s
endif

RM = rm -vf

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $<

$(OBJ): $(SRC) *.c *.h
	$(CC) $(CFLAGS) -c $<

clean:
	@$(RM) $(PROGRAM) $(OBJ)

install: all
	@mkdir -p $(RCDIR)
	@cp -vn $(RCFILE) $(RCDIR)
	@mkdir -p $(BINDIR)
	@cp -vf $(PROGRAM) $(BINDIR)
	@chmod 755 $(BINDIR)/$(PROGRAM)

uninstall:
	@$(RM) $(BINDIR)/$(PROGRAM)

.PHONY: all clean install uninstall
