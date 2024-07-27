# Compiler flags
CC=gcc
OPT=-O3
CFLAGS=$(OPT) -g -Wall -Wextra -pedantic -std=c11

# Source and Object Files
MAINOBJ=src/perft/perft.o
SRCS=src/attack_tables/attack_tables.c  \
     src/board_utils/board_utils.c      \
	 src/generator/generator.c          \
	 src/move_encoding/move_encoding.c  \
	 src/board/board.c
OBJS=$(SRCS:.c=.o)

# Output Binaries
TARGET=bin/perft
LIBTARGET=bin/spark.a

# Rules
all: $(TARGET) $(LIBTARGET)

$(TARGET): $(OBJS) $(MAINOBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(MAINOBJ)

$(LIBTARGET): $(OBJS)
	ar rcs $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm  $(OBJS) $(MAINOBJ) $(TARGET) $(LIBTARGET)

test:
	@bin/perft
