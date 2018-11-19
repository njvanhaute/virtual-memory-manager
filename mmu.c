#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct log_addr_t {
    uint8_t pg_num, pg_off;    
} LogAddr;

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
        printf("0x%x\n", curr);
        printf("0x%x\n", masked);
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
    uint16_t msb16 = i & 0xFFFF0000;
    msb16 >>= 8;
    uint16_t lsb16 = i & 0x0000FFFF;
    la->pg_num = (uint8_t)msb16;
    la->pg_off = (uint8_t)lsb16;
    return la;
}

uint16_t mask_addr_rep(uint32_t i) {
    return (uint16_t)(i);
}
