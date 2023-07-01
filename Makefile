# Compiler and Flags
CC := gcc
CFLAGS := -O3 -g -Wall -Wextra -pedantic -std=c11

# Source and Object Files
SRCS := src/main.c src/attack_tables/attack_tables.c src/bit_utils/bit_utils.c src/board_utils/board_utils.c src/generator/generator.c src/move_encoding/move_encoding.c
OBJS := $(SRCS:.c=.o)

# Output Executable
TARGET := build/spark

# Build Rule
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean Rule
clean:
	rm -f $(OBJS) $(TARGET)

run:
	@build/spark
