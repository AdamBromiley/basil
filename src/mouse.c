#include "mouse.h"

#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>

#include <X11/Xlib.h>


/* Connect to X server */
Display * openXDisplay(void)
{
    return XOpenDisplay(NULL);
}


/* Close connection to X server */
void closeXDisplay(Display *display)
{
    if (display)
        XCloseDisplay(display);
}


int openMouseFile(const char *mouse)
{
    /* By default, the mouse uses the PS/2 protocol. It can be initialised with
     * magic byte sequences through a write command, though, for additional
     * functionality such as a scroll wheel
     */
    return open(mouse, O_RDONLY);
}


void closeMouseFile(int fd)
{
    close(fd);
}


int getDisplayDimensions(Coordinates *dimensions, Display *display)
{
    XWindowAttributes geometry;
    Window root;
    Status ret;

    if (!dimensions || !display)
        return 1;

    /* Get geometry of the root window (the entire display) */
    root = DefaultRootWindow(display);

    /* For clean code and because we are only calling this once, we will not use
     * XGetGeometry(), which returns the relevant members of the
     * XWindowsAttribute struct, but as individual variables
     */
    ret = XGetWindowAttributes(display, root, &geometry);

    if (!ret)
        return 1;

    dimensions->x = geometry.width;
    dimensions->y = geometry.height;

    return 0;
}


int readPS2Packet(uint8_t *packet, size_t n, int fd)
{
    /* PS/2 packets are 3 bytes long. The first byte has the structure:
     *
     *     Bit:   0       1       2        3      4   5    6     7  
     *         +------+-------+--------+--------+---+---+-----+-----+
     *         |      |       |        |        | Sign  | Overflow  |
     *         | Left | Right | Middle | Always +---+---+-----+-----+
     *         | Btn  | Btn   | Btn    | One    | X | Y |  X  |  Y  |
     *         +------+-------+--------+--------+---+---+-----+-----+
     * 
     * Bits 0 to 2 are for the mouse buttons (PS/2 only supports 3-button
     * mice).
     * Bit 3 should always be one, and hence can be used for a quick
     * error-check.
     * 
     * Bytes 2 and 3 are 8-bit magnitudes for the X and Y, respectively,
     * relative distance moved. Bits 4 and 5 of the first byte are their
     * respective signs (negative is left/down, positive is right/up). Bits
     * 6 and 7 of the first byte are "overflows" however, in practice, are
     * not useful and usually indicate an erroneous packet.
     */
    ssize_t ret = read(fd, packet, n);

    if (ret == -1)
    {
        if (errno == EINTR)
            return 1;
        
        return -1;
    }
    else if ((size_t) ret != n || !((packet[0] >> 3) & 1))
    {
        return 1;
    }

    return 0;
}


/* Use the X11 library to get the initial position of the mouse pointer */
int getMousePosition(Coordinates *pos, Display *display)
{
    /* The root and child windows the pointer is in */
    Window root, child;

    /* For storing the mouse pointer coordinates, relative to the specified
     * window's origin (in this case, root's). Redundant because the previous
     * two arguments to XQueryPointer()
     */
    int ptrWinX, ptrWinY;

    /* For storing the current state of the mouse pointer */
    unsigned int state;

    /* Get the root window (covers the entire display) */
    Window defaultRoot = DefaultRootWindow(display);

    /* For the specified window (defaultRoot - the root window), get the:
     *     - Root window of the pointer (&root) (expect same as defaultRoot)
     *     - Child window of the pointer (&child)
     *     - Coordinates relative to the root window's origin (&pos->x, &pos->y)
     *     - Coordinates relative to the specified window's origin
     *       (&ptrWinX, &ptrWinY) (expect same as root-relative coordinates)
     *     - The current state of the mouse pointer (as a bitmask)
     * 
     * This will fail with False if the mouse is not on the display.
     */
    if (XQueryPointer(display, defaultRoot, &root, &child, &pos->x, &pos->y, &ptrWinX, &ptrWinY, &state) == False)
        return 1;

    return 0;
}