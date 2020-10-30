#include "cheat.h"
#include "csv.h"
#include "full_name.h"
#include "program_name.h"
#include "random.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <getopt.h>


char *programName = NULL;


static int usage(void);
static int csvNamePicker(FILE *fp, bool mode, char c);
static int getRandomName(FullName *name, CSV *csv);


int main(int argc, char **argv)
{
    const char *GETOPT_STRING = ":c::";
    const struct option LONG_OPTIONS[] =
    {
        {"cheat", optional_argument, NULL, 'c'}, /* Enable cheats */
        {"help", no_argument, NULL, 'h'},
        {NULL, 0, NULL, 0}
    };

    FILE *inputFile;
    char *inputFilePath;

    bool mode = false;
    char cheatChar = '\0';

    int opt;

    programName = argv[0];

    while ((opt = getopt_long(argc, argv, GETOPT_STRING, LONG_OPTIONS, NULL)) != -1)
    {
        switch (opt)
        {
            case 'c':
                mode = true;

                if (optarg)
                {
                    if (strlen(optarg) != 1)
                    {
                        fprintf(stderr, "%s: -%c: Failed to parse argument\n", programName, optopt);
                        fprintf(stderr, "Try \'%s --help\' for more information\n", programName);
                        return EXIT_FAILURE;
                    }

                    cheatChar = optarg[0];
                }

                break;
            case 'h':
                return usage();
            case '?':
                if (optopt == '\0' && argv[optind - 1] != NULL)
                    fprintf(stderr, "%s: Invalid option: \'%s\'\n", programName, argv[optind - 1]);
                else if (optopt != '\0')
                    fprintf(stderr, "%s: Invalid option: \'-%c\'\n", programName, optopt);
                
                fprintf(stderr, "Try \'%s --help\' for more information\n", programName);
                return EXIT_FAILURE;
            case ':':
                fprintf(stderr, "%s: -%c: Option argument required\n", programName, optopt);
                fprintf(stderr, "Try \'%s --help\' for more information\n", programName);
                return EXIT_FAILURE;
            default:
                fprintf(stderr, "%s: Unknown error when reading command-line options\n", programName);
                fprintf(stderr, "Try \'%s --help\' for more information\n", programName);
                return EXIT_FAILURE;
        }
    }

    if (optind == argc)
    {
        inputFile = stdin;
    }
    else if (optind == argc - 1)
    {
        inputFilePath = argv[optind];

        if (!strcmp(inputFilePath, "-"))
        {
            inputFile = stdin;
        }
        else
        {
            inputFile = fopen(inputFilePath, "rb");

            if (!inputFile)
            {
                fprintf(stderr, "%s: %s: %s\n", programName, inputFilePath, strerror(errno));
                return EXIT_FAILURE;
            }
        }
    }
    else
    {
        fprintf(stderr, "%s: Too many arguments supplied\n", programName);
        fprintf(stderr, "Try \'%s --help\' for more information\n", programName);
        return EXIT_FAILURE;
    }

    if (csvNamePicker(inputFile, mode, cheatChar))
    {
        fclose(inputFile);
        return EXIT_FAILURE;
    }

    fclose(inputFile);

    return EXIT_SUCCESS;
}


/* --help output */
static int usage(void)
{
    printf("Usage: %s [OPTION]... [FILE]\n", programName);
    printf("       %s --help\n\n", programName);
    printf("Draw a random name from a CSV FILE, or standard input.\n\n");
    printf("With no FILE, or when FILE is -, read standard input.\n\n");
    printf("Mandatory arguments to long options are mandatory for short options too.\n");
    printf("Options:\n");
    printf("  -c[CHAR], --cheat=CHAR   Enable cheats, optionally choosing a name beginning\n");
    printf("                           with CHAR (else the CHAR will be selected with the\n");
    printf("                           mouse pointer)\n");
    printf("Miscellaneous:\n");
    printf("            --help         Display this help message and exit\n\n");
    printf("The CSV input must conform to the standard text/csv MIME type (RFC 7111). This\n");
    printf("means CRLF line endings (with the final CRLF optional) and properly escaped\n");
    printf("fields.\n\n");
    printf("RFC 7111: <https://tools.ietf.org/html/rfc7111>\n");
    printf("RFC 4180: <https://tools.ietf.org/html/rfc4180>\n\n");
    printf("Examples:\n");
    printf("  %s\n", programName);
    printf("  %s -c\n", programName);
    printf("  %s -cA\n", programName);
    printf("  %s --cheat=A\n\n", programName);

    return EXIT_SUCCESS;
}


/* Entry to main program */
static int csvNamePicker(FILE *fp, bool mode, char c)
{
    CSV *csv;
    FullName name;

    csv = createCSV();

    if (!csv)
    {
        fprintf(stderr, "%s: CSV object could not be created\n", programName);
        return 1;
    }

    switch (loadCSV(csv, fp, false))
    {
        case 0:
            break;
        case 1:
            fprintf(stderr, "%s: CSV format incorrect (it must comply to RFC 7111)\n", programName);
            freeCSV(csv);
            return 1;
        case 2:
            fprintf(stderr, "%s: Unknown error processing CSV\n", programName);
            freeCSV(csv);
            return 1;
        case -2:
            fprintf(stderr, "%s: Unknown memory error loading CSV\n", programName);
            freeCSV(csv);
            return 1;
        default:
            fprintf(stderr, "%s: Unknown execution error loading CSV\n", programName);
            freeCSV(csv);
            return 1;
    }

    if (csv->fields != 2)
    {
        fprintf(stderr, "%s: CSV records are not firstname,surname\n", programName);
        freeCSV(csv);
        return 1;
    }

    if (mode)
    {
        switch (cheat(&name, csv, c))
        {
            case 0:
                break;
            case 1:
                /* If no names beginning with specified character*/
                if (getRandomName(&name, csv))
                {
                    freeCSV(csv);
                    return 1;
                }

                break;
            default:
                freeCSV(csv);
                return 1;
        }
    }
    else
    {
        if (getRandomName(&name, csv))
        {
            freeCSV(csv);
            return 1;
        }
    }

    printf("%s %s\n", name.firstname, name.surname);

    freeCSV(csv);

    return 0;
}


/* Standard mode (no cheats) */
static int getRandomName(FullName *name, CSV *csv)
{
    long record;

    if (getRandomLong(&record, 1, csv->records))
    {
        fprintf(stderr, "%s: Unknown error generating random number\n", programName);
        return 1;
    }
    
    name->firstname = getCSVValue(csv, record, 1);
    name->surname = getCSVValue(csv, record, 2);

    if (!name->firstname || !name->surname)
    {
        fprintf(stderr, "%s: Unknown error getting CSV record\n", programName);
        return 1;
    }

    return 0;
}