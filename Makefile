# Adapted for the Charly VM
# Original: https://hiltmon.com/blog/2013/07/03/a-simple-c-plus-plus-project-structure/

CC := clang
OPT := -O0
SRCDIR := src
BUILDDIR := build
INCLUDEDIR := include
TARGET := bin/vm
SRCEXT := cpp
HEADEREXT := h
HEADERS := $(shell find $(INCLUDEDIR) -type f -name *.$(HEADEREXT))
SOURCES := $(shell find $(SRCDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(SRCDIR)/%,$(BUILDDIR)/$(basename $(notdir %)),$(SOURCES:.$(SRCEXT)=.o))
CPPSTD := c++17
CFLAGS := -std=$(CPPSTD) -g -Wall -Wextra -Werror -Wno-switch -ferror-limit=10 $(OPT)
LFLAGS := -lm
INC := -I libs -I $(INCLUDEDIR)
LIB := -lstdc++

$(TARGET): $(OBJECTS)
	$(call colorecho, " Linking...", 2)
	@$(CC) $(OBJECTS) $(CFLAGS) $(LIB) $(LFLAGS) -o $(TARGET)
	$(call colorecho, " Built executable $(TARGET)", 2)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(dir $@)
	$(call colorecho, " Building $@", 2)
	@$(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	$(call colorecho, " Cleaning...", 2)
	@rm -rf $(BUILDDIR) $(TARGET)

rebuild:
	@make clean
	@make -j

format:
	clang-format -i $(SOURCES) $(HEADERS) -style=file

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all --show-reachable=no bin/vm todos.md

.PHONY: clean rebuild format valgrind

# Create colored output
define colorecho
      @tput setaf $2
      @echo $1
      @tput sgr0
endef
