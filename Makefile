EXE 	:= asunder
CC		:= clang
INCLUDE := /usr/include/gtk-2.0 /usr/include/glib-2.0 /usr/lib/i386-linux-gnu/glib-2.0/include /usr/include/cairo /usr/include/pango-1.0 /usr/lib/i386-linux-gnu/gtk-2.0/include /usr/include/gdk-pixbuf-2.0 /usr/include/atk-1.0
CFLAGS	:= -Weverything -Wno-disabled-macro-expansion -O3 -std=gnu99 $(foreach include,$(INCLUDE),-isystem$(include))
LDFLAGS := -Weverything -lgtk-x11-2.0 -lgdk-x11-2.0 -lpangocairo-1.0 -latk-1.0 -lcairo -lgdk_pixbuf-2.0 -lpango-1.0 -lgobject-2.0 -lgthread-2.0 -pthread -lglib-2.0 -lcddb
OBJ 	:= wrappers.o threads.o prefs.o main.o support.o util.o interface.o callbacks.o completion.o

.PHONY: clean

all: build $(EXE)

build:
	mkdir -p build

$(EXE): $(foreach obj,$(OBJ),build/$(obj))
	$(CC) -o $@ $^ $(LDFLAGS)

build/%.o: src/%.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	@ rm -rf build
	@ rm -f $(EXE)
