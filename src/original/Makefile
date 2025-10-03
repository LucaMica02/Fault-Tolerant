CC     = mpicc
CFLAGS = -Wall

# Targets
all: rd raben

rd: rd.c
	$(CC) $(CFLAGS) -o rd.exe rd.c

raben: raben.c
	$(CC) $(CFLAGS) -o raben.exe raben.c

clean:
	rm -f rd.exe raben.exe
