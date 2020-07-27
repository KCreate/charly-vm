# Adapted for the Charly VM
# Original: https://hiltmon.com/blog/2013/07/03/a-simple-c-plus-plus-project-structure/

CC := clang
OPT := -O0
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
					-ferror-limit=50 \
					-ffast-math
CFLAGSPROD := -std=$(CPPSTD) \
							-Wall \
							-Wextra \
							-Werror \
							-Wno-unused-private-field \
							-Wno-trigraphs \
							-ferror-limit=1 \
							-flto \
							-ffast-math
LFLAGS := -lm
INC := -I libs -I $(INCLUDEDIR) -I/usr/local/opt/llvm/include/c++/v1 -I/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk
LIB := -lstdc++
RUNTIME_PROFILER := examples/runtime-profiler.ch

$(TARGET): $(OBJECTS)
	$(call colorecho, " Linking...", 2)
	@$(CC) $(OBJECTS) $(CFLAGS) $(OPT) $(LIB) $(LFLAGS) -o $(TARGET)
	$(call colorecho, " Built executable $(TARGET)", 2)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) $(OPT) $(INC) -c -o $@ $<
	$(call colorecho, " Built $@", 2)

production:
	$(call colorecho, " Building production binary $(PRODTARGET)", 2)
	@$(CC) $(SOURCES) $(CFLAGSPROD) $(OPTPROD) $(INC) $(LIB) $(LFLAGS) -o $(PRODTARGET)

profiledproduction:
	$(call colorecho, " Building profiling production binary $(TARGET)", 2)
	@$(CC) $(SOURCES) $(CFLAGSPROD) $(OPTPROD) $(INC) $(LIB) $(LFLAGS) -fprofile-instr-generate=default.profraw -o $(TARGET)
	$(call colorecho, " Running runtime profiler", 2)
	@$(TARGET) $(RUNTIME_PROFILER)
	$(call colorecho, " Converting profile to clang format", 2)
	@llvm-profdata merge -output=code.profdata default.profraw
	$(call colorecho, " Building profile guided production binary", 2)
	@$(CC) $(SOURCES) $(CFLAGSPROD) $(OPTPROD) $(INC) $(LIB) $(LFLAGS) -fprofile-instr-use=code.profdata -o $(TARGET)
	@rm code.profdata default.profraw
	$(call colorecho, " Finished profile guided production binary", 2)

clean:
	$(call colorecho, " Cleaning dev...", 2)
	@rm -rf $(BUILDDIR) $(TARGET)

clean_prod:
	$(call colorecho, " Cleaning prod...", 2)
	@rm -rf $(BUILDDIR) $(PRODTARGET)

rebuild:
	@make clean
	@make -j

format:
	$(call colorecho, " Formatting...", 2)
	@clang-format -i $(SOURCES) $(HEADERS) -style=file

test:
	@find test -name "*.ch" | xargs bin/dev

.PHONY: whole clean rebuild format valgrind test

# Create colored output
define colorecho
      @tput setaf $2
      @echo $1
      @tput sgr0
endef
