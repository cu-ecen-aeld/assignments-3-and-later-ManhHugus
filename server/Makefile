# Makefile for aesdsocket server
# Supports cross-compilation via CC passed from environment (e.g., buildroot)

TARGET = aesdsocket
SRC = aesdsocket.c
OBJ = $(SRC:.c=.o)
PREFIX ?=/usr
BINDIR ?=$(PREFIX)/bin

CFLAGS ?= -Wall -Wextra -O2
LDFLAGS ?=

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean install
