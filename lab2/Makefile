CC := cc
CPPFLAGS := -Iinclude
CFLAGS := -W -Wall -Wextra -std=c11 -pedantic -Wno-unused-parameter -Wno-unused-variable
LDFLAGS :=

PARENT_OBJS := src/parent.o src/env_utils.o
CHILD_OBJS := src/child.o

.PHONY: all clean

all: parent child

parent: $(PARENT_OBJS)
	$(CC) $(LDFLAGS) $(PARENT_OBJS) -o $@

child: $(CHILD_OBJS)
	$(CC) $(LDFLAGS) $(CHILD_OBJS) -o $@

src/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o parent child
