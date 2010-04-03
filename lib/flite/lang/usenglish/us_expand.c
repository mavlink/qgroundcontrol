/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2001                            */
/*                        All Rights Reserved.                           */
/*                                                                       */
/*  Permission is hereby granted, free of charge, to use and distribute  */
/*  this software and its documentation without restriction, including   */
/*  without limitation the rights to use, copy, modify, merge, publish,  */
/*  distribute, sublicense, and/or sell copies of this work, and to      */
/*  permit persons to whom this work is furnished to do so, subject to   */
/*  the following conditions:                                            */
/*   1. The code must retain the above copyright notice, this list of    */
/*      conditions and the following disclaimer.                         */
/*   2. Any modifications must be clearly marked as such.                */
/*   3. Original authors' names are not deleted.                         */
/*   4. The authors' names are not used to endorse or promote products   */
/*      derived from this software without specific prior written        */
/*      permission.                                                      */
/*                                                                       */
/*  CARNEGIE MELLON UNIVERSITY AND THE CONTRIBUTORS TO THIS WORK         */
/*  DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING      */
/*  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT   */
/*  SHALL CARNEGIE MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE      */
/*  FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES    */
/*  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN   */
/*  AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,          */
/*  ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF       */
/*  THIS SOFTWARE.                                                       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  January 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  English text expanders                                               */
/*                                                                       */
/*  numbers, digits, ids (years), money                                  */
/*                                                                       */
/*************************************************************************/

#include <ctype.h>
#include "us_text.h"

static const char * const digit2num[] = {
    "zero",
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine" };

static const char * const digit2teen[] = {
    "ten",  /* shouldn't get called */
    "eleven",
    "twelve",
    "thirteen",
    "fourteen",
    "fifteen",
    "sixteen",
    "seventeen",
    "eighteen",
    "nineteen" };

static const char * const digit2enty[] = {
    "zero",  /* shouldn't get called */
    "ten",
    "twenty",
    "thirty",
    "forty",
    "fifty",
    "sixty",
    "seventy",
    "eighty",
    "ninety" };

static const char * const ord2num[] = {
    "zeroth",
    "first",
    "second",
    "third",
    "fourth",
    "fifth",
    "sixth",
    "seventh",
    "eighth",
    "ninth" };

static const char * const ord2teen[] = {
    "tenth",  /* shouldn't get called */
    "eleventh",
    "twelfth",
    "thirteenth",
    "fourteenth",
    "fifteenth",
    "sixteenth",
    "seventeenth",
    "eighteenth",
    "nineteenth" };

static const char * const ord2enty[] = {
    "zeroth",  /* shouldn't get called */
    "tenth",
    "twentieth",
    "thirtieth",
    "fortieth",
    "fiftieth",
    "sixtieth",
    "seventieth",
    "eightieth",
    "ninetieth" };

cst_val *en_exp_number(const char *numstring)
{
    /* Expand given token to list of words pronouncing it as a number */
    int num_digits = cst_strlen(numstring);
    char part[4];
    cst_val *p;
    int i;

    if (num_digits == 0)
	return NULL;
    else if (num_digits == 1)
	return en_exp_digits(numstring);
    else if (num_digits == 2)
    {
	if (numstring[0] == '0')
	{
	    if (numstring[1] == '0')
		return 0;
	    else
		return cons_val(string_val(digit2num[numstring[1]-'0']),0);
	}
	else if (numstring[1] == '0')
	    return cons_val(string_val(digit2enty[numstring[0]-'0']),0);
	else if (numstring[0] == '1')
	    return cons_val(string_val(digit2teen[numstring[1]-'0']),0);
	else 
	    return cons_val(string_val(digit2enty[numstring[0]-'0']),
			    en_exp_digits(numstring+1));
    }
    else if (num_digits == 3)
    {
	if (numstring[0] == '0')
	    return en_exp_number(numstring+1);
	else
	    return cons_val(string_val(digit2num[numstring[0]-'0']),
				cons_val(string_val("hundred"),
					     en_exp_number(numstring+1)));
    }
    else if (num_digits < 7)
    {
	for (i=0; i < num_digits-3; i++)
	    part[i] = numstring[i];
	part[i]='\0';
	p = en_exp_number(part);
	if (p == 0)  /* no thousands */
	    return en_exp_number(numstring+i);
	else
	    return val_append(p,cons_val(string_val("thousand"),
					 en_exp_number(numstring+i)));
    }
    else if (num_digits < 10)
    {
	for (i=0; i < num_digits-6; i++)
	    part[i] = numstring[i];
	part[i]='\0';
	p = en_exp_number(part);
	if (p == 0)  /* no millions */
	    return en_exp_number(numstring+i);
	else
	    return val_append(p,cons_val(string_val("million"),
					 en_exp_number(numstring+i)));
    }
    else if (num_digits < 13)
    {   /* If there are pedantic brits out there, tough!, 10^9 is a billion */
	for (i=0; i < num_digits-9; i++)
	    part[i] = numstring[i];
	part[i]='\0';
	p = en_exp_number(part);
	if (p == 0)  /* no billions */
	    return en_exp_number(numstring+i);
	else
	    return val_append(p,cons_val(string_val("billion"),
					 en_exp_number(numstring+i)));
    }
    else  /* Way too many digits here, to be a number */
    {
	return en_exp_digits(numstring);
    }
}

cst_val *en_exp_ordinal(const char *rawnumstring)
{
    /* return ordinal for digit string */
    cst_val *card, *o;
    const cst_val *t;
    const char *l;
    const char *ord;
    char *numstring;
    int i,j;

    numstring = cst_strdup(rawnumstring);
    for (j=i=0; i < cst_strlen(rawnumstring); i++)
	if (rawnumstring[i] != ',')
	{
	    numstring[j] = rawnumstring[i];
	    j++;
	}
    numstring[j] = '\0';
    card = val_reverse(en_exp_number(numstring));
    cst_free(numstring);

    l = val_string(val_car(card));
    ord = 0;
    for (i=0; i<10; i++)
	if (cst_streq(l,digit2num[i]))
	    ord = ord2num[i];
    if (!ord)
	for (i=0; i<10; i++)
	    if (cst_streq(l,digit2teen[i]))
		ord = ord2teen[i];
    if (!ord)
	for (i=0; i<10; i++)
	    if (cst_streq(l,digit2enty[i]))
		ord = ord2enty[i];
    if (cst_streq(l,"hundred"))
	ord = "hundredth";
    if (cst_streq(l,"thousand"))
	ord = "thousandth";
    if (cst_streq(l,"billion"))
	ord = "billtionth";
    if (!ord)  /* dunno, so don't convert anything */
	return card;
    o = cons_val(string_val(ord),0);
    for (t=val_cdr(card); t; t=val_cdr(t))
	o = cons_val(val_car(t),o);
    delete_val(card);
    return o;
}

cst_val *en_exp_id(const char *numstring)
{
    /* Expand numstring as pairs as in years or ids */
    char aaa[3];

    if ((cst_strlen(numstring) == 4) && 
	(numstring[2] == '0') &&
	(numstring[3] == '0'))
    {
	if (numstring[1] == '0')
	    return en_exp_number(numstring); /* 2000, 3000 */
	else
	{
	    aaa[0] = numstring[0];
	    aaa[1] = numstring[1];
	    aaa[2] = '\0';
	    return val_append(en_exp_number(aaa),
			      cons_val(string_val("hundred"),0));
	}
    }
    else if ((cst_strlen(numstring) == 3) && 
             (numstring[0] != '0') &&
             (numstring[1] == '0') && 
             (numstring[2] == '0'))
    {
        return cons_val(string_val(digit2num[numstring[0]-'0']),
                        cons_val(string_val("hundred"),0));
    }
    else if ((cst_strlen(numstring) == 2) && (numstring[0] == '0')
             && (numstring[1] == '0'))
	return cons_val(string_val("zero"),
                        cons_val(string_val("zero"),NULL));
    else if ((cst_strlen(numstring) == 2) && (numstring[0] == '0'))
	return cons_val(string_val("oh"),
			en_exp_digits(&numstring[1]));
    else if (((cst_strlen(numstring) == 4) && 
	 ((numstring[1] == '0'))) ||
	(cst_strlen(numstring) < 3))
	return en_exp_number(numstring);
    else if (cst_strlen(numstring)%2 == 1)
    {
	return cons_val(string_val(digit2num[numstring[0]-'0']),
			en_exp_id(&numstring[1]));
    }
    else 
    {
	aaa[0] = numstring[0];
	aaa[1] = numstring[1];
	aaa[2] = '\0';
	return val_append(en_exp_number(aaa),en_exp_id(&numstring[2]));
    }
}

cst_val *en_exp_real(const char *numstring)
{
    char *aaa, *p;
    cst_val *r;

    if (numstring && (numstring[0] == '-'))
	r = cons_val(string_val("minus"),
		     en_exp_real(&numstring[1]));
    else if (numstring && (numstring[0] == '+'))
	r = cons_val(string_val("plus"),
		     en_exp_real(&numstring[1]));
    else if (((p=strchr(numstring,'e')) != 0) ||
	     ((p=strchr(numstring,'E')) != 0))
    {
	aaa = cst_strdup(numstring);
	aaa[cst_strlen(numstring)-cst_strlen(p)] = '\0';
	r = val_append(en_exp_real(aaa),
		       cons_val(string_val("e"),
				en_exp_real(p+1)));
	cst_free(aaa);
    }
    else if ((p=strchr(numstring,'.')) != 0)
    {
	aaa = cst_strdup(numstring);
	aaa[cst_strlen(numstring)-cst_strlen(p)] = '\0';
	r = val_append(en_exp_number(aaa),
		       cons_val(string_val("point"),
				en_exp_digits(p+1)));
	cst_free(aaa);
    }
    else
	r = en_exp_number(numstring);  /* I don't think you can get here */

    return r;
}

cst_val *en_exp_digits(const char *numstring)
{
    /* Expand given token to list of words pronouncing it as digits */
    cst_val *d = 0;
    const char *p;

    for (p=numstring; *p; p++)
    {
	if ((*p >= '0') && (*p <= '9'))
	    d = cons_val(string_val(digit2num[*p-'0']),d);
	else
	    d = cons_val(string_val("umpty"),d);
    }

    return val_reverse(d);
}

cst_val *en_exp_letters(const char *lets)
{
    /* returns these as list of single char symbols */
    char *aaa;
    cst_val *r;
    int i;

    aaa = cst_alloc(char,2);
    aaa[1] = '\0';
    for (r=0,i=0; lets[i] != '\0'; i++)
    {
	aaa[0] = lets[i];
	if (isupper((int)aaa[0])) 
	    aaa[0] = tolower((int)aaa[0]);
	if (strchr("0123456789",aaa[0]))
	    r = cons_val(string_val(digit2num[aaa[0]-'0']),r);
	else if (cst_streq(aaa,"a"))
	    r = cons_val(string_val("_a"),r);
	else
	    r = cons_val(string_val(aaa),r);
    }
    cst_free(aaa);

    return val_reverse(r);
}

int en_exp_roman(const char *roman)
{
    int val;
    const char *p;
    val = 0;

    for (p=roman; *p != 0; p++)
    {
	if (*p == 'X')
	    val += 10;
	else if (*p == 'V')
	    val += 5;
	else if (*p == 'I')
	{
	    if (p[1] == 'V')
	    {
		val += 4;
		p++;
	    }
	    else if (p[1] == 'X')
	    {
		val += 9;
		p++;
	    }
	    else 
		val += 1;
	}
    }
    return val;
}


