#ifndef GRAPHIC_H
#define GRAPHIC_H

#include <glib.h>  /* defines inline for better portability */

typedef unsigned int Uint;

typedef struct
{
  unsigned short r,v,b;
}
Color;

extern const Color BLACK;
extern const Color WHITE;
extern const Color RED;
extern const Color BLUE;
extern const Color GREEN;
extern const Color YELLOW;
extern const Color ORANGE;
extern const Color VIOLET;

#endif /*GRAPHIC_H*/
