EXE := asunder
CC := clang
CFLAGS := -Wall -Wextra -O2 -Wno-deprecated-declarations $(shell pkg-config --cflags gtk+-2.0 libcddb)
LDFLAGS := $(shell pkg-config --libs gtk+-2.0 libcddb) -lgthread-2.0 -pthread
CFLAGS_DEBUG := $(CFLAGS) -fsanitize=address -g
LDFLAGS_DEBUG := $(LDFLAGS) -fsanitize=address -g
OBJS := wrappers.o threads.o prefs.o main.o support.o util.o interface.o callbacks.o completion.o

.PHONY: clean

all: build $(EXE) build_debug $(EXE)_debug

build:
	mkdir -p build

$(EXE): $(foreach obj,$(OBJS),build/$(obj))
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(EXE)_debug: $(foreach obj,$(OBJS),build_debug/$(obj))
	$(CC) -o $@ $^ $(LDFLAGS_DEBUG)

build_debug:
	mkdir -p build_debug

build_debug/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS_DEBUG)

clean:
	@ rm -rf build
	@ rm -rf build_debug
	@ rm -f $(EXE) $(EXE)_debug
