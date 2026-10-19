CC ?= cc

CFLAGS = -Wall -Wextra -Wpedantic -std=c17 -Iinclude -MMD -MP

LDLIBS = -lm

TARGET = pipro

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

PREFIX ?= /usr/local
BINDIR ?= ${PREFIX}/bin
MANDIR?=${PREFIX}/man

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@ $(LDLIBS)

clean:
	rm -rf build $(TARGET)

install: pipro
	mkdir -p "${DESTDIR}${BINDIR}"
	mkdir -p ${DESTDIR}${MANDIR}/man1
	install -m 755 pipro "${DESTDIR}${BINDIR}/pipro"
	install -m 644 pipro.1 ${DESTDIR}${MANDIR}/man1

uninstall:
	rm "${DESTDIR}${BINDIR}/pipro"
	rm "${DESTDIR}${MANDIR}/man1/pipro.1"

-include $(OBJ:.o=.d)