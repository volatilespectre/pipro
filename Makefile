CC ?= cc

CFLAGS = -Wall -Wextra -Wpedantic -std=c17 -Iinclude -MMD -MP

LDLIBS = -lm

TARGET = pipro

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

PREFIX ?= /usr/local
BINDIR ?= ${PREFIX}/bin

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
	install -m 755 pipro "${DESTDIR}${BINDIR}/pipro"

uninstall:
	rm "${DESTDIR}${BINDIR}/pipro"

-include $(OBJ:.o=.d)