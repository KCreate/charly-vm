# Adapted for the Charly VM
# Original: https://hiltmon.com/blog/2013/07/03/a-simple-c-plus-plus-project-structure/

# Hide the annoying Entering / Leaving directory prints
MAKEFLAGS += --no-print-directory

CC := clang
OPT := -O2
OPTPROD := -O3 -Ofast
SRCDIR := src
BUILDDIR := build
INCLUDEDIR := include
TARGET := bin/dev
PRODTARGET := bin/charly
SRCEXT := cpp
HEADEREXT := h
HEADERS := $(shell find $(INCLUDEDIR) -type f -name *.$(HEADEREXT))
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/$(basename $(notdir %)),$(SOURCES:.$(SRCEXT)=.o))
CPPSTD := c++17
CFLAGS := -std=$(CPPSTD) \
					-g \
					-Wall \
					-Wextra \
					-Werror \
					-Wno-unused-private-field \
					-Wno-trigraphs \
					-ferror-limit=100 \
					-ffast-math
CFLAGSPROD := -std=$(CPPSTD) \
							-Wall \
							-Wextra \
							-Werror \
							-Wno-unused-private-field \
							-Wno-trigraphs \
							-ferror-limit=100 \
							-flto \
							-ffast-math \
							-DNDEBUG
LFLAGS := -lm -lpthread -ldl
INC := -I libs -I $(INCLUDEDIR) -I/usr/local/opt/llvm/include/c++/v1 -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
LIB := -lstdc++
RUNTIME_PROFILER := test/main.ch --runtime-profiler

$(TARGET): $(OBJECTS)
	@echo "Linking..."
	@$(CC) $(OBJECTS) $(CFLAGS) $(OPT) $(LIB) $(LFLAGS) -o $(TARGET)
	@echo "Built executable $(TARGET)"

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(OPT) $(INC) -c -o $@ $<
	@echo "Built $@"

production:
	@echo "Building production binary $(PRODTARGET)"
	@$(CC) $(SOURCES) $(CFLAGSPROD) $(OPTPROD) $(INC) $(LIB) $(LFLAGS) -o $(PRODTARGET)

profiledproduction:
	@echo "Building profiling production binary $(PRODTARGET)"
	@$(CC) $(SOURCES) $(CFLAGSPROD) $(OPTPROD) $(INC) $(LIB) $(LFLAGS) -fprofile-instr-generate=default.profraw -o $(PRODTARGET)
	@echo "Running runtime profiler"
	@$(PRODTARGET) $(RUNTIME_PROFILER)
	@echo "Converting profile to clang format"
	@llvm-profdata merge -output=code.profdata default.profraw
	@echo "Building profile guided production binary"
	@$(CC) $(SOURCES) $(CFLAGSPROD) $(OPTPROD) $(INC) $(LIB) $(LFLAGS) -fprofile-instr-use=code.profdata -o $(PRODTARGET)
	@rm code.profdata default.profraw
	@echo "Finished profile guided production binary"

clean:
	@make clean_dev
	@make clean_prod

clean_dev:
	@echo "Cleaning dev..."
	@rm -rf $(BUILDDIR) $(TARGET)

clean_prod:
	@echo "Cleaning prod..."
	@rm -rf $(BUILDDIR) $(PRODTARGET)

rebuild:
	@make clean_dev
	@make -j

format:
	@echo "Formatting..."
	@clang-format -i $(SOURCES) $(HEADERS) -style=file

test:
	bin/dev test/main.ch

.PHONY: whole clean clean_dev clean_prod rebuild format test
