CC=clang
OPT=-O0
OBJS=build/main.o build/value.o build/scope.o

all: objects
	$(CC) $(OPT) $(OBJS) -std=c++14 -lstdc++ -o bin/vm

objects: $(OBJS)

rebuild: clean all

build/%.o: src/%.cpp
	$(CC) $(OPT) -std=c++14 -c $< -o $@

clean:
	rm -rf bin/*
	rm -rf build/*
