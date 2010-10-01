/*
 * chess.h - part of the chess demo in the glut distribution.
 *
 * (C) Henk Kok (kok@wins.uva.nl)
 *
 * This file can be freely copied, changed, redistributed, etc. as long as
 * this copyright notice stays intact.
 */

#define PION    1
#define TOREN   2
#define PAARD   3
#define LOPER   4
#define KONING  5
#define DAME    6

#define NORTH           1
#define SOUTH           2
#define EAST            3
#define WEST            4
#define NORTHWEST       5
#define NORTHEAST       6
#define SOUTHWEST       7
#define SOUTHEAST       8

#define ACC 8
#define TXSX 128
#define TXSY 128

extern void GenerateTextures(void);
extern void read_move(void);
extern int solve_path(int x1, int y1, int x2, int y2);
extern void proceed(void);
extern void init(void);
extern void do_display(void);
extern void init_lists(void);
