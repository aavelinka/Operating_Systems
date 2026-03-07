CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS = -Iinclude

SRC = src/main.c src/options.c src/walker.c src/strlist.c
OBJ = $(SRC:.c=.o)

.PHONY: all clean

all: dirwalk

dirwalk: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $@

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) dirwalk
