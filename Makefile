CC=clang
OPT=-O0
TEST=false
CFLAGS=-g -fdata-sections -ffunction-sections $(OPT)
OBJS=build/test.o build/main.o build/vm.o build/buffer.o build/frame.o

all: $(OBJS)
ifeq ($(TEST), true)
	$(CC) $(CFLAGS) $(filter-out build/main.o, $(OBJS)) -dead_strip -o bin/vm
else
	$(CC) $(CFLAGS) $(filter-out build/test.o, $(OBJS)) -dead_strip -o bin/vm
endif

build/%.o: src/%.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -rf bin/*
	rm -rf build/*
