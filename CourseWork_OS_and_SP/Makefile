CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Werror -pedantic -O2 -D_POSIX_C_SOURCE=200809L
CPPFLAGS := -Iinclude
LDFLAGS :=

TARGET := dispatcher
SRC := \
	src/main.c \
	src/options.c \
	src/memory_utils.c \
	src/string_list.c \
	src/file_collector.c \
	src/process_dispatcher.c

.PHONY: all clean

all: clean $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) -o $(TARGET) $(LDFLAGS)

clean:
	rm -rf \
		$(TARGET) \
		work \
		.idea \
		.vscode \
		.cache \
		.ccls-cache \
		*.o \
		*.obj \
		*.out \
		*.swp \
		*~ \
		.DS_Store \
		compile_commands.json
