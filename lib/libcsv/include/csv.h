#ifndef CSV_H
#define CSV_H


#include <stdbool.h>
#include <stdio.h>


typedef struct CSV
{
    char *csv;
    long sz;
    bool header;
    long fields, records;
} CSV;


CSV * createCSV(void);
void freeCSV(CSV *csv);
int loadCSV(CSV *csv, FILE *fp, bool header);

int writeCSV(FILE *fp, CSV *csv, char delim);
int writeCSVHeader(FILE *fp, CSV *csv, char delim);
int writeCSVRecords(FILE *fp, CSV *csv, char delim);

char * getCSVValue(CSV *csv, long record, long field);


#endif