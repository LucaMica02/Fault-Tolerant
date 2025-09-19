CC = mpicc
CFLAGS = -Wall
TARGET = main

SRCS = main.c recursive_doubling.c errhandler.c util.c
#SRCS = rd.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)