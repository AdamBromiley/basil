#ifndef MOUSE_H
#define MOUSE_H


#include <stddef.h>
#include <stdint.h>

#include <X11/Xlib.h>


#define PS2_PACKET_SIZE 3


typedef struct Coordinates
{
    int x, y;
} Coordinates;


Display * openXDisplay(void);
void closeXDisplay(Display *display);

int openMouseFile(const char *mouse);
void closeMouseFile(int fd);

int getDisplayDimensions(Coordinates *dimensions, Display *display);

int readPS2Packet(uint8_t *packet, size_t n, int fd);
int getMousePosition(Coordinates *pos, Display *display);


#endif