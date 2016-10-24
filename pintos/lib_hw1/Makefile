CC = gcc
CFLAGS = -W -Wall
TARGET = testlib
OBJECTS = bitmap.o hash.o list.o main.o

all = $(TARGET)

$(TARGET) : $(OBJECTS)
		$(CC) $(CFLAGS) -o $@ $^

clean :
		rm -f $(TARGET) $(OBJECTS)
