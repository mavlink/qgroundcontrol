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
/*  US English text analysis functions                                   */
/*                                                                       */
/*************************************************************************/

#include <ctype.h>
#include "flite.h"
#include "usenglish.h"
#include "us_text.h"
#include "cst_regex.h"

static int text_splitable(const char *s,int i);
static cst_val *state_name(const char *name,cst_item *t);

/* compiled us regexes */
#include "us_regexes.h"

static const char * const wandm_abbrevs[99][2] =
{
    { "LB", "pounds" },
    { "LBS", "pounds" },
    { "lb", "pounds" },
    { "lbs", "pounds" },
    { "ft", "feet" },
    { "FT", "feet" },
    { "kg", "kilograms" },
    { "km", "kilometers" },
    { "oz", "ounces" },
    { "hz", "hertz" },
    { "Hz", "hertz" },
    { "HZ", "hertz" },
    { "KHz", "kilohertz" },
    { "MHz", "megahertz" },
    { "GHz", "gigahertz" },
    { NULL, NULL },
};

static const char * const eedwords[] = {
    "to",
    "can",
    "can't",
    "cannot",
    "cant",
    "could",
    "couldn't",
    "couldnt",
    "will",
    "shall",
    NULL};

void us_text_init()
{
    /* Nothing */
}

void us_text_deinit()
{
    /* Nothing */
}

static int rex_like(const cst_item *t)
{
    /* returns 1 if this is in a king like context */
    char *pn = cst_downcase(ffeature_string(t,"p.name"));
    char *ppn = cst_downcase(ffeature_string(t,"p.p.name"));
    int v = 0;
    
    if (cst_streq(pn,"louis") ||
	cst_streq(pn,"henry") ||
	cst_streq(pn,"charles") ||
	cst_streq(pn,"philip") ||
	cst_streq(pn,"george") ||
	cst_streq(pn,"edward") ||
	cst_streq(pn,"pius") ||
	cst_streq(pn,"william") ||
	cst_streq(pn,"richard") ||
	cst_streq(pn,"ptolemy") ||
	cst_streq(pn,"john") ||
	cst_streq(pn,"paul") ||
	cst_streq(pn,"peter") ||
	cst_streq(pn,"nicholas") ||
	cst_streq(pn,"frederick") ||
	cst_streq(pn,"james") ||
	cst_streq(pn,"alfonso") ||
	cst_streq(pn,"ivan") ||
	cst_streq(pn,"napolean") ||
	cst_streq(pn,"leo") ||
	cst_streq(pn,"gregory") ||
	cst_streq(pn,"catherine") ||
	cst_streq(pn,"alexandria") ||
	cst_streq(pn,"pierre") ||
	cst_streq(pn,"elizabeth") ||
	cst_streq(pn,"mary"))
	v = 1;
    else if (cst_streq(ppn,"king") ||
	     cst_streq(ppn,"queen") ||
	     cst_streq(ppn,"pope") ||
	     cst_streq(ppn,"duke") ||
	     cst_streq(ppn,"tsar") ||
	     cst_streq(ppn,"emperor") ||
	     cst_streq(ppn,"shah") ||
	     cst_streq(ppn,"ceasar") ||
	     cst_streq(ppn,"duchess") ||
	     cst_streq(ppn,"tsarina") ||
	     cst_streq(ppn,"empress") ||
	     cst_streq(ppn,"baron") ||
	     cst_streq(ppn,"baroness") ||
	     cst_streq(ppn,"sultan") ||
	     cst_streq(ppn,"count") ||
	     cst_streq(ppn,"countess"))
	v = 1;

    cst_free(pn);
    cst_free(ppn);
    return v;
}

static int section_like(const cst_item *t)
{
    /* returns 1 if this is in a king like context */
    char *pn = cst_downcase(ffeature_string(t,"p.name"));
    int v = 0;

    if (cst_streq(pn,"section") ||
	cst_streq(pn,"chapter") ||
	cst_streq(pn,"part") ||
	cst_streq(pn,"phrase") ||
	cst_streq(pn,"verse") ||
	cst_streq(pn,"scene") ||
	cst_streq(pn,"act") ||
	cst_streq(pn,"book") ||
	cst_streq(pn,"volume") ||
	cst_streq(pn,"chap") ||
	cst_streq(pn,"war") ||
	cst_streq(pn,"apollo") ||
	cst_streq(pn,"trek") ||
	cst_streq(pn,"fortran"))
	v = 1;
    
    cst_free(pn);
    
    return v;
}

cst_utterance *us_textanalysis(cst_utterance *u)
{
    if (!feat_present(u->features, "tokentowords_func"))
	utt_set_feat(u, "tokentowords_func", itemfunc_val(us_tokentowords));

    return default_textanalysis(u);
}

static cst_val *us_tokentowords_one(cst_item *token, const char *name);
cst_val *us_tokentowords(cst_item *token)
{
    return us_tokentowords_one(token, item_feat_string(token, "name"));
}

static cst_val *add_break(cst_val *l)
{
    /* add feature (break 1) to last item in this list */
    const cst_val *i;
    cst_val *t;
    cst_features *f;

    for (i=l; val_cdr(i); i=val_cdr(i));

    if (i)  /* might be empty list */
    {
	f = new_features();
	feat_set_string(f,"break","1");
	t = cons_val(val_car(i),features_val(f));
	set_car((cst_val *)i,t);
    }

    return l;
}

static cst_val *us_tokentowords_one(cst_item *token, const char *name)
{
    /* Return list of words that expand token/name */
    char *p, *aaa, *bbb, *ccc;
    int i,j,k,l;
    cst_val *r, *s;
    const char *nsw = "";
    cst_lexicon *lex;
    cst_utterance *utt;
    /* printf("token_name %s name %s\n",item_name(token),name); */
    /* FIXME: For SAPI and friends, any tokens with explicit
       pronunciations need to be passed through as-is.  This should be
       done in the interface code rather than here once the
       tokentowords hook is accessible. AWB: no, they should set the
       nsw feature and this function should deal with it (doesn't yet though)*/
    if (item_feat_present(token,"phones"))
	return cons_val(string_val(name),NULL);

    if (item_feat_present(token,"nsw"))
	nsw = item_feat_string(token,"nsw");

    utt = item_utt(token);
    lex = val_lexicon(feat_val(utt->features,"lexicon"));

    if ((cst_streq("a",name) || cst_streq("A",name)) &&
        ((item_next(token) == 0) ||
         (!cst_streq(name,item_name(token))) ||
         (!cst_streq("",ffeature_string(token,"punc")))))
    {   /* if A is a sub part of a token, then its ey not ah */
	r = cons_val(string_val("_a"),0);
    }
    else if (cst_strlen(name) == 0)
        r = NULL;
    else if (cst_regex_match(dottedabbrevs,name))
    {   /* X.X.X */
	aaa = cst_strdup(name);
	for (i=j=0; aaa[i]; i++)
	    if (aaa[i] != '.')
	    {
		aaa[j] = aaa[i];
		j++;
	    }
	aaa[j] = '\0';
	r = en_exp_letters(aaa);
	cst_free(aaa);
    }
    else if (cst_regex_match(cst_rx_commaint,name))
    {   /* 99,999,999 */
	aaa = cst_strdup(name);
	for (j=i=0; i < cst_strlen(name); i++)
	    if (name[i] != ',')
	    {
		aaa[j] = name[i];
		j++;
	    }
	aaa[j] = '\0';
	r = en_exp_real(aaa);
	cst_free(aaa);
    }
    else if (cst_regex_match(sevenphonenumber,name))
    {   /* 234-3434 telephone numbers */
	p=strchr(name,'-');
	aaa = cst_strdup(name);
	aaa[cst_strlen(name)-cst_strlen(p)] = '\0';
	bbb = cst_strdup(p+1);
	r = val_append(add_break(en_exp_digits(aaa)),
		       en_exp_digits(bbb));
	cst_free(aaa);
	cst_free(bbb);
    }
    else if 
     ((cst_regex_match(threedigits,name) &&
      ((!cst_regex_match(cst_rx_digits,ffeature_string(token,"p.name"))
	&& cst_regex_match(threedigits,ffeature_string(token,"n.name"))
	&& cst_regex_match(fourdigits,ffeature_string(token,"n.n.name"))) ||
       (cst_regex_match(sevenphonenumber,ffeature_string(token,"n.name"))) ||
       (!cst_regex_match(cst_rx_digits,ffeature_string(token,"p.p.name"))
	&& cst_regex_match(threedigits,ffeature_string(token,"p.name"))
	&& cst_regex_match(fourdigits,ffeature_string(token,"n.name"))))) ||
      (cst_regex_match(fourdigits,name) &&
       (!cst_regex_match(cst_rx_digits,ffeature_string(token,"n.name"))
	&& cst_regex_match(threedigits,ffeature_string(token,"p.name"))
	&& cst_regex_match(threedigits,ffeature_string(token,"p.p.name")))))
    {
	/* part of a telephone number */
	if (cst_streq("",ffeature_string(token,"punc")))
	    item_set_string(token,"punc",",");
	r = add_break(en_exp_digits(name));
    }
    else if (cst_regex_match(numbertime,name))
    {
	p=strchr(name,':');
	aaa = cst_strdup(name);
	aaa[cst_strlen(name)-cst_strlen(p)] = '\0';
	bbb = cst_strdup(p+1);

	r = en_exp_number(aaa);
	if (!cst_streq("00",bbb))
	    r = val_append(r,en_exp_id(bbb));
	/* r = add_break(r); */

	cst_free(aaa);
	cst_free(bbb);
    }
    else if (cst_regex_match(numbertimexm,name))
    {
	p=strchr(name,':');
        if (!p) p=strchr(name,'.');
	aaa = cst_strdup(name);
	aaa[cst_strlen(name)-cst_strlen(p)] = '\0';
	bbb = cst_strdup(p+1);
	bbb[2] = '\0';
        ccc = cst_strdup(p+3);

	r = en_exp_number(aaa);
	if (!cst_streq("00",bbb))
	    r = val_append(r,en_exp_id(bbb));
	/* r = add_break(r); */

        r = val_append(r,en_exp_letters(ccc));

	cst_free(aaa);
	cst_free(bbb);
	cst_free(ccc);
    }
    else if (cst_regex_match(digits2dash,name))
    {   /* 999-999-999 etc */
	bbb = cst_strdup(name);
	for (r=0,aaa=p=bbb; *p; p++)
	{
	    if (*p == '-')
	    {
		*p = '\0';
		r = val_append(val_reverse(add_break(en_exp_digits(aaa))),r);
		aaa = p+1;
	    }
	}
	r = val_append(val_reverse(add_break(en_exp_digits(aaa))),r);
	r = val_reverse(r);
	cst_free(bbb);
    }
    else if (cst_regex_match(cst_rx_digits,name))
    {   /* string of digits (use cart to disambiguate) */
	if (cst_streq("nide",nsw))
	    r = en_exp_id(name);
	else {
	    const cst_val *tv;
	    const char *ts;
	    char *rname;
	    
	    rname = cst_strdup(item_feat_string(token,"name"));
	    if (cst_streq(name,rname))
		tv = cart_interpret(token,&us_nums_cart);
	    else 
	    {   /* in a recursive call */
		item_set_string(token,"name",name);
		tv = cart_interpret(token,&us_nums_cart);
		item_set_string(token,"name",rname);
	    }
	    cst_free(rname);
	    ts = val_string(tv);
	    if (cst_streq(ts,"ordinal"))
		r = en_exp_ordinal(name);
	    else if (cst_streq(ts,"digits"))
		r = en_exp_digits(name);
	    else if (cst_streq(ts,"year"))
		r = en_exp_id(name);
	    else
		r = en_exp_number(name);
	}
    }
    else if (cst_regex_match(romannums,name))
    {   /* Roman numerals */
	if (cst_streq("",ffeature_string(token,"p.punc")))
	{   /* no preceeding punc */
	    char n[10];
	    cst_sprintf(n,"%d",en_exp_roman(name));
	    if (rex_like(token))
		r = cons_val(string_val("the"),
			     en_exp_ordinal(n));
	    else if (section_like(token))
		r = en_exp_number(n);
	    else
		r = en_exp_letters(name);
	}
	else
	    r = en_exp_letters(name);
    }
    else if (cst_regex_match(drst,name))  
    {   /* St Andrew's St, Dr King Dr */
	const char *street;
	const char *saint;
	if ((name[0] == 's') || (name[0] == 'S'))
	{
	    street = "street";
	    saint = "saint";
	}
	else
	{
	    street = "drive";
	    saint = "doctor";
	}
	if ((item_next(token) == 0) ||
	    strchr(item_feat_string(token,"punc"),','))
	    r = cons_val(string_val(street),NULL);
	else if (strchr(ffeature_string(token,"punc"),','))
	    r = cons_val(string_val(saint),NULL);
	else 
	{
	    const char *pname = ffeature_string(token,"p.name");
	    const char *nname = ffeature_string(token,"n.name");
	    if ((pname[0] >= 'A') && (pname[0] <= 'Z') &&
		(nname[0] >= 'a') && (nname[0] <= 'z'))
		r = cons_val(string_val(street),NULL);
	    else if ((pname[0] >= '0') && (pname[0] <= '9') &&
		     (nname[0] >= 'a') && (nname[0] <= 'z'))
		r = cons_val(string_val(street),NULL);
	    else if ((pname[0] >= 'a') && (pname[0] <= 'z') &&
		     (nname[0] >= 'A') && (nname[0] <= 'Z'))
		r = cons_val(string_val(saint),NULL);
	    else if (cst_streq(ffeature_string(token,"n.whitespace")," "))
		r = cons_val(string_val(saint),NULL);
	    else
		r = cons_val(string_val(street),NULL);
	}
	if (cst_streq(item_feat_string(token,"punc"),"."))
	    item_set_string(token,"punc","");
    }
    else if (cst_streq(name,"Mr"))
    {
	item_set_string(token,"punc","");
	r = cons_val(string_val("mister"),NULL);
    }
    else if (cst_streq(name,"Mrs"))
    {
	item_set_string(token,"punc","");
	r = cons_val(string_val("missus"),NULL);
    }
    else if ((cst_streq(name,"read")) ||
             (cst_streq(name,"lead")))
    {   /* checking WSJ examples, this seems a quick and easy way to */
        /* get manty of these correct */
        const char *pname = ffeature_string(token,"p.name");

        for (i=0; eedwords[i]; i++)
            if (cst_streq(pname,eedwords[i]))
                break;
        
        if (eedwords[i])
        {  /* reed or leed */
            if (name[0] == 'r')
                r = cons_val(string_val("reed"),NULL);
            else
                r = cons_val(string_val("leed"),NULL);
        }
        else /* red or led */
        {
            if (name[0] == 'r')
                r = cons_val(string_val("red"),NULL);
            else
                r = cons_val(string_val("led"),NULL);
        }
    }
    else if (cst_streq(name,"am") || cst_streq(name,"AM"))
    {
        if (!cst_streq(name,item_name(token)))
            r = en_exp_letters(name);
        else if (item_prev(token) &&
                 (cst_regex_match(numbertime,ffeature_string(token,"p.name")) ||
                  cst_regex_match(cst_rx_digits,ffeature_string(token,"p.name"))))
            r = en_exp_letters(name);
        else 
            r = cons_val(string_val(name),NULL);
    }
    else if ((cst_strlen(name) == 1) &&
	     (name[0] >= 'A') &&
	     (name[0] <= 'Z') &&
	     (cst_streq(" ",ffeature_string(token,"n.whitespace"))) &&
	     (ffeature_string(token,"n.name")[0] >= 'A') &&
	     (ffeature_string(token,"n.name")[0] <= 'Z'))
    {
	item_set_string(token,"punc","");
	aaa = cst_downcase(name);
	if (cst_streq(aaa,"a"))
	    r = cons_val(string_val("_a"),0);
	else
	    r = cons_val(string_val(aaa),0);
	cst_free(aaa);
    }
    else if (cst_regex_match(cst_rx_double,name))
    {   /* real numbers */
	r = en_exp_real(name);
    }
    else if (cst_regex_match(ordinal_number,name))
    {   /* explicit ordinals */
	aaa = cst_strdup(name);
	aaa[cst_strlen(name)-2] = '\0';
	r = en_exp_ordinal(aaa);
	cst_free(aaa);
    }
    else if ((cst_regex_match(illion,name)) &&
	     (cst_regex_match(usmoney,ffeature_string(token,"p.name"))))
    {
	r = cons_val(string_val(name),
		     cons_val(string_val("dollars"),NULL));
    }
    else if (cst_regex_match(usmoney,name))
    {
	/* US money */
/*	printf("money, money, money %s\n", name); */
	p = strchr(name,'.');

	if (cst_regex_match(illion,ffeature_string(token,"n.name")))
	{   /* carl sagan's billions and billions */
	    r = en_exp_real(&name[1]);
	}
	else if (!p)
	{
	    aaa = cst_strdup(&name[1]);
	    if (cst_streq("1",aaa))
		r = cons_val(string_val("dollar"),NULL);
	    else
		r = cons_val(string_val("dollars"),NULL);
	    r = val_append(us_tokentowords_one(token,aaa),r);
	    cst_free(aaa);
	}
	else if ((cst_strlen(p) == 1) || (cst_strlen(p) > 3))
	{   /* simply read as mumble point mumble */
	    r = val_append(en_exp_real(&name[1]),
			   cons_val(string_val("dollars"),NULL));
	}
	else
	{
	    aaa = cst_strdup(name);
	    aaa[cst_strlen(name)-cst_strlen(p)] = '\0';
	    for (i=j=0; aaa[i] != '\0'; i++)
	    {
		if (aaa[i] != ',')
		{
		    aaa[j] = aaa[i];
		    j++;
		}
	    }
	    aaa[j] = '\0';
	    if (cst_streq("00",p+1))
		r = 0;
	    else if (cst_streq("01",p+1))
		r = val_append(en_exp_number(p+1),
			       cons_val(string_val("cent"),NULL));
	    else
		r = val_append(en_exp_number(p+1),
			       cons_val(string_val("cents"),NULL));

	    if (cst_streq("1",aaa+1))
		r = cons_val(string_val("dollar"),r);
	    else
		r = cons_val(string_val("dollars"),r);

	    r = val_append(en_exp_number(aaa+1),r);
	    cst_free(aaa);
	}
    }
    else if (name[cst_strlen(name)-1] == '%')
    {
	aaa = cst_strdup(name);
	aaa[cst_strlen(aaa)-1] = '\0';
	r = val_append(us_tokentowords_one(token,aaa),
		       cons_val(string_val("per"),
				cons_val(string_val("cent"),NULL)));
	cst_free(aaa);

    }
    else if (cst_regex_match(numess,name)) 
    {   /* 60s and 7s and 9s */
	aaa = cst_strdup(name);
	aaa[cst_strlen(name)-1] = '\0';
	r = val_append(en_exp_number(aaa),
		       cons_val(string_val("'s"),0));
	cst_free(aaa);
    }
    else if ((p=(cst_strrchr(name,'\''))))
    {
	static const char *pc[] = { "'s", "'ll", "'ve", "'d", NULL };

	bbb = cst_downcase(p);
	if (cst_member_string(bbb, pc))
	{
	    aaa = cst_strdup(name);
	    aaa[cst_strlen(name)-cst_strlen(p)] = '\0';
	    r = val_append(us_tokentowords_one(token,aaa),
			   cons_val(string_val(bbb),0));
	    cst_free(aaa);
	}
	else if (cst_streq(p,"'tve")) /* admittedly rare and weird */
	{
	    aaa = cst_strdup(name);
	    aaa[cst_strlen(name)-cst_strlen(p)+2] = '\0';
	    r = val_append(us_tokentowords_one(token,aaa),
			   cons_val(string_val("'ve"),0));
	    cst_free(aaa);
	}
	else
	{
	    aaa = cst_strdup(name);
	    strcpy(&aaa[cst_strlen(name)-cst_strlen(p)],p+1);
	    r = us_tokentowords_one(token,aaa);
	    cst_free(aaa);
	}
	cst_free(bbb);
    }
    else if ((cst_regex_match(digitsslashdigits,name)) &&
	     (cst_streq(name,item_name(token))))
    {   /* might be fraction, or not */
	p=strchr(name,'/');
	aaa = cst_strdup(name);
	aaa[cst_strlen(name)-cst_strlen(p)] = '\0';
	bbb = cst_strdup(p+1);
	if ((cst_streq("1",aaa)) && (cst_streq("2",bbb)))
	    r = cons_val(string_val("a"),
			 cons_val(string_val("half"),0));
	else if (atoi(aaa) < (atoi(bbb)))
	{
	    r = val_append(en_exp_number(aaa),
			   en_exp_ordinal(bbb));
	    if (atoi(aaa) > 1)
		r = val_append(r,cons_val(string_val("'s"),0));
	}
	else
	    r = val_append(en_exp_number(aaa),
			   cons_val(string_val("slash"),
				    en_exp_number(bbb)));

	if ((cst_regex_match(cst_rx_digits,ffeature_string(token,"p.name")))
	    && (item_prev(token)))  /* don't mistake "0" as a number */
	    r = cons_val(string_val("and"),r);
	cst_free(aaa);
	cst_free(bbb);
    }
    else if ((p=(strchr(name,'-'))))
    {   /* aaa-bbb */
	aaa = cst_strdup(name);
	aaa[cst_strlen(name)-cst_strlen(p)] = '\0';
	bbb = cst_strdup(p+1);
	if (cst_regex_match(cst_rx_digits,aaa) &&
	    cst_regex_match(cst_rx_digits,bbb))
	{
            ccc = cst_strdup(name);
	    item_set_string(token,"name",bbb);
	    r = us_tokentowords_one(token,bbb);
	    item_set_string(token,"name",aaa);
	    r = val_append(us_tokentowords_one(token,aaa),
			   cons_val(string_val("to"),r));
	    item_set_string(token,"name",ccc);
            cst_free(ccc);
	}
	else
	    r = val_append(us_tokentowords_one(token,aaa),
			   us_tokentowords_one(token,bbb));
	cst_free(aaa);
	cst_free(bbb);
    }
    else if (cst_regex_match(wandm,name))
    {   /* weights and measures */

        for (j=cst_strlen(name)-1; j > 0; j--)
            if (cst_strchr("0123456789",name[j]))
                break;
        j += 1;
        for (i=0; wandm_abbrevs[i][0]; i++)
            if (cst_streq(name+j,wandm_abbrevs[i][0]))
                break;
        aaa = cst_strdup(name);
        aaa[j] = '\0';
        /* remove any commas */
        for (k=0,l=0; aaa[l]; k++,l++)
        {
            if (aaa[l] == ',') l++;
            aaa[k] = aaa[l];
        }
        aaa[k] = '\0';
        if (!wandm_abbrevs[i][0]) /* didn't find an expansion */
	    r = val_append(en_exp_number(aaa),
			   us_tokentowords_one(token,name+j));
        else
            r = val_append(en_exp_number(aaa),
                           cons_val(string_val(wandm_abbrevs[i][1]),NULL));

        cst_free(aaa);
    }
    else if ((cst_strlen(name) > 1) && (!cst_regex_match(cst_rx_alpha,name)))
    {   /* its not just alphas */
	for (i=0; name[i] != '\0'; i++)
	    if (text_splitable(name,i))
		break;
	aaa = cst_strdup(name);
	aaa[i+1] = '\0';
	bbb = cst_strdup(&name[i+1]);
	item_set_string(token,"nsw","nide");
	r = val_append(us_tokentowords_one(token,aaa),
		       us_tokentowords_one(token,bbb));
	cst_free(aaa);
	cst_free(bbb);
    }
    else if ((s = state_name(name,token)))
    {
	r = s;
    }
    else if ((cst_strlen(name) > 1) && 
	     (cst_regex_match(cst_rx_alpha,name)) &&
             (!in_lex(lex,name,NULL)) &&
	     (!us_aswd(name)))
        /* Still not quiet right, if there is a user_lex we need to check */
        /* it too -- but user_lex isn't user setable yet */
	/* Need common exception list */
	/* unpronouncable list of alphas */
	r = en_exp_letters(name);

    /* buckets of other stuff missing */

    else  /* just a word */
    {
	aaa = cst_downcase(name);
	r = cons_val(string_val(aaa),0);
	cst_free(aaa);
    }
    return r;
}

static int text_splitable(const char *s,int i)
{
    /* should token be split after this */

    if (strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",s[i]) &&
	strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ",s[i+1]))
	return FALSE;
    else if (strchr("0123456789",s[i]) &&
	     strchr("0123456789",s[i+1]))
	return FALSE;
    else
	return TRUE;
}

static const char * const states[99][5] =
{
  { "AL", "ambiguous", "alabama" , NULL, NULL },
  { "Al", "ambiguous", "alabama" , NULL, NULL },
  { "Ala", "", "alabama" , NULL, NULL },
  { "AK", "", "alaska" , NULL, NULL },
  { "Ak", "", "alaska" , NULL, NULL },
  { "AZ", "", "arizona" , NULL, NULL },
  { "Az", "", "arizona" , NULL, NULL },
  { "CA", "", "california" , NULL, NULL },
  { "Ca", "", "california" , NULL, NULL },
  { "Cal", "ambiguous", "california" , NULL, NULL },
  { "Calif", "", "california" , NULL, NULL },
  { "CO", "ambiguous", "colorado" , NULL, NULL },
  { "Co", "ambiguous", "colorado" , NULL, NULL },
  { "Colo", "", "colorado" , NULL, NULL },
  { "DC", "", "d" , "c", NULL },
  { "DE", "", "delaware" , NULL, NULL },
  { "De", "ambiguous", "delaware" , NULL, NULL },
  { "Del", "ambiguous", "delaware" , NULL, NULL },
  { "FL", "", "florida" , NULL, NULL },
  { "Fl", "ambiguous", "florida" , NULL, NULL },
  { "Fla", "", "florida" , NULL, NULL },
  { "GA", "", "georgia" , NULL, NULL },
  { "Ga", "", "georgia" , NULL, NULL },
  { "HI", "", "hawaii" , NULL, NULL },
  { "Hi", "ambiguous", "hawaii" , NULL, NULL },
  { "IA", "", "iowa" , NULL, NULL },
  { "Ia", "ambiguous", "iowa" , NULL, NULL },
  { "Ind", "ambiguous", "indiana" , NULL, NULL },
  { "ID", "ambiguous", "idaho" , NULL, NULL },
  { "IL", "ambiguous", "illinois" , NULL, NULL },
  { "Il", "ambiguous", "illinois" , NULL, NULL },
  { "ILL", "ambiguous", "illinois" , NULL, NULL },
  { "KS", "", "kansas" , NULL, NULL },
  { "Ks", "", "kansas" , NULL, NULL },
  { "Kans", "", "kansas" , NULL, NULL },
  { "KY", "ambiguous", "kentucky" , NULL, NULL },
  { "Ky", "ambiguous", "kentucky" , NULL, NULL },
  { "LA", "ambiguous", "louisiana" , NULL, NULL },
  { "La", "ambiguous", "louisiana" , NULL, NULL },
  { "Lou", "ambiguous", "louisiana" , NULL, NULL },
  { "Lous", "ambiguous", "louisiana" , NULL, NULL },
  { "MA", "ambiguous", "massachusetts" , NULL, NULL },
  { "Mass", "ambiguous", "massachusetts" , NULL, NULL },
  { "Ma", "ambiguous", "massachusetts" , NULL, NULL },
  { "MD", "ambiguous", "maryland" , NULL, NULL },
  { "Md", "ambiguous", "maryland" , NULL, NULL },
  { "ME", "ambiguous", "maine" , NULL, NULL },
  { "Me", "ambiguous", "maine" , NULL, NULL },
  { "MI", "", "michigan" , NULL, NULL },
  { "Mi", "ambiguous", "michigan" , NULL, NULL },
  { "Mich", "ambiguous", "michigan" , NULL, NULL },
  { "MN", "ambiguous", "minnestota" , NULL, NULL },
  { "Minn", "ambiguous", "minnestota" , NULL, NULL },
  { "MS", "ambiguous", "mississippi" , NULL, NULL },
  { "Miss", "ambiguous", "mississippi" , NULL, NULL },
  { "MT", "ambiguous", "montanna" , NULL, NULL },
  { "Mt", "ambiguous", "montanna" , NULL, NULL },
  { "MO", "ambiguous", "missouri" , NULL, NULL },
  { "Mo", "ambiguous", "missouri" , NULL, NULL },
  { "NC", "ambiguous", "north" , "carolina", NULL },
  { "ND", "ambiguous", "north" , "dakota", NULL },
  { "NE", "ambiguous", "nebraska" , NULL, NULL },
  { "Ne", "ambiguous", "nebraska" , NULL, NULL },
  { "Neb", "ambiguous", "nebraska" , NULL, NULL },
  { "NH", "ambiguous", "new" , "hampshire", NULL },
  { "NV", "", "nevada" , NULL, NULL },
  { "Nev", "", "nevada" , NULL, NULL },
  { "NY", "", "new" , "york", NULL },
  { "OH", "ambiguous", "ohio" , NULL, NULL },
  { "OK", "ambiguous", "oklahoma" , NULL, NULL },
  { "Okla", "", "oklahoma" , NULL, NULL },
  { "OR", "ambiguous", "oregon" , NULL, NULL },
  { "Or", "ambiguous", "oregon" , NULL, NULL },
  { "Ore", "ambiguous", "oregon" , NULL, NULL },
  { "PA", "ambiguous", "pennsylvania" , NULL, NULL },
  { "Pa", "ambiguous", "pennsylvania" , NULL, NULL },
  { "Penn", "ambiguous", "pennsylvania" , NULL, NULL },
  { "RI", "ambiguous", "rhode" , "island", NULL },
  { "SC", "ambiguous", "south" , "carlolina", NULL },
  { "SD", "ambiguous", "south" , "dakota", NULL },
  { "TN", "ambiguous", "tennesee" , NULL, NULL },
  { "Tn", "ambiguous", "tennesee" , NULL, NULL },
  { "Tenn", "ambiguous", "tennesee" , NULL, NULL },
  { "TX", "ambiguous", "texas" , NULL, NULL },
  { "Tx", "ambiguous", "texas" , NULL, NULL },
  { "Tex", "ambiguous", "texas" , NULL, NULL },
  { "UT", "ambiguous", "utah" , NULL, NULL },
  { "VA", "ambiguous", "virginia" , NULL, NULL },
  { "WA", "ambiguous", "washington" , NULL, NULL },
  { "Wa", "ambiguous", "washington" , NULL, NULL },
  { "Wash", "ambiguous", "washington" , NULL, NULL },
  { "WI", "ambiguous", "wisconsin" , NULL, NULL },
  { "Wi", "ambiguous", "wisconsin" , NULL, NULL },
  { "WV", "ambiguous", "west" , "virginia", NULL },
  { "WY", "ambiguous", "wyoming" , NULL, NULL },
  { "Wy", "ambiguous", "wyoming" , NULL, NULL },
  { "Wyo", "", "wyoming" , NULL, NULL },
  { "PR", "ambiguous", "puerto" , "rico", NULL },
  { NULL, NULL, "puerto" , "rico", NULL }
};

static cst_val *state_name(const char *name,cst_item *t)
{
    int s,j;
    int do_it = 0;
    cst_val *r = 0;

    for (s=0; states[s][0]; s++)
    {
	if (cst_streq(states[s][0],name))
	{
	    if (cst_streq(states[s][1],"ambiguous"))
	    {
		const char *pname = ffeature_string(t,"p.name");
		const char *nname = ffeature_string(t,"n.name");
		    /* previous name is capitalized */
		if (((strchr("ABCDEFGHIJKLMNOPQRSTUVWXYZ",pname[0])) &&
		     (cst_strlen(pname) > 2) &&
		     (cst_regex_match(cst_rx_alpha,pname))) &&
		    ((strchr("abcdefghijklmnopqrstuvwxyz",nname[0])) ||
		     (item_next(t) == 0) ||
		     (cst_streq(".",item_feat_string(t,"punc"))) ||
		     (((cst_strlen(nname) == 5 || (cst_strlen(nname) == 10)) &&
		       cst_regex_match(cst_rx_digits,nname)))))
		    do_it = 1;
		else
		    do_it = 0;
	    }
	    else
		do_it = 1;

	    if (do_it)
	    {
		for (j=2; states[s][j]; j++)
		    r = cons_val(string_val(states[s][j]),r);
		return val_reverse(r);
	    }
	}
    }
    return r;

}



