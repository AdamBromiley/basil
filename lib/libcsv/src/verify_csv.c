#include "verify_csv.h"

#include "csv.h"

#include <stddef.h>


static int verifyLine(CSV *csv, long *pos);
static int verifyRecord(CSV *csv, long *pos);
static int verifyField(CSV *csv, long *pos);

static int verifyUnescapedField(CSV *csv, long *pos);
static int verifyUnescapedChar(CSV *csv, long *pos);

static int verifyEscapedField(CSV *csv, long *pos);
static int verifyEscapedString(CSV *csv, long *pos);
static int verifyEscapedChar(CSV *csv, long *pos);

static int isRegularChar(char c);
static int isSpecialChar(char c);


/* Check the accordance of the CSV to the standard.
 * NOTE: *pos must = 0 when first called.
 */
int verifyCSV(CSV *csv, long *pos)
{
    int ret;

    if (!csv || !pos)
        return 1;
    
    ret = verifyLine(csv, pos);

    if (ret == 1 || *pos >= csv->sz)
        return 1;
    else if (ret == -1)
        return 0;

    return verifyCSV(csv, pos);
}


/* Check record and EOL
 * Returns:
 *      0 = Valid
 *      1 = Invalid
 *     -1 = EOF
 */
static int verifyLine(CSV *csv, long *pos)
{
    char *eol;
    int ret = verifyRecord(csv, pos);

    if (ret || *pos >= csv->sz)
        return 1;

    eol = csv->csv + *pos;

    /* If pointer at EOF (should be null-terminator) */
    if (*pos == csv->sz - 1)
    {
        if (*eol == '\0')
            return -1;
    }
    else if (*pos == csv->sz - 2)
    {
        return 1;
    }
    else if (*eol == '\r' && *(eol + 1) == '\n')
    {
        *pos += 2;
        eol += 2;

        /* If CRLF at EOF (with null-terminator next) */
        if (*pos == csv->sz - 1)
        {
            if (*eol == '\0')
                return -1;
            else
                return 1;
        }

        /* Regular CRLF EOL */
        return 0;
    }

    return 1;
}


/* Check fields */
static int verifyRecord(CSV *csv, long *pos)
{
    int ret = verifyField(csv, pos);

    if (ret || *pos >= csv->sz)
        return 1;
    
    if (*(csv->csv + *pos) == ',' && *pos < csv->sz - 1)
    {
        ++(*pos);
        ret = verifyRecord(csv, pos);

        if (ret || *pos >= csv->sz)
            return 1;
    }

    return 0;
}


/* Check if escaped or unescaped field - returns on delimiter */
static int verifyField(CSV *csv, long *pos)
{
    int ret;

    if (*(csv->csv + *pos) == '"')
        ret = verifyEscapedField(csv, pos);
    else
        ret = verifyUnescapedField(csv, pos);

    if (ret || *pos >= csv->sz)
        return 1;
    
    return 0;
}


/* Check if all characters of unescaped field are valid */
static int verifyUnescapedField(CSV *csv, long *pos)
{
    int ret = verifyUnescapedChar(csv, pos);

    if (ret == 1 || *pos >= csv->sz)
        return 1;
    else if (ret)
        return 0;

    ret = verifyUnescapedField(csv, pos);

    if (ret || *pos >= csv->sz)
        return 1;

    return 0;
}


/* Check if unescaped character is valid
 * Returns:
 *      0 = Valid
 *      1 = Invalid
 *     -1 = End of field (comma detected)
 *     -2 = End of field and record (CR detected)
 *     -3 = End of field and file (null-terminator detected)
 */
static int verifyUnescapedChar(CSV *csv, long *pos)
{
    char c = *(csv->csv + *pos);

    if (isRegularChar(c))
        ++(*pos);
    else if (c == ',')
        return -1;
    else if (c == '\r')
        return -2;
    else if (c == '\0')
        return -3;
    else
        return 1;
    
    return 0;
}


/* Check if escaped field is quote-enclosed */
static int verifyEscapedField(CSV *csv, long *pos)
{
    int ret;

    if (*(csv->csv + *pos) != '"')
        return 1;

    if (++(*pos) >= csv->sz)
        return 1;

    ret = verifyEscapedString(csv, pos);

    if (ret || *pos >= csv->sz - 1)
        return 1;

    if (*(csv->csv + *pos) != '"')
        return 1;

    ++(*pos);
    
    return 0;
}


/* Check if all characters of escaped field are valid */
static int verifyEscapedString(CSV *csv, long *pos)
{
    int ret = verifyEscapedChar(csv, pos);

    if (ret == 1 || *pos >= csv->sz)
        return 1;
    else if (ret == -1)
        return 0;

    ret = verifyEscapedString(csv, pos);

    if (ret || *pos >= csv->sz)
        return 1;

    return 0;
}


/* Check if escaped character is valid
 * Returns:
 *      0 = Valid
 *      1 = Invalid
 *     -1 = End of escaped field (non-escaped quotation mark detected)
 */
static int verifyEscapedChar(CSV *csv, long *pos)
{
    char c = *(csv->csv + *pos);

    if (isRegularChar(c) || isSpecialChar(c))
    {
        ++(*pos);
    }
    else if (c == '"')
    {
        /* Check if escaped quote. If not, field has been terminated */
        if (*pos < csv->sz - 1 && *(csv->csv + *pos + 1) == '"')
            *pos += 2;
        else
            return -1;
    }
    else
    {
        return 1;
    }
    
    return 0;
}


/* Check if unescaped character is valid according to the CSV standard */
static int isRegularChar(char c)
{
    const char VALID_UNESCAPED_CHARS[] =
    {
        ' ', '!', '#', '$', '%', '&', '\'', '(', ')', '*', '+', '-', '.', '/',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=',
        '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
        'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
        'Z', '[', '\\', ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
        'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u',
        'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~'
    };

    size_t len = sizeof(VALID_UNESCAPED_CHARS) / sizeof(char);

    for (size_t i = 0; i < len; ++i)
    {
        if (c == VALID_UNESCAPED_CHARS[i])
            return 1;
    }

    return 0;
}


/* Check if escaped special character is valid according to the CSV standard */
static int isSpecialChar(char c)
{
    const char VALID_SPECIAL_CHARS[] = {',', '\n', '\r'};

    size_t len = sizeof(VALID_SPECIAL_CHARS) / sizeof(char);

    for (size_t i = 0; i < len; ++i)
    {
        if (c == VALID_SPECIAL_CHARS[i])
            return 1;
    }

    return 0;
}