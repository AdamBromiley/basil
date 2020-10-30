# CSV Name Picker

A utility program for drawing a random name from a CSV file.

## Features
- Uniform random number generation through a bespoke library that interfaces with `/dev/urandom`
- Full support of the `text/csv` MIME type ([RFC 7111](https://tools.ietf.org/html/rfc7111) and [RFC 4180](https://tools.ietf.org/html/rfc4180)) via a custom CSV handling library with dynamic memory management
- A cheat function that will select names beginning with a certain letter, dependant on mouse click position
- CSV input available via `stdin` - allowing for output redirection into the program - and as a filename argument

## Dependencies
* A Linux system (must have `/dev/urandom` and `/dev/input/mice`)
* The X Window System (X11) development library, installed to system:
    ```
    sudo apt install libx11-dev
    ```

## Usage
From the program's root directory, `make` compiles the `drawname` binary.

Run the program with `./drawname`. By default, without any command-line arguments, the program expects a CSV from standard input.

**Note: The provided CSV file MUST conform to standards. This means CRLF line endings and properly escaped symbols are necessary.**

### Cheating
CSV Name Picker has a way of discreetly cheating to only select those with first names beginning with a given letter of the alphabet.

To enable cheats, `-c`/`--cheat` must be used when invoking the program. At this point, the letter may be selected as an argument to the option.

Alternatively, the letter is picked by left-clicking in a specific region of the display. The display is divided into 28 invisible rectangles (26 letters plus two blank regions). Click in the region that maps to the desired letter to cheat:
```
+---+---+---+---+---+---+---+
| A | B | C | D | E | F | G |
+---+---+---+---+---+---+---+
| H | I | J | K | L | M | N |
+---+---+---+---+---+---+---+
| O | P | Q | R | S | T | U |
+---+---+---+---+---+---+---+
| V | W | X | Y | Z |   |   |
+---+---+---+---+---+---+---+
```
The two empty regions may be used to skip the cheat and have the program randomly select any name.

**Note: The cheat requires reading from `/dev/input/mice`, which may need elevated privileges to succeed. Try invoking the program with `sudo`.**

### Help

Help can be provided with `./drawname --help`.

```
Usage: ./drawname [OPTION]... [FILE]
       ./drawname --help

Draw a random name from a CSV FILE, or standard input.

With no FILE, or when FILE is -, read standard input.

Mandatory arguments to long options are mandatory for short options too.
Options:
  -c[CHAR], --cheat=CHAR   Enable cheats, optionally choosing a name beginning
                           with CHAR (else the CHAR will be selected with the
                           mouse pointer)
Miscellaneous:
            --help         Display this help message and exit

The CSV input must conform to the standard text/csv MIME type (RFC 7111). This
means CRLF line endings (with the final CRLF optional) and properly escaped
fields.

RFC 7111: <https://tools.ietf.org/html/rfc7111>
RFC 4180: <https://tools.ietf.org/html/rfc4180>

Examples:
  ./drawname
  ./drawname -c
  ./drawname -cA
  ./drawname --cheat=A

```

## Testing
A test dataset, conforming to RFC 7111, is provided in [test/test_data.csv](test/test_data.csv).