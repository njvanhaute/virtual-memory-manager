#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#define PAGESIZE 256
#define NUMPAGES 256
#define FRAMESIZE 256
#define NUMFRAMES 128
#define TLBSIZE 16
#define BS_PATH "BACKING_STORE.bin"

typedef struct log_addr_t {
    uint8_t pg_num, pg_off;    
} LogAddr;

typedef struct page_entry_t {
    uint8_t fr_num;
    bool valid;
    unsigned long long last_used;
} PageEntry;

typedef struct phys_mem_block_t {
    char **store;
} PhysMem;

typedef struct tlb_unit_t {
    uint8_t pg_num, fr_num;
} TLBEntry;

FILE *open_addr_file(char **);
FILE *open_backing_store(void);
uint16_t get_logical_addr(uint32_t);
LogAddr *create_log_addr(uint16_t);
PageEntry *create_page_entry(uint8_t);
PageEntry **init_page_table(void);
PhysMem *init_phys_mem(void);
TLBEntry **init_tlb(void);
TLBEntry *new_tlb_entry(uint8_t, uint8_t);
int8_t query_tlb(TLBEntry **, uint8_t);
uint16_t mask_addr_rep(uint32_t);
bool should_replace(int);
int find_lru(PageEntry **);
int find_first_valid(PageEntry **);
void clear_tlb_entry(TLBEntry **, uint8_t);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: ./mmu addr_file\n");
        return 1;
    }

    FILE *addr_fp = open_addr_file(argv);
    FILE *bs_fp = open_backing_store();

    PageEntry **pageTable = init_page_table();
    PhysMem *memBlock = init_phys_mem(); 
    TLBEntry **TLB = init_tlb();

    int tlbPtr = 0;
    int framePtr = 0;
    int numTranslated = 0;
    int numPageFaults = 0;
    int tlbHits = 0;
    unsigned long long counter = 0;

    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, addr_fp)) != -1) {
        uint32_t curr = atoi(line);
        uint16_t masked = mask_addr_rep(curr);
        LogAddr *la = create_log_addr(masked);
        int8_t buffQuery = query_tlb(TLB, la->pg_num);
        uint8_t fr_num = 0;
        if (buffQuery != -1) {
            fr_num = (uint8_t)buffQuery;
            tlbHits++;
            pageTable[la->pg_num]->last_used = counter;
        } else {
            PageEntry *pe = pageTable[la->pg_num];
            if (!pe->valid) {  
                long offset = la->pg_num * PAGESIZE;
                fseek(bs_fp, offset, SEEK_SET);
                
                int memLoc = 0;
                if (should_replace(framePtr)) {
                    // Find least recently used page number
                    int lru_idx = find_lru(pageTable);
                    // Set page invalid
                    pageTable[lru_idx]->valid = false;
                    // Read into page frame previously used
                    memLoc = pageTable[lru_idx]->fr_num;
                    // If necessary, clear previous entry from TLB
                    clear_tlb_entry(TLB, lru_idx);
                } else {
                    // Read into open page
                    memLoc = framePtr;
                } 
                fread(memBlock->store[memLoc], 1, FRAMESIZE, bs_fp);
                pe->fr_num = memLoc;
                pe->valid = true;
                fr_num = pe->fr_num;
                framePtr++;
                numPageFaults++;
            } 
            pe->last_used = counter;            
            fr_num = pe->fr_num;
            TLB[tlbPtr]->pg_num = la->pg_num;
            TLB[tlbPtr]->fr_num = fr_num;
            tlbPtr = (tlbPtr + 1) % TLBSIZE;
        }  
        int physAddr = fr_num * FRAMESIZE + la->pg_off;
        int value = memBlock->store[fr_num][la->pg_off];                
        printf("Virtual address: %d Physical address: %d Value: %d\n", curr, physAddr, value); 
        numTranslated++;
        counter++;
    }

    printf("Number of Translated Addresses = %d\n", numTranslated);    
    printf("Page Faults = %d\n", numPageFaults);
    printf("Page Fault Rate = %.3lf\n", (float)(numPageFaults) / (numTranslated));
    printf("TLB Hits = %d\n", tlbHits); 
    printf("TLB Hit Rate = %.3lf\n", (float)(tlbHits) / (numTranslated));
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
    pe->last_used = 0;
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

TLBEntry **init_tlb(void) {
    TLBEntry **tlb = malloc(sizeof(TLBEntry *) * TLBSIZE);
    int i;
    for (i = 0; i < TLBSIZE; i++) {
        tlb[i] = new_tlb_entry(-1, -1); 
    }

    return tlb;
}

TLBEntry *new_tlb_entry(uint8_t pg_num, uint8_t fr_num) {
    TLBEntry *tlbe = malloc(sizeof(TLBEntry));
    tlbe->pg_num = pg_num;
    tlbe->fr_num = fr_num;
    return tlbe;
}

int8_t query_tlb(TLBEntry **TLB, uint8_t pg_num) {
    int i;
    for (i = 0; i < TLBSIZE; i++) {
        TLBEntry *curr = TLB[i];
        if (curr->pg_num == pg_num) {
            return curr->fr_num;
        
        }
    }
    return -1;
}

bool should_replace(int framePtr) {
    return framePtr < 0 || framePtr > NUMFRAMES - 1;
}

int find_lru(PageEntry **pageTable) { 
    int min_idx = find_first_valid(pageTable);
    unsigned long long min_last_used = pageTable[min_idx]->last_used;
    int i;
    for (i = 1; i < NUMPAGES; i++) {
        PageEntry *curr_page = pageTable[i];
        if (curr_page->last_used < min_last_used && curr_page->valid) {
            min_last_used = curr_page->last_used;
            min_idx = i;
        }
    }
    return min_idx;
}

int find_first_valid(PageEntry **pageTable) {
    int i;
    for (i = 0; i < NUMPAGES; i++) {
        if (pageTable[i]->valid) {
            return i;
        }
    }
    return -1;
}

void clear_tlb_entry(TLBEntry **TLB, uint8_t pg_num) {
    int i;
    for (i = 0; i < TLBSIZE; i++) {
        if (TLB[i]->pg_num == pg_num) {
            TLB[i]->pg_num = -1;
            TLB[i]->fr_num = -1;
        }
    }
}
