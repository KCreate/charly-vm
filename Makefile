CC=clang
CCFLAGS=-std=c++14 -g
OPT=-O0
OBJS=build/main.o build/scope.o build/gc.o build/vm.o build/internals.o

all: objects
	$(CC) $(OPT) $(OBJS) $(CCFLAGS) -lstdc++ -o bin/vm

objects: $(OBJS)

rebuild: clean all

build/%.o: src/%.cpp
	$(CC) $(OPT) $(CCFLAGS) -c $< -o $@

clean:
	rm -rf bin/*
	rm -rf build/*
