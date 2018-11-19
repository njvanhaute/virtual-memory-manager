#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define PAGESIZE 256
#define FRAMESIZE 256
#define NUMFRAMES 256
#define TLBSIZE 16
#define PHYSMEMSIZE (NUMFRAMES * FRAMESIZE)

typedef struct log_addr_t {
    uint8_t pg_num, pg_off;    
} LogAddr;

typedef struct phys_addr_t {
    uint8_t fr_num, fr_off;
} PhysAddr;

FILE *open_addr_file(char **argv);
uint16_t get_logical_addr(uint32_t);
LogAddr *create_log_addr(uint16_t);
uint16_t mask_addr_rep(uint32_t);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: ./mmu addr_file\n");
        return 1;
    }

    FILE *fp = open_addr_file(argv);
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, fp)) != -1) {
        uint32_t curr = atoi(line);
        uint16_t masked = mask_addr_rep(curr);
        LogAddr *newAddr = create_log_addr(masked);
    }
    
    fclose(fp);
    if (line)
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

uint16_t mask_addr_rep(uint32_t i) {
    return (uint16_t)(i);
}
