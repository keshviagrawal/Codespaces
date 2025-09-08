# CC = gcc
# CFLAGS = -Wall -Wextra -std=c99
# SRCS = src/main.c src/parser.c src/hop.c src/reveal.c src/log.c src/input_redir.c 
# TARGET = shell.out

# # Default target: just build
# all: $(TARGET)

# $(TARGET): $(OBJS)
# 	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# # Generic rule: how to make .o from .c
# %.o: %.c
# 	$(CC) $(CFLAGS) -c $< -o $@

# clean:
# 	rm -f $(TARGET) $(OBJS)


CC = gcc
CFLAGS = -Wall -Wextra -std=c99
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
