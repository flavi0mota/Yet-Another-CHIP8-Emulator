CC = gcc
CFLAGS= -std=c17 -Wall -Wextra -Werror
SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)
TARGET = chip8

.PHONY: all clean

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ `sdl2-config --cflags --libs`

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
