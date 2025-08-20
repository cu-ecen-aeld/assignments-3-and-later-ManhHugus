CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror -g -O0 
TARGET = writer
SOURCES = writer.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c 
	$(CC) $(CFLAGS) -c $< -o $@

clean: 
	rm -f $(TARGET) $(OBJECTS)

.PHONY: all clean
