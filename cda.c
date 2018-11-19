#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "cda.h"

struct cda {
    int size, cap;
    int front; // Index indicating the front of the array. No need to store the
               // back; we can compute it with the front and the size.
    void **arr;
    void (*display)(FILE *, void *);
};

static void increaseCap(CDA *items);
static void reduceCap(CDA *items);

// Function: newCDA
// Takes in a function pointer to display the objects stored in the array.
// Returns the newly allocated CDA

CDA *newCDA(void (*d)(FILE *, void *)) {
    CDA *cda = (CDA *)malloc(sizeof(CDA));
    assert(cda != 0);
    cda->size = 0;
    cda->front = 0;
    cda->cap = 1;
    cda->arr = (void **)malloc(cda->cap * sizeof(void *));
    assert(cda->arr != 0);
    cda->display = d;
    return cda;
}

// Function: insertCDAfront
// Takes in a CDA object and a void pointer.
// Inserts the void pointer at the front of the contiguous region.

void insertCDAfront(CDA *items, void *value) {
    if (items->size == items->cap) {
        increaseCap(items);
    }
    items->front--;
    if (items->front < 0) {
        items->front += items->cap;
    }
    assert(items->front < items->cap && items->front >= 0);
    items->arr[items->front] = value;
    items->size++;
}

// Function: insertCDAback
// Takes in a CDA object and a void pointer.
// Inserts the void pointer at the back of the contiguous region.

void insertCDAback(CDA *items, void *value) {
    if (items->size == items->cap) {
        increaseCap(items);
    }
    int back = (items->front + items->size) % items->cap;
    items->arr[back] = value;
    items->size++;
}

// Function: removeCDAfront
// Takes in a CDA object
// Removes and returns the void pointer at the front of the contiguous region.

void *removeCDAfront(CDA *items) {
    assert(items->size > 0);
    if (items->cap > 1 && (items->size - 1) / (float) items->cap < 0.25) {
        reduceCap(items);
    }
    void *front = items->arr[items->front];
    items->front = (items->front + 1) % items->cap; 
    items->size--;
    return front;
}

// Function: removeCDAback
// Takes in a CDA object
// Removes and returns the void pointer at the back of the contiguous region.

void *removeCDAback(CDA *items) {
    assert(items->size > 0);
    if (items->cap > 1 && (items->size - 1) / (float) items->cap < 0.25) {
        reduceCap(items);
    }
    int b = (items->front - 1 + items->size) % items->cap;
    void *back = items->arr[b];
    items->size--;
    return back;
} 

// Function: unionCDA
// Takes in recipient and donor CDAs.
// Appends all the items in the donor array to the recipient array.
// The donor array is empty after the union operation.

void unionCDA(CDA *recipient, CDA *donor) {
    for (int i = 0; i < donor->size; i++) {
        int index = donor->front + i;
        insertCDAback(recipient, donor->arr[index % donor->cap]);
    }
    while (donor->size != 0) {
        removeCDAback(donor);
    }   
}

// Function: getCDA
// Takes in a CDA object and an index
// Returns the value at the specified index from the perspective of the user

void *getCDA(CDA *items, int index) {
    assert(index >= 0 && index < items->size);
    return items->arr[(items->front + index) % items->cap];
}

// Function: setCDA
// Takes in a CDA object, an index, and a void pointer.
// Changes the value at the specified index to be that of the void pointer.
// If a value is replaced, returns the replaced value. Otherwire, returns NULL

void *setCDA(CDA *items, int index, void *value) {
    assert(index >= -1 && index <= items->size);
    void *replaced = 0;
    if (index == items->size) {
        insertCDAback(items, value);
    }
    else if (index == -1) {
        insertCDAfront(items, value);
    }
    else {
        replaced = items->arr[(items->front + index) % items->cap];
        items->arr[(items->front + index) % items->cap] = value;
    }
    return replaced;
}

// Function: extractCDA
// Takes in a CDA object
// Returns the underlying C array shrunk to an exact fit.
// The CDA object gets a new array of capacity 1 and size 0.

void **extractCDA(CDA *items) {
    if (items->size == 0) {
        return 0;
    }
    void **arr = (void **)malloc(items->size * sizeof(void *));
    assert(arr != 0);
    for (int i = 0; i < items->size; i++) {
        arr[i] = items->arr[(items->front + i) % items->cap];
    }
    free(items->arr);
    items->size = 0;
    items->cap = 1;
    items->arr = (void **)malloc(items->cap * sizeof(void *));
    assert(items->arr != 0);
    return arr;
}

// Function: sizeCDA
// Takes in a CDA object
// Returns the size of the CDA

int sizeCDA(CDA *items) {
    return items->size;
}

// Function: visualizeCDA
// Takes in a file pointer and a CDA object
// Prints the contiguous region of the array enclosed with parentheses
//     and separated by commas, followed by the size of the unfilled region
//     enclosed in parentheses.

void visualizeCDA(FILE *fp, CDA *items) {
    displayCDA(fp, items);
    fprintf(fp, "(%d)", items->cap - items->size);
}

// Function: displayCDA
// Takes in a file pointer and a CDA object
// Similar to the visualizeCDA function, except it doesn't print the size of
//     the unfilled region.

void displayCDA(FILE *fp, CDA *items) {
    fprintf(fp, "(");
    for (int i = items->front; i < items->front + items->size; i++) {
        items->display(fp, items->arr[i % items->cap]);
        if (i < items->front + items->size - 1) {
            fprintf(fp, ",");
        }
    }
    fprintf(fp, ")");
}

// Static function: increaseCap
// Takes in a CDA object
// Double the size of the CDA array, and frees the old one
    
static void increaseCap(CDA *items) {
    int oldFront = items->front;
    int oldCap = items->cap;
    items->cap *= 2;
    void **newArr = (void **)malloc(items->cap * sizeof(void *));
    assert(newArr != 0);
    for (int i = 0; i < items->size; i++) { 
        newArr[i] = items->arr[(oldFront++) % oldCap];
    }
    free(items->arr);
    items->arr = newArr;
    items->front = 0;
}

// Static function: reduceCap
// Takes in a CDA object.
// Similar to the increaseCap function, except it halves the size of the array.

static void reduceCap(CDA *items) {
    int oldFront = items->front;
    int oldCap = items->cap;
    items->cap /= 2;
    void **newArr = (void **)malloc(items->cap * sizeof(void *));
    assert(newArr != 0);
    for (int i = 0; i < items->size; i++) {
        newArr[i] = items->arr[(oldFront++) % oldCap];
    }
    free(items->arr);
    items->arr = newArr;
    items->front = 0;
} 
