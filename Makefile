CC=clang
CCFLAGS=-std=c++17 \
				-g \
				-Wall \
				-Wextra \
				-Werror \
				-Wno-long-long \
				-Wno-variadic-macros \
				-fexceptions \
				-ferror-limit=10000 \
				-Ilibs \
				-Isrc/headers
OPT=-O0
OBJS=build/main.o \
		 build/gc.o \
		 build/vm.o \
		 build/internals.o \
		 build/operators.o \
		 build/buffer.o \
		 build/cli.o \
		 build/symboltable.o \
		 build/managedcontext.o \
		 build/block.o

all: objects
	$(CC) $(OPT) $(OBJS) $(CCFLAGS) -lstdc++ -lm -o bin/vm

objects: $(OBJS)

build/%.o: src/%.cpp
	$(CC) $(OPT) $(CCFLAGS) -c $< -o $@

rebuild:
	make clean
	make -j

clean:
	rm -rf bin/*
	rm -rf build/*

format: all
	clang-format -i src/*.cpp src/headers/*.h -style=file

valgrind: all
	valgrind --leak-check=full --show-leak-kinds=all --show-reachable=no bin/vm todos.md
