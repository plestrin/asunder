EXE := asunder
CC := clang
CFLAGS := -Wall -Wextra -O3 $(shell pkg-config --cflags gtk+-2.0 libcddb)
LDFLAGS := $(shell pkg-config --libs gtk+-2.0 libcddb) -lgthread-2.0 -pthread
OBJS := wrappers.o threads.o prefs.o main.o support.o util.o interface.o callbacks.o completion.o

.PHONY: clean

all: build $(EXE)

build:
	mkdir -p build

$(EXE): $(foreach obj,$(OBJS),build/$(obj))
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -rf build
	@ rm -f $(EXE)
