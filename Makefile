CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic -std=c17 -Iinclude -MMD -MP

TARGET = pipro

SRC = $(wildcard src/*.c)
OBJ = $(SRC:src/%.c=build/%.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf build $(TARGET)

-include $(OBJ:.o=.d)