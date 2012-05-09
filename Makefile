PROGRAM = mantis-wm
VERSION = 0.1.0

PREFIX = $(DESTDIR)/usr
BINDIR = $(PREFIX)/bin

CC = cc
LD = $(CC)

INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lc -lX11

CPPFLAGS+= -DVERSION=\"$(VERSION)\" -DPROGRAM=\"$(PROGRAM)\"
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

SRC = main.c
OBJ = $(SRC:.c=.o)

RM = rm -vf

all: $(PROGRAM)

$(PROGRAM): $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $<

$(OBJ): $(SRC) *.c *.h
	$(CC) $(CFLAGS) -c $<

clean:
	@$(RM) $(PROGRAM) $(OBJ)

install: all
	@mkdir -p $(BINDIR)
	@cp -fv $(PROGRAM) $(BINDIR)
	@chmod 755 $(BINDIR)/$(PROGRAM)

uninstall:
	@$(RM) $(BINDIR)/$(PROGRAM)

.PHONY: all clean install uninstall
