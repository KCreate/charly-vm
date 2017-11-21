CC=clang
CCFLAGS=-std=c++14 -g \
				-Wall \
				-Wextra \
				-Werror \
				-Wno-long-long \
				-Wno-variadic-macros \
				-fexceptions \
				-ferror-limit=10000 \
				-Ilibs
OPT=-O0
OBJS=build/main.o build/gc.o build/vm.o build/internals.o build/operators.o build/buffer.o

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
