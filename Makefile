CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c17 -Iinclude -MMD -MP

LDLIBS = -lm -g -fsanitize=address

TARGET = pipro

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDLIBS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@ $(LDLIBS)

clean:
	rm -rf build $(TARGET)

-include $(OBJ:.o=.d)