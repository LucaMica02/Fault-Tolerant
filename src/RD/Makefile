CC = mpicc
CFLAGS = -Wall
LDFLAGS = -lm
TARGET = main

SRCS = main.c recursive_doubling.c errhandler.c util.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)