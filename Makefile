# Compiler flags
CC=gcc
OPT=-O3
CFLAGS=$(OPT) -g -Wall -Wextra -pedantic -std=c11

# Source and Object Files
MAIN_OBJ=src/perft/perft.o
SRCS=src/attack_tables/attack_tables.c  \
     src/board_utils/board_utils.c      \
	 src/generator/generator.c          \
	 src/move_encoding/move_encoding.c  \
	 src/board/board.c
OBJS=$(SRCS:.c=.o)

# Output Binaries
TARGET=bin/perft
LIB_TARGET=bin/spark.a

# Rules
all: $(TARGET) $(LIB_TARGET)

$(TARGET): $(OBJS) $(MAIN_OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(MAIN_OBJ)

$(LIB_TARGET): $(OBJS)
	ar rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm  $(OBJS) $(MAIN_OBJ) $(TARGET) $(LIB_TARGET)

test:
	@bin/perft
