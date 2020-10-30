#include "cheat.h"

#include "csv.h"
#include "full_name.h"
#include "mouse.h"
#include "program_name.h"
#include "random.h"

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>


static int getRandomNameCheated(FullName *name, char c, CSV *csv);

static int getMouseClick(Coordinates *dimensions, Coordinates *position);
static char positionToCharacter(Coordinates *dimensions, Coordinates *position);

static long countCSVByChar(CSV *csv, char c);
static char * getCSVRecordByChar(CSV *csv, long record, char c);


/* Use specified character or one from mouse click to generate random name */
int cheat(FullName *name, CSV *csv, char c)
{
    Coordinates dimensions, position;

    if (c == '\0')
    {
        /* Capture position of mouse click */
        if (getMouseClick(&dimensions, &position))
            return -1;

        /* Map mouse position to a character */
        c = positionToCharacter(&dimensions, &position);

        if (c == '\0')
            return 1;
    }
    
    /* Get a random name beginning with the character */
    switch (getRandomNameCheated(name, c, csv))
    {
        case 0:
            return 0;
        case 1:
            return 1;
        default:
            return -1;
    }
}


/* Capture the pointer position when a left-click occurs */
static int getMouseClick(Coordinates *dimensions, Coordinates *position)
{
    const char *MOUSE_INPUT_FILE = "/dev/input/mice";
    
    int fd;
    Display *display = openXDisplay();
    
    if (!display)
    {
        fprintf(stderr, "%s: Could not connect to X server\n", programName);
        return 1;
    }

    fd = openMouseFile(MOUSE_INPUT_FILE);

    if (fd == -1)
    {
        fprintf(stderr, "%s: %s: %s\n", programName, MOUSE_INPUT_FILE, strerror(errno));
        closeXDisplay(display);
        return 1;
    }

    if (getDisplayDimensions(dimensions, display))
    {
        fprintf(stderr, "%s: Could not get X window attributes\n", programName);
        closeXDisplay(display);
        openMouseFile(MOUSE_INPUT_FILE);
        return 1;
    }

    while (1)
    {
        uint8_t ps2Packet[PS2_PACKET_SIZE];

        switch (readPS2Packet(ps2Packet, sizeof(ps2Packet), fd))
        {
            case 0: /* Success */
                break;
            case 1: /* Erroneous packet or interrupt */
                continue;
            default: /* Fatal error */
                fprintf(stderr, "%s: %s\n", programName, strerror(errno));
                closeXDisplay(display);
                openMouseFile(MOUSE_INPUT_FILE);
                return 1;
        }

        /* If left-click is depressed */
        if ((ps2Packet[0] >> 0) & 1)
        {
            /* Check for unclick (also flushes the stream) */
            switch (readPS2Packet(ps2Packet, sizeof(ps2Packet), fd))
            {
                case 0: /* Success */
                    break;
                case 1: /* Erroneous packet or interrupt */
                    continue;
                default: /* Fatal error */
                    fprintf(stderr, "%s: %s\n", programName, strerror(errno));
                    closeXDisplay(display);
                    openMouseFile(MOUSE_INPUT_FILE);
                    return 1;
            }

            if (!((ps2Packet[0] >> 0) & 1))
            {
                if (!getMousePosition(position, display))
                    break;
            }
        }
    }

    closeXDisplay(display);
    closeMouseFile(fd);

    return 0;
}


/* Map pointer position to a letter in the alphabet */
static char positionToCharacter(Coordinates *dimensions, Coordinates *position)
{
    const char ALPHABET_GRID[4][7] =
    {
        {'A', 'B', 'C', 'D', 'E', 'F', 'G'},
        {'H', 'I', 'J', 'K', 'L', 'M', 'N'},
        {'O', 'P', 'Q', 'R', 'S', 'T', 'U'},
        {'V', 'W', 'X', 'Y', 'Z', '\0', '\0'}
    };

    const int ALPHABET_GRID_COLUMNS = (long) (sizeof(ALPHABET_GRID[0])/ sizeof(char));
    const int ALPHABET_GRID_ROWS = (long) (sizeof(ALPHABET_GRID) / (size_t) ALPHABET_GRID_COLUMNS);

    /* We divide the screen into a grid of 7 columns and 4 rows. This yields 28
     * cells, each of which has a letter mapped to it (26 was not chosen because
     * that would require a 13 by 2 grid, which does mean there are two blank
     * cells).
     * 
     * The grid will hence look like so:
     *     +---+---+---+---+---+---+---+
     *     | A | B | C | D | E | F | G |
     *     +---+---+---+---+---+---+---+
     *     | H | I | J | K | L | M | N |
     *     +---+---+---+---+---+---+---+
     *     | O | P | Q | R | S | T | U |
     *     +---+---+---+---+---+---+---+
     *     | V | W | X | Y | Z |   |   |
     *     +---+---+---+---+---+---+---+
     *
     * The final two blank cells return no letter.
     * 
     * *position will map to a cell, and the respective character is returned
     */
    Coordinates cellSize =
    {
        .x = dimensions->x / ALPHABET_GRID_COLUMNS,
        .y = dimensions->y / ALPHABET_GRID_ROWS
    };

    int column = position->x / cellSize.x;
    int row = position->y / cellSize.y;

    /* The screen dimensions will probably not be divisible by 7 or 4, so the
     * final column and row will have slightly a greater width and height than
     * the other cells
     */
    if (column >= ALPHABET_GRID_COLUMNS)
        column = ALPHABET_GRID_COLUMNS - 1;
    else if (column < 0)
        column = 0;

    if (row >= ALPHABET_GRID_ROWS)
        row = ALPHABET_GRID_ROWS - 1;
    else if (row < 0)
        row = 0;

    return ALPHABET_GRID[row][column];
}


/* Select a random name beginning with the specified character */
static int getRandomNameCheated(FullName *name, char c, CSV *csv)
{
    long record;
    long nameCount = countCSVByChar(csv, c);

    if (nameCount < 1)
        return 1;

    if (getRandomLong(&record, 1, nameCount))
    {
        fprintf(stderr, "%s: Unknown error generating random number\n", programName);
        return -1;
    }

    name->firstname = getCSVRecordByChar(csv, record, c);

    if (!name->firstname)
    {
        fprintf(stderr, "%s: Unknown error getting CSV record\n", programName);
        return -1;
    }

    name->surname = name->firstname + strlen(name->firstname) + 1;

    return 0;
}


/* Count number of CSV records beginning with a specified character */
static long countCSVByChar(CSV *csv, char c)
{
    long count = 0, pos = 0;

    while (pos < csv->sz)
    {
        char *record = csv->csv + pos;

        /* If first name begins with the character */
        if (toupper(*record) == toupper(c))
            ++count;

        /* Skip to start of next record */
        for (long i = 0; i < csv->fields && pos < csv->sz; ++i)
        {
            char *field = csv->csv + pos;
            pos += (long) strlen(field) + 1;
        }
    }

    return count;
}


/* Get a CSV record beginning with a specified character */
static char * getCSVRecordByChar(CSV *csv, long record, char c)
{
    char *line;
    long count = 0, pos = 0;

    while (count < record)
    {
        line = csv->csv + pos;

        if (pos >= csv->sz)
            return NULL;

        /* If first name begins with the character */
        if (toupper(*line) == toupper(c))
        {
            if (++count == record)
                break;
        }

        /* Skip to start of next record */
        for (long i = 0; i < csv->fields && pos < csv->sz; ++i)
        {
            char *field = csv->csv + pos;
            pos += (long) strlen(field) + 1;
        }
    }

    return line;
}