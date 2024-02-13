SRC_FILES:=$(wildcard src/*.c)
TARGET:=wed
BUILD_DIR=build/
CC:=gcc
CFLAGS:= -Wall -Werror -Wpedantic -std=c17 -ggdb
LIBS:= -lncurses

all: build

build: $(SRC_FILES)
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC_FILES) $(LIBS) -o $(BUILD_DIR)$(TARGET)


run: build
	$(BUILD_DIR)$(TARGET)

clean:
	rm -r ./$(BUILD_DIR)
