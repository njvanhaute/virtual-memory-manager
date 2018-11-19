hostd: fifo.c lru.c 
	gcc -g fifo.c -o fifo -Wall
	gcc -g lru.c -o lru -Wall

.PHONY: clean
clean:
	rm -f fifo lru 
