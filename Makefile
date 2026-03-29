CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -lsqlite3
TARGET = file_server

SRCS = src/server.c src/db.c src/auth.c src/file_handler.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) -lsqlite3

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean