.SUFFIXES:
.SUFFIXES: .c .h .o .so

COMMA = ,
NULL =
SPACE = $(NULL) $(NULL)




# Output binary
_BIN = drawname
BDIR = .
BIN = $(BDIR)/$(_BIN)

# Source code
_SRC = cheat.c namedrawer.c mouse.c
SDIR = src
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

# Header files
_DEPS = cheat.h fullname.h mouse.h programname.h
HDIR = include
DEPS = $(patsubst %,$(HDIR)/%,$(_DEPS))

# Object files
_OBJS = cheat.o namedrawer.o mouse.o
ODIR = obj
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))




# Library directory
LDIR = lib

# Directories with dynamic libraries in
_LDIRS = libcsv librand
LDIRS = $(patsubst %,$(LDIR)/%,$(_LDIRS))
LIDIRS = $(patsubst %,%/include,$(LDIRS))

# Include directories
_IDIRS = include $(LIDIRS)
IDIRS = $(patsubst %,-I%,$(_IDIRS))

# Library paths
LPATHS = $(patsubst %,-L%,$(LDIRS))
# Runtime library search paths
RPATHS = $(subst $(SPACE),$(COMMA),$(patsubst %,-rpath=%,$(LDIRS)))

# Libraries to be linked with `-l`
_LDLIBS = csv rand X11
LDLIBS = $(patsubst %,-l%,$(_LDLIBS))




# Directories that need Make calls
SUBMAKE = $(LDIRS)




# Compiler name
CC = gcc

# Compiler options
CFLAGS = $(IDIRS) -g -std=c99 -pedantic \
	-Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 \
	-Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs \
	-Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=5 \
	-Wswitch-default -Wundef


# Linker name
LD = gcc

# Linker options
LDFLAGS = $(LPATHS) -Wl,$(RPATHS) $(LDLIBS)




.PHONY: all
all: $(BIN)




.PHONY: mkdirs $(SUBMAKE)
mkdirs: $(SUBMAKE)
$(SUBMAKE):
	$(MAKE) -C $@




# Compile source into object files
$(OBJS): $(ODIR)/%.o: $(SDIR)/%.c
	@ mkdir -p $(ODIR)
	$(CC) -c $< $(CFLAGS) -o $@

# Link object files into executable
$(BIN): $(OBJS) $(SUBMAKE)
	@ mkdir -p $(BDIR)
	$(LD) $(OBJS) $(LDFLAGS) -o $(BIN)




.PHONY: clean clean-all
# Remove object files and binary
clean:
	rm -f $(OBJS) $(BIN)
# Perform `make clean` on dependencies in addition
clean-all: clean
	for directory in $(SUBMAKE); do \
		$(MAKE) -C $$directory clean; \
	done