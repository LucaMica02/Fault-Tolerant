CC = mpicc
CFLAGS = -Wall
TARGET = main

SRCS = rabenseifner.c errhandler.c util.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)