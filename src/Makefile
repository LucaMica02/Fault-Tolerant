CC = mpicc
CFLAGS = -Wall
TARGET = main

$(TARGET): recursive_doubling.c
	$(CC) $(CFLAGS) -o $(TARGET) recursive_doubling.c

clean:
	rm -f $(TARGET)
