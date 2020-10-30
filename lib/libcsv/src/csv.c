#include "csv.h"

#include "verify_csv.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static size_t bufferInput(char **dest, size_t n, FILE *fp);

static int processCSV(CSV *csv);
static void splitCSV(CSV *csv);
static int splitRecords(CSV *csv);
static int countFields(CSV *csv);
static int shrinkCSV(CSV *csv);
static int splitQuotes(CSV *csv);


/* Generate CSV object */
CSV * createCSV(void)
{
    CSV *csv = malloc(sizeof(CSV));
    return csv;
}


/* Free CSV object */
void freeCSV(CSV *csv)
{
    if (csv)
    {
        if (csv->csv)
            free(csv->csv);
        
        free(csv);
    }
}


/* Load CSV from open file into CSV object (bool header = if header exists) */
int loadCSV(CSV *csv, FILE *fp, bool header)
{
    const size_t INPUT_BUFFER_SIZE = 4096;

    size_t sz;
    char *tmp;
    long pos = 0L;

    if (!csv || !fp)
        return -1;

    csv->header = header;

    /* Allocate CSV array (will expand with more input via bufferInput()) */
    csv->csv = malloc(INPUT_BUFFER_SIZE);

    if (!csv->csv)
        return -2;

    /* Load CSV from open file stream into the array */
    sz = bufferInput(&csv->csv, INPUT_BUFFER_SIZE, fp);

    if (!sz || sz >= LONG_MAX)
        return -2;

    csv->sz = (long) sz;

    /* Increase size by one to include null-terminator */
    if (++(csv->sz) < 2L || (unsigned long) csv->sz > SIZE_MAX)
        return -2;

    tmp = realloc(csv->csv, (size_t) csv->sz);

    if (!tmp)
        return -2;

    csv->csv = tmp;

    csv->csv[csv->sz - 1] = '\0';

    /* Ensure CSV conforms to the MIME text/csv standard (RFC 7111) */
    if (verifyCSV(csv, &pos))
        return 1;

    /* Split on commas and EOLs */
    if (processCSV(csv))
        return 2;

    return 0;
}


/* Buffer input stream */
static size_t bufferInput(char **dest, size_t n, FILE *fp)
{
    size_t used = 0;
    char *tmp;

    if (!dest || !(*dest) || n == 0)
        return 0;

    /* Keep on reading unless error or EOF */
    do
    {
        size_t readBytes = fread(*dest + used, 1, n - used, fp);

        if (ferror(fp))
            return 0;

        if (!readBytes)
            continue;

        used += readBytes;

        /* If buffer is full, realloc with twice the space */
        if (used == n)
        {
            if (n > SIZE_MAX / 2)
                return 0;

            n *= 2;
            tmp = realloc(*dest, n);

            if (!tmp)
                return 0;
            
            *dest = tmp;
        }
    }
    while (!feof(fp));

    if (!used)
        return 0;

    /* Shrink buffer down to number of bytes used */
    tmp = realloc(*dest, used);

    if (!tmp)
        return 0;

    *dest = tmp;
    
    /* Return size of buffer */
    return used;
}


/* Output the CSV */
int writeCSV(FILE *fp, CSV *csv, char delim)
{
    if (!fp || !csv || !csv->csv)
        return 1;

    if (csv->header)
        writeCSVHeader(fp, csv, delim);

    writeCSVRecords(fp, csv, delim);

    return 0;
}


/* Output the CSV header */
int writeCSVHeader(FILE *fp, CSV *csv, char delim)
{
    if (!fp || !csv || !csv->csv)
        return 1;

    for (long i = 0, pos = 0; i < csv->fields && pos < csv->sz; ++i)
    {
        char *field = csv->csv + pos;
        fputs(field, fp);

        /* While not final field, separate fields with delim */
        if (i < csv->fields - 1)
            fputc(delim, fp);

        pos += (long) strlen(field) + 1;
    }

    fputs("\r\n", fp);
    fflush(fp);

    return 0;
}


/* Output the CSV records */
int writeCSVRecords(FILE *fp, CSV *csv, char delim)
{
    long pos = 0;

    if (!fp || !csv || !csv->csv)
        return 1;

    /* Skip header row if exists */
    if (csv->header)
    {
        for (long i = 0; i < csv->fields && pos < csv->sz; ++i)
        {
            char *field = csv->csv + pos;
            pos += (long) strlen(field) + 1;
        }
    }

    for (long i = 0; i < csv->records && pos < csv->sz; ++i)
    {
        for (long j = 0; j < csv->fields && pos < csv->sz; ++j)
        {
            char *field = csv->csv + pos;
            fputs(field, fp);

            /* While not final field, separate fields with delim */
            if (j < csv->fields - 1)
                fputc(delim, fp);

            pos += (long) strlen(field) + 1;
        }

        fputs("\r\n", fp);
    }

    fflush(fp);

    return 0;
}


/* Get a value from the specified record number and field. Indexing starts at 1
 * and header row is included as a "record"
 */
char * getCSVValue(CSV *csv, long record, long field)
{
    long pos = 0;

    if (!csv || !csv->csv)
        return NULL;

    /* Range check */
    if (record < 1 || record > csv->records || field < 1 || field > csv->fields)
        return NULL;

    for (long i = 1; i < record && pos < csv->sz; ++i)
    {
        for (long j = 0; j < csv->fields && pos < csv->sz; ++j)
        {
            char *value = csv->csv + pos;
            pos += (long) strlen(value) + 1;
        }
    }

    for (long j = 1; j < field && pos < csv->sz; ++j)
    {
        char *value = csv->csv + pos;
        pos += (long) strlen(value) + 1;
    }

    return csv->csv + pos;
}


/* Process CSV array into a navigatable format */
static int processCSV(CSV *csv)
{
    if (countFields(csv))
        return 1;

    splitCSV(csv);
    shrinkCSV(csv);
    splitRecords(csv);
    splitQuotes(csv);

    return 0;
}


/* Split the CSV buffer at CRLF, and count records */
static void splitCSV(CSV *csv)
{
    bool escaped = false;

    csv->records = 0;

    for (long i = 0; i < csv->sz - 1; ++i)
    {
        char c = csv->csv[i];
        char next = csv->csv[i + 1];

        /* Edge case where the final line is not CRLF-terminated */
        if (i == csv->sz - 3 && !(c == '\r' && next == '\n'))
            ++(csv->records);

        if (c == '"')
        {
            if (escaped)
            {
                if (next == '"')
                    ++i;
                else
                    escaped = false;
            }
            else
            {
                escaped = true;
            }
        }
        else if (!escaped)
        {
            if (c == '\r' && next == '\n')
            {
                csv->csv[i] = '\0';
                csv->csv[++i] = '\0';

                /* Each unescaped CRLF = new record */
                ++(csv->records);
            }
        }
    }

    if (csv->header && csv->records > 0)
        --(csv->records);
}


/* Split the CSV buffer at unescaped commas */
static int splitRecords(CSV *csv)
{
    bool escaped = false;

    for (long i = 0; i < csv->sz - 1; ++i)
    {
        char c = csv->csv[i];

        if (c == '"')
        {
            if (escaped)
            {
                if (csv->csv[i + 1] == '"')
                    ++i;
                else
                    escaped = false;
            }
            else
            {
                escaped = true;
            }
        }
        else if (c == ',' && !escaped)
        {
            csv->csv[i] = '\0';
        }
    }

    return 0;
}


/* Remove quotes and shrink */
static int splitQuotes(CSV *csv)
{
    char *tmpptr;
    long sz = csv->sz;
    bool escaped = false;

    for (long i = 0; i < csv->sz - 1; ++i)
    {
        char c = csv->csv[i];

        if (c == '"')
        {
            if (escaped && csv->csv[i + 1] == '"')
            {
                ++i;
            }
            else
            {
                escaped = !escaped;
                memmove(csv->csv + i, csv->csv + i + 1, (size_t) (csv->sz - i - 1));
                --sz;
                --i;
            }
        }
    }

    tmpptr = realloc(csv->csv, (size_t) sz);

    if (!tmpptr)
        return 1;

    csv->csv = tmpptr;

    return 0;
}


/* Calculate number of fields in the CSV by counting the fields in the first
 * record/header
 */
static int countFields(CSV *csv)
{
    long fieldsTmp = 0;
    bool header = true, escaped = false;

    csv->fields = 0;

    for (long i = 0; i < csv->sz - 1; ++i)
    {
        char c = csv->csv[i];
        char next = csv->csv[i + 1];

        if (c == '"')
        {
            if (escaped)
            {
                if (csv->csv[i + 1] == '"')
                    ++i;
                else
                    escaped = false;
            }
            else
            {
                escaped = true;
            }
        }
        else if (!escaped)
        {
            if (c == ',')
                ++fieldsTmp;
            
            if ((c == '\r' && next == '\n'))
            {
                ++i;
                ++fieldsTmp;

                if (header)
                {
                    header = false;
                    csv->fields = fieldsTmp;
                }

                if (fieldsTmp != csv->fields)
                    return 1;

                fieldsTmp = 0;
            }
            else if (i == csv->sz - 2 && next == '\0' && ++fieldsTmp != csv->fields)
            {
                if (header)
                {
                    header = false;
                    csv->fields = fieldsTmp;
                }
                else
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}


/* Turn all '\0''\0' from old CRLFs into '\0' */
static int shrinkCSV(CSV *csv)
{
    char *tmpptr;
    long sz = csv->sz;

    for (long i = csv->sz - 2; i >= 0; --i)
    {
        char c = csv->csv[i];
        char next = csv->csv[i + 1];

        if (c == '\0' && next == '\0')
        {
            memmove(csv->csv + i, csv->csv + i + 1, (size_t) (csv->sz - i - 1));
            sz--;
        }
    }

    tmpptr = realloc(csv->csv, (size_t) sz);

    if (!tmpptr)
        return 1;
    
    csv->csv = tmpptr;
    csv->sz = sz;

    return 0;
}