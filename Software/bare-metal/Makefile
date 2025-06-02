# Variables
SRC ?=
OBJ ?=
M ?=

# Compiler and flags
CC = riscv64-unknown-elf-gcc
CFLAGS = -fno-common -fno-builtin-printf -specs=htif_nano.specs -march=rv64imafd -O3
OBJS = tasks.o deque.o tasks_gc.o

# Default rule
all: build

# Build the project
build:
ifeq ($(SRC),)
	@echo "Error: SRC is not set. Use 'make SRC=your_file'"
else
	@echo ">>Jessica: Compiling $(SRC).c"
	rm -f $(SRC).o $(SRC).riscv
	$(CC) $(CFLAGS) -c $(SRC).c
	$(CC) $(CFLAGS) -c tasks.c deque.c tasks_gc.c

	if [ "$(M)" != "" ]; then \
		$(CC) $(CFLAGS) -static -Wl,--allow-multiple-definition -DUSE_PUBLIC_MALLOC_WRAPPERS malloc.o $(SRC).o tasks.o tasks_gc.o deque.o -o $(SRC).riscv; \
	else \
		$(CC) $(CFLAGS) $(SRC).o tasks.o tasks_gc.o deque.o -o $(SRC).riscv; \
	fi
	@echo ">>Jessica: Generated $(SRC).riscv"
endif

# Compile individual object file
object:
ifeq ($(OBJ),)
	@echo "Error: OBJ is not set. Use 'make object OBJ=your_file'"
else
	@echo ">>Jessica: Compiling $(OBJ).c"
	rm -f $(OBJ).o
	$(CC) $(CFLAGS) -DUSE_PUBLIC_MALLOC_WRAPPERS -c $(OBJ).c
	@echo ">>Jessica: Generated $(OBJ).o"
endif

# Clean files
clean:
	@echo ">>Jessica: Cleaning generated files"
	rm -f *.o *.riscv
