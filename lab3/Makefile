CC := cc
CFLAGS := -W -Wall -Wextra -std=c11 -pedantic -Wno-unused-parameter -Wno-unused-variable
TARGET := lab3
SRC := main.c parent_process.c child_process.c
OBJ := $(SRC:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)
