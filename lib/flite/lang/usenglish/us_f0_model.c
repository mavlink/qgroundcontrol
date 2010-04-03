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
/*  An F0 model                                                          */
/*    This is derived fromthe f2b model freely distributed in Festival   */
/*                                                                       */
/*************************************************************************/

#include "cst_hrg.h"
#include "cst_phoneset.h"
#include "us_f0.h"

static void apply_lr_model(cst_item *s,
			   const us_f0_lr_term *f0_lr_terms,
			   float *start,
			   float *mid,
			   float *end)
{
    int i;
    const cst_val *v=0;
    float fv;

    /* Interceptors */
    *start = f0_lr_terms[0].start;
    *mid = f0_lr_terms[0].mid;
    *end = f0_lr_terms[0].end;
    for (i=1; f0_lr_terms[i].feature; i++)
    {
	if (!cst_streq(f0_lr_terms[i].feature,f0_lr_terms[i-1].feature))
	    v = ffeature(s,f0_lr_terms[i].feature);
	if (f0_lr_terms[i].type)
	{
	    if (cst_streq(val_string(v),f0_lr_terms[i].type))
		fv = 1.0;
	    else
		fv = 0.0;
	}
	else
	    fv = val_float(v);
	(*start) += fv*f0_lr_terms[i].start;
	(*mid) += fv*f0_lr_terms[i].mid;
	(*end) += fv*f0_lr_terms[i].end;
/*	printf("f %s start %f mid %f end %f\n",
	       f0_lr_terms[i].feature,
	       *start,*mid,*end);  */
    }
}

static void add_target_point(cst_relation *targ,float pos, float f0)
{
    cst_item *t;

/*    printf("target %f at %f\n",f0,pos); */
    t = relation_append(targ,NULL);
    item_set_float(t,"pos",pos);
    /* them there can sometimes do silly things, so guard for that */
    if (f0 > 500.0)
	item_set_float(t,"f0",500.0);
    else if (f0 < 50.0)
	item_set_float(t,"f0",50.0);
    else
	item_set_float(t,"f0",f0);
}

/* model mean and stddev take from f2b/kal_diphone */
#define model_mean 170.0
#define model_stddev 34
#define map_f0(v,m,s) ((((v-model_mean)/model_stddev)*s)+m)

static int post_break(cst_item *syl)
{
    if ((item_prev(syl) == 0) ||
	(cst_streq("pau",
		   ffeature_string(syl,
				   "R:SylStructure.daughter.R:Segment.p.name"))))
	return TRUE;
    else
	return FALSE;
}

static int pre_break(cst_item *syl)
{
    if ((item_next(syl) == 0) ||
	(cst_streq("pau",
		   ffeature_string(syl,
				   "R:SylStructure.daughtern.R:Segment.n.name"))))
	return TRUE;
    else
	return FALSE;
}

static float vowel_mid(cst_item *syl)
{
    /* return time point mid way in vowel in this syl */
    cst_item *s;
    cst_item *ts;
    const cst_phoneset *ps = item_phoneset(syl);

    ts = item_daughter(item_as(syl,"SylStructure"));
    for (s=ts; s; s = item_next(s))
    {
	if (cst_streq("+", phone_feature_string(ps,item_feat_string(s,"name"),
						"vc")))
	{
	    return (item_feat_float(s,"end")+
		    ffeature_float(s,"R:Segment.p.end"))/2.0;
	}
    }

    /* no segments, shouldn't happen */
    if (ts == 0)
	return 0;

    /* no vowel in syllable, shouldn't happen */
    return (item_feat_float(ts,"end")+
	    ffeature_float(ts,"R:Segment.p.end"))/2.0;
}

cst_utterance *us_f0_model(cst_utterance *u)
{
    /* F0 target model: Black and Hunt ICSLP96, three points per syl  */
    cst_item *syl, *t, *nt;
    cst_relation *targ_rel;
    float mean, stddev, local_mean, local_stddev;
    float start, mid, end, lend;
    float seg_end;

    if (feat_present(u->features,"no_f0_target_model"))
        return u;

    targ_rel = utt_relation_create(u,"Target");
    mean = get_param_float(u->features,"int_f0_target_mean", 100.0);
    mean *= get_param_float(u->features,"f0_shift", 1.0);
    stddev = get_param_float(u->features,"int_f0_target_stddev", 12.0);
    
    lend = 0;
    for (syl=relation_head(utt_relation(u,"Syllable"));
	 syl;
	 syl = item_next(syl))

    {
/*	printf("word %s, accent %s endtone %s\n",
	       ffeature_string(syl,"R:SylStructure.parent.name"),
	       ffeature_string(syl,"accent"),
	       ffeature_string(syl,"endtone")); */
	if (!item_daughter(item_as(syl,"SylStructure")))
	    continue;  /* no segs in syl */

	local_mean = ffeature_float(syl,"R:SylStructure.parent.R:Token.parent.local_f0_shift");
	if (local_mean)
		local_mean *= mean;
	else
		local_mean = mean;
	local_stddev = ffeature_float(syl,"R:SylStructure.parent.R:Token.parent.local_f0_range");
	if (local_stddev == 0.0)
		local_stddev = stddev;

	apply_lr_model(syl,f0_lr_terms,&start,&mid,&end);
	if (post_break(syl))
	    lend = map_f0(start,local_mean,local_stddev);
	add_target_point(targ_rel,
			 ffeature_float(syl,
				"R:SylStructure.daughter.R:Segment.p.end"),
			 map_f0((start+lend)/2.0,local_mean,local_stddev));
	add_target_point(targ_rel,
			 vowel_mid(syl),
			 map_f0(mid,local_mean,local_stddev));
	lend = map_f0(end,local_mean,local_stddev);
	if (pre_break(syl))
	    add_target_point(targ_rel,
			  ffeature_float(syl,"R:SylStructure.daughtern.end"),
			     map_f0(end,local_mean,local_stddev));
    }
    
    /* Guarantee targets go from start to end of utterance */
    t = relation_head(targ_rel);
    if (t == 0)
	add_target_point(targ_rel,0,mean);
    else if (item_feat_float(t,"pos") > 0)
    {
	nt = item_prepend(t,NULL);
	item_set_float(nt,"pos",0.0);
	item_set_float(nt,"f0",item_feat_float(t,"f0"));
    }
	
    t = relation_tail(targ_rel);
    seg_end = item_feat_float(relation_tail(utt_relation(u,"Segment")),"end");
    if (item_feat_float(t,"pos") < seg_end)
	add_target_point(targ_rel,seg_end,item_feat_float(t,"f0"));

    return u;
}
