CC = mpicc
CFLAGS = -Wall
TARGET = main

# Add all .c files here
SRCS = main.c recursive_doubling_v1.c recursive_doubling_v2.c recursive_doubling_v3.c errhandler.c util.c

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

# LINK ALL THE .c TOGHETER