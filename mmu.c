#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cda.h"

#define PAGESIZE 256
#define NUMPAGES 256
#define FRAMESIZE 256
#define NUMFRAMES 256
#define TLBSIZE 16
#define PHYSMEMSIZE (NUMFRAMES * FRAMESIZE)
#define BS_PATH "BACKING_STORE.bin"

typedef struct log_addr_t {
    uint8_t pg_num, pg_off;    
} LogAddr;

typedef struct phys_addr_t {
    uint8_t fr_num, fr_off;
} PhysAddr;

typedef struct page_entry_t {
    uint8_t fr_num;
    bool valid;
} PageEntry;

typedef struct phys_mem_block_t {
    char **store;
} PhysMem;

FILE *open_addr_file(char **argv);
FILE *open_backing_store(void);
uint16_t get_logical_addr(uint32_t);
LogAddr *create_log_addr(uint16_t);
PageEntry *create_page_entry(uint8_t);
PageEntry **init_page_table(void);
PhysMem *init_phys_mem(void);
uint16_t mask_addr_rep(uint32_t);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: ./mmu addr_file\n");
        return 1;
    }

    FILE *addr_fp = open_addr_file(argv);
    FILE *bs_fp = open_backing_store();

    PageEntry **pageTable = init_page_table();
    PhysMem *memBlock = init_phys_mem();

    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while ((read = getline(&line, &len, addr_fp)) != -1) {
        uint32_t curr = atoi(line);
        uint16_t masked = mask_addr_rep(curr);
        LogAddr *la = create_log_addr(masked);
        PageEntry *pe = pageTable[la->pg_num];
        if (!pe->valid) { 
            long offset = la->pg_num * PAGESIZE;
            fseek(bs_fp, offset, SEEK_SET);

        }
    }
    
    fclose(addr_fp);
    fclose(bs_fp);
    free(line);

    return 0;
}

FILE *open_addr_file(char **argv) {
    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        fprintf(stderr, "Couldn't open address file.\n");
        exit(1);
    }
    return fp;
}

FILE *open_backing_store(void) {
    FILE *fp = fopen(BS_PATH, "rb");
    if (!fp) {
        fprintf(stderr, "Couldn't open backing store.\n");
        exit(1);
    }
    return fp;
}

uint16_t get_logical_addr(uint32_t i) {
    return (uint16_t)(i >> 16);
}

LogAddr *create_log_addr(uint16_t i) {
    LogAddr *la = malloc(sizeof(LogAddr));
    uint16_t msb = i >> 8; 
    uint16_t lsb = i & 0xFF;
    la->pg_num = (uint8_t)msb;
    la->pg_off = (uint8_t)lsb;
    return la;
}

PageEntry *create_page_entry(uint8_t fr_num) {
    PageEntry *pe = malloc(sizeof(PageEntry));
    pe->fr_num = fr_num;
    pe->valid = false;
    return pe;
}

PageEntry **init_page_table(void) {
    PageEntry **pt = malloc(sizeof(PageEntry *) * NUMPAGES);
    int i;
    for (i = 0; i < NUMPAGES; i++) {
        pt[i] = create_page_entry(0);
    }
    return pt;
}

PhysMem *init_phys_mem(void) {
    PhysMem *pm = malloc(sizeof(PhysMem));
    pm->store = malloc(sizeof(char *) * NUMFRAMES);
    int i;
    for (i = 0; i < NUMFRAMES; i++) {
        pm->store[i] = malloc(sizeof(char) * FRAMESIZE);
    }
    return pm;
}
uint16_t mask_addr_rep(uint32_t i) {
    return (uint16_t)(i);
}
