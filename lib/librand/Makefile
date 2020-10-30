.SUFFIXES:
.SUFFIXES: .c .h .o .so




# Output dynamic library
_DLIB = librand.so
DLDIR = .
DLIB = $(DLDIR)/$(_DLIB)

# Source code
_SRC = random.c
SDIR = src
SRC = $(patsubst %,$(SDIR)/%,$(_SRC))

# Header files
_DEPS = random.h
HDIR = include
DEPS = $(patsubst %,$(HDIR)/%,$(_DEPS))

# Object files
_OBJS = random.o
ODIR = obj
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))




# Include directories
_IDIRS = include
IDIRS = $(patsubst %,-I%,$(_IDIRS))




# Compiler name
CC = gcc

# GCC options
CFLAGS = $(IDIRS) -fPIC -g -std=c99 -pedantic \
	-Wall -Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 \
	-Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs \
	-Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=5 \
	-Wswitch-default -Wundef


# Linker name
LD = gcc

# Linker options
LDFLAGS = -shared




.PHONY: all
all: $(DLIB)




# Compile source into object files
$(OBJS): $(ODIR)/%.o: $(SDIR)/%.c $(DEPS)
	@ mkdir -p $(ODIR)
	$(CC) -c $< $(CFLAGS) -o $@

# Compile object files into dynamic library
$(DLIB): $(OBJS)
	@ mkdir -p $(DLDIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $(DLIB)




# Remove object files and dynamic library
.PHONY: clean

clean:
	rm -f $(OBJS) $(DLIB)