# Compiler and Flags
CC := gcc
CFLAGS := -O3 -g -Wall -Wextra -pedantic -std=c11

# Source and Object Files

MAIN_SRC := src/main.c
MAIN_OBJ := src/main.o
SRCS := src/attack_tables/attack_tables.c src/bit_utils/bit_utils.c src/board_utils/board_utils.c src/generator/generator.c src/move_encoding/move_encoding.c src/board/board.c
OBJS := $(SRCS:.c=.o)

# Output Executable
TARGET := build/spark


LIBTARGET := build/spark.a

# Build Rule
all: $(TARGET) $(LIBTARGET)

$(TARGET): $(OBJS) $(MAIN_OBJ)
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(MAIN_OBJ)

$(LIBTARGET): $(OBJS)
	ar rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean Rule
clean:
	rm -f $(OBJS) $(MAINOBJ) $(TARGET) $(LIBTARGET)

run:
	@build/spark
