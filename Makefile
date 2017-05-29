CC=clang
OPT=-O0
TEST=false
OBJS=build/test.ll build/main.ll build/vm.ll build/buffer.ll build/frame.ll build/environment.ll

all: objects
ifeq ($(TEST), true)
	clang $(OPT) $(filter-out build/main.ll, $(OBJS)) -o bin/vm
else
	clang $(OPT) $(filter-out build/test.ll, $(OBJS)) -o bin/vm
endif

objects: $(OBJS)

rebuild: clean all

build/%.ll: src/%.c
	$(CC) $(OPT) -S -emit-llvm $< -o $@
	opt $@ -O3 -S -o $@

clean:
	rm -rf bin/*
	rm -rf build/*
