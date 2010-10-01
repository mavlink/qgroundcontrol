/*
 *	matrix -
 *		Some utilities for working with matricies.
 *
 *				Paul Haeberli - 1985
 */
#include <stdio.h>

void
invertmat(float from[4][4], float to[4][4])
{
    float wtemp[4][8];
    float m0,m1,m2,m3,s;
    float *r0,*r1,*r2,*r3, *rtemp;

    r0 = wtemp[0];
    r1 = wtemp[1];
    r2 = wtemp[2];
    r3 = wtemp[3];
    r0[0] = from[0][0];		/* build up [A][I]	*/
    r0[1] = from[0][1];
    r0[2] = from[0][2];
    r0[3] = from[0][3];
    r0[4] = 1.0;
    r0[5] = 0.0;
    r0[6] = 0.0;
    r0[7] = 0.0;
    r1[0] = from[1][0];
    r1[1] = from[1][1];
    r1[2] = from[1][2];
    r1[3] = from[1][3];
    r1[4] = 0.0;
    r1[5] = 1.0;
    r1[6] = 0.0;
    r1[7] = 0.0;
    r2[0] = from[2][0];
    r2[1] = from[2][1];
    r2[2] = from[2][2];
    r2[3] = from[2][3];
    r2[4] = 0.0;
    r2[5] = 0.0;
    r2[6] = 1.0;
    r2[7] = 0.0;
    r3[0] = from[3][0];
    r3[1] = from[3][1];
    r3[2] = from[3][2];
    r3[3] = from[3][3];
    r3[4] = 0.0;
    r3[5] = 0.0;
    r3[6] = 0.0;
    r3[7] = 1.0;

    if (r0[0] == 0.0) {		/* swap rows if needed		*/
	if (r1[0] == 0.0) {
	    if (r2[0] == 0.0) {
		if (r3[0] == 0.0) goto singular;
		rtemp = r0; r0 = r3; r3 = rtemp;
	    }
	    else {rtemp = r0; r0 = r2; r2 = rtemp;}
	}
	else {rtemp = r0; r0 = r1; r1 = rtemp;}
    }
    m1 = r1[0]/r0[0];		/* eliminate first variable	*/
    m2 = r2[0]/r0[0];
    m3 = r3[0]/r0[0];
    s = r0[1];
    r1[1] = r1[1] - m1 * s;
    r2[1] = r2[1] - m2 * s;
    r3[1] = r3[1] - m3 * s;
    s = r0[2];
    r1[2] = r1[2] - m1 * s;
    r2[2] = r2[2] - m2 * s;
    r3[2] = r3[2] - m3 * s;
    s = r0[3];
    r1[3] = r1[3] - m1 * s;
    r2[3] = r2[3] - m2 * s;
    r3[3] = r3[3] - m3 * s;
    s = r0[4];
    if (s != 0.0) {
	r1[4] = r1[4] - m1 * s;
	r2[4] = r2[4] - m2 * s;
	r3[4] = r3[4] - m3 * s;
    }
    s = r0[5];
    if (s != 0.0) {
	r1[5] = r1[5] - m1 * s;
	r2[5] = r2[5] - m2 * s;
	r3[5] = r3[5] - m3 * s;
    }
    s = r0[6];
    if (s != 0.0) {
	r1[6] = r1[6] - m1 * s;
	r2[6] = r2[6] - m2 * s;
	r3[6] = r3[6] - m3 * s;
    }
    s = r0[7];
    if (s != 0.0) {
	r1[7] = r1[7] - m1 * s;
	r2[7] = r2[7] - m2 * s;
	r3[7] = r3[7] - m3 * s;
    }

    if (r1[1] == 0.0) {		/* swap rows if needed		*/
	if (r2[1] == 0.0) {
	    if (r3[1] == 0.0) goto singular;
	    rtemp = r1; r1 = r3; r3 = rtemp;
	}
	else {rtemp = r1; r1 = r2; r2 = rtemp;}
    }
    m2 = r2[1]/r1[1];		/* eliminate second variable	*/
    m3 = r3[1]/r1[1];
    r2[2] = r2[2] - m2 * r1[2];
    r3[2] = r3[2] - m3 * r1[2];
    r3[3] = r3[3] - m3 * r1[3];
    r2[3] = r2[3] - m2 * r1[3];
    s = r1[4];
    if (s != 0.0) {
	r2[4] = r2[4] - m2 * s;
	r3[4] = r3[4] - m3 * s;
    }
    s = r1[5];
    if (s != 0.0) {
	r2[5] = r2[5] - m2 * s;
	r3[5] = r3[5] - m3 * s;
    }
    s = r1[6];
    if (s != 0.0) {
	r2[6] = r2[6] - m2 * s;
	r3[6] = r3[6] - m3 * s;
    }
    s = r1[7];
    if (s != 0.0) {
	r2[7] = r2[7] - m2 * s;
	r3[7] = r3[7] - m3 * s;
    }

    if (r2[2] == 0.0) {		/* swap last 2 rows if needed	*/
	if (r3[2] == 0.0) goto singular;
	rtemp = r2; r2 = r3; r3 = rtemp;
    }
    m3 = r3[2]/r2[2];		/* eliminate third variable	*/
    r3[3] = r3[3] - m3 * r2[3];
    r3[4] = r3[4] - m3 * r2[4];
    r3[5] = r3[5] - m3 * r2[5];
    r3[6] = r3[6] - m3 * r2[6];
    r3[7] = r3[7] - m3 * r2[7];

    if (r3[3] == 0.0) goto singular;
    s = 1.0f/r3[3];		/* now back substitute row 3	*/
    r3[4] = r3[4] * s;
    r3[5] = r3[5] * s;
    r3[6] = r3[6] * s;
    r3[7] = r3[7] * s;

    m2 = r2[3];			/* now back substitute row 2	*/
    s = 1.0f/r2[2];
    r2[4] = s * (r2[4] - r3[4] * m2);
    r2[5] = s * (r2[5] - r3[5] * m2);
    r2[6] = s * (r2[6] - r3[6] * m2);
    r2[7] = s * (r2[7] - r3[7] * m2);
    m1 = r1[3];
    r1[4] = (r1[4] - r3[4] * m1);
    r1[5] = (r1[5] - r3[5] * m1);
    r1[6] = (r1[6] - r3[6] * m1);
    r1[7] = (r1[7] - r3[7] * m1);
    m0 = r0[3];
    r0[4] = (r0[4] - r3[4] * m0);
    r0[5] = (r0[5] - r3[5] * m0);
    r0[6] = (r0[6] - r3[6] * m0);
    r0[7] = (r0[7] - r3[7] * m0);

    m1 = r1[2];			/* now back substitute row 1	*/
    s = 1.0f/r1[1];
    r1[4] = s * (r1[4] - r2[4] * m1);
    r1[5] = s * (r1[5] - r2[5] * m1);
    r1[6] = s * (r1[6] - r2[6] * m1);
    r1[7] = s * (r1[7] - r2[7] * m1);
    m0 = r0[2];
    r0[4] = (r0[4] - r2[4] * m0);
    r0[5] = (r0[5] - r2[5] * m0);
    r0[6] = (r0[6] - r2[6] * m0);
    r0[7] = (r0[7] - r2[7] * m0);

    m0 = r0[1];			/* now back substitute row 0	*/
    s = 1.0f/r0[0];
    r0[4] = s * (r0[4] - r1[4] * m0);
    r0[5] = s * (r0[5] - r1[5] * m0);
    r0[6] = s * (r0[6] - r1[6] * m0);
    r0[7] = s * (r0[7] - r1[7] * m0);

    to[0][0] = r0[4];		/* copy results back		*/
    to[0][1] = r0[5];
    to[0][2] = r0[6];
    to[0][3] = r0[7];
    to[1][0] = r1[4];
    to[1][1] = r1[5];
    to[1][2] = r1[6];
    to[1][3] = r1[7];
    to[2][0] = r2[4];
    to[2][1] = r2[5];
    to[2][2] = r2[6];
    to[2][3] = r2[7];
    to[3][0] = r3[4];
    to[3][1] = r3[5];
    to[3][2] = r3[6];
    to[3][3] = r3[7];
    return;

singular:
    fprintf(stderr,"invertmat: singular matrix\n");
    return;
}
