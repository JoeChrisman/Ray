CC = cc

CFLAGS_DEBUG = -std=c11 -g -O0 -Wall
CFLAGS_RELEASE = -std=c11 -O3 -Wall -DNDEBUG

TARGET = Ray

SRCS = \
	main.c \
	Uci.c \
	Position.c \
	Notation.c \
	Move.c \
	AttackTables.c \
	MoveGen.c \
	Search.c \
	Eval.c \
	HashTable.c \
	MoveOrder.c \
	Bitboard.c \
	Utils.c \
	Perft.c \
	SearchManager.c

OBJS = $(SRCS:.c=.o)

.PHONY: all debug release clean

all: release

debug: CFLAGS = $(CFLAGS_DEBUG)
debug: $(TARGET)

release: CFLAGS = $(CFLAGS_RELEASE)
release: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
