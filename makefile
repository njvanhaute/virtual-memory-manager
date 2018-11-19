hostd: mmu.c 
	gcc -g mmu.c -o mmu -Wall 	

.PHONY: clean
clean:
	rm -f mmu 
