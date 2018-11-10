CC = arm-linux-gnueabihf-gcc
INCLUDE_DIR=$(PWD)/../include
CFLAGS =-I$(INCLUDE_DIR) -Wall

all: monitor dense_mm fibonacci

clean:
	rm -f monitor dense_mm fibonacci

monitor: monitor.c
	$(CC) $(CFLAGS) monitor.c -o monitor

dense_mm: dense_mm.c
	$(CC) $(CFLAGS) -fopenmp dense_mm.c -o dense_mm

fibonacci: fibonacci.c
	$(CC) fibonacci.c -o fibonacci
