#ifndef RANDOM_H
#define RANDOM_H


#include <stddef.h>


int getRandomBytes(void *dest, size_t n);
int getRandomULong(unsigned long int *x, unsigned long int min, unsigned long int max);
int getRandomLong(long int *x, long int min, long int max);


#endif