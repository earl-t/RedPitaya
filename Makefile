# Makefile for rtc.c

# Compiler
CC = cc

# Compiler flags
CFLAGS = -std=gnu11 -Wall

# Libraries
LIBS = -static -lm -li2c

# Target executable
TARGET = rtc

# Source files
SRCS = rtc.c

# Object files
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

# Rule to compile source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean rule
clean:
	rm -f $(OBJS) $(TARGET)
