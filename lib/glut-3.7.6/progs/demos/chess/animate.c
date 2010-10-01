/*
 * animate.c - part of the chess demo in the glut distribution.
 *
 * (C) Henk Kok (kok@wins.uva.nl)
 *
 * This file can be freely copied, changed, redistributed, etc. as long as
 * this copyright notice stays intact.
 */

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include "chess.h"

#define MOVE_FRAC 8

int frac;
int stage;
int piece;
int piece2;
int X, Y, NX, NY, XD, YD;
GLfloat CX1, CY1;
GLfloat CX2, CY2, CZ2;

FILE *inpf = NULL;

extern int path[10][10];
extern int board[10][10];
extern int cycle[10][10], cyclem, cycle2;
extern int stunt[10][10], stuntm, stunt2;

void read_move(void)
{
    char buf[256];

    if (!inpf)
	inpf = fopen("chess.inp", "r");

    if (!inpf)
    {
	fprintf(stderr, "Could not open file chess.inp for reading.\n");
	exit(1);
    }

    if (feof(inpf))
	return;

    fgets(buf, 200, inpf);

    CX1 = NX = X = buf[0]-'a'+1;
    CY1 = NY = Y = buf[1]-'0';
    CX2 = XD = buf[2]-'a'+1;
    CY2 = YD = buf[3]-'0';

    piece = board[X][Y];
    piece2 = board[XD][YD];
    board[X][Y] = 0;
    board[XD][YD] = 0;
    solve_path(X, Y, XD, YD);
    stage = 0;
    frac = 0;
    cyclem = cycle[X][Y];
    cycle2 = cycle[XD][YD];
    stuntm = stunt[X][Y];
    stunt2 = stunt[XD][YD];
    switch(path[X][Y])
    {
        case NORTH:	NY--; break;
        case SOUTH:	NY++; break;
        case WEST:	NX--; break;
        case EAST:	NX++; break;
        case NORTHWEST:	NX--; NY--; break;
        case NORTHEAST:	NX++; NY--; break;
        case SOUTHWEST:	NX--; NY++; break;
        case SOUTHEAST:	NX++; NY++; break;
    }
    if (NY == YD && NX == XD)
	stage = 1;
}

void proceed(void)
{
    frac ++;
    if (stage == 1)
    {
	if (piece2 && frac <= MOVE_FRAC * 2)
	{
	    CZ2 = -((GLfloat) frac)/MOVE_FRAC/1.8;
	    return;
	}
	frac = 0;
	piece2 = 0;
	stage ++;
	return;
    }
    else if (stage == 3)
	return;
    else
	CZ2 = 0.0;

    if (frac >= MOVE_FRAC)
    {
	frac = 0;
	X = NX;
	Y = NY;
	if (NX == XD && NY == YD)
	{
	    board[XD][YD] = piece;
	    cycle[XD][YD] = cyclem;
	    stunt[XD][YD] = stuntm;
	    piece = 0;
	    read_move();
	    return;
	}
        switch(path[X][Y])
        {
            case NORTH:		NY--; break;
            case SOUTH:		NY++; break;
            case WEST:		NX--; break;
            case EAST:		NX++; break;
            case NORTHWEST:	NX--; NY--; break;
            case NORTHEAST:	NX++; NY--; break;
            case SOUTHWEST:	NX--; NY++; break;
            case SOUTHEAST:	NX++; NY++; break;
        }
	if (NX == XD && NY == YD)
	    stage ++;
    }
    CX1 = ((GLfloat) (X*(MOVE_FRAC-frac) + NX*frac))/ MOVE_FRAC;
    CY1 = ((GLfloat) (Y*(MOVE_FRAC-frac) + NY*frac))/ MOVE_FRAC;
}
