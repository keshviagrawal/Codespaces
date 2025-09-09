CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Iinclude
SRCS = src/main.c src/parser.c src/hop.c src/reveal.c src/log.c src/input_redir.c src/executor.c
OBJS = $(SRCS:.c=.o)
TARGET = shell.out

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Pattern rule to compile .c to .o
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
