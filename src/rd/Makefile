CC = mpicc
CFLAGS = -Wall 
TARGET = main

SRCS = recursive_doubling.c errhandler.c util.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) -lm

clean:
	rm -f $(TARGET)