/*
 * pathplan.c - part of the chess demo in the glut distribution.
 *
 * (C) Henk Kok (kok@wins.uva.nl)
 *
 * This file can be freely copied, changed, redistributed, etc. as long as
 * this copyright notice stays intact.
 */

#include "chess.h"

extern int board[10][10];

int path[10][10];
int hops[10][10];

int steps;
int cur_hops;

void init_board(void)
{
    int i,j;
    for (i=0;i<10;i++)
    {
	for(j=0;j<10;j++)
	{
	    hops[i][j] = 0;
	    path[i][j] = (board[i][j]?-1:0);
	}
    }
}

void test_exit(int i, int j, int dir)
{
    if (i<0 || i>9 || j<0 || j>9)
	return;
    if (path[i][j])
	return;
    steps ++;
    path[i][j] = dir;
    hops[i][j] = cur_hops + 1;
}

int solve_path(int x1, int y1, int x2, int y2)
{
    int i,j;
    init_board();
    path[x2][y2] = 9;
    hops[x2][y2] = 1;
    path[x1][y1] = 0;
    cur_hops = 1;
    for (;;)
    {
	steps = 0;
	for (i=0;i<10;i++)
	{
	    for (j=0;j<10;j++)
	    {
		if (hops[i][j] != cur_hops)
		    continue;
		test_exit(i, j-1, SOUTH);
		test_exit(i, j+1, NORTH);
		test_exit(i-1, j, EAST);
		test_exit(i+1, j, WEST);
	    }
	}
	for (i=0;i<10;i++)
	{
	    for (j=0;j<10;j++)
	    {
		if (hops[i][j] != cur_hops)
		    continue;
		test_exit(i-1, j-1, SOUTHEAST);
		test_exit(i+1, j-1, SOUTHWEST);
		test_exit(i-1, j+1, NORTHEAST);
		test_exit(i+1, j+1, NORTHWEST);
	    }
	}
	cur_hops++;
	if (path[x1][y1])
	    return 1;
	if (steps == 0)
	    return 0;
    }
}
