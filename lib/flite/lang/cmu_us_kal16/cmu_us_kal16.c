/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 1999-2000                          */
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
/*               Date:  December 2000                                    */
/*************************************************************************/
/*                                                                       */
/*  A simple voice defintion                                             */
/*                                                                       */
/*************************************************************************/

#include "flite.h"
#include "cst_diphone.h"
#include "usenglish.h"
#include "cmu_lex.h"

static cst_utterance *cmu_us_kal16_postlex(cst_utterance *u);
extern cst_diphone_db cmu_us_kal16_db;

cst_voice *cmu_us_kal16_diphone = NULL;

cst_voice *register_cmu_us_kal16(const char *voxdir)
{
    cst_voice *v;
    cst_lexicon *lex;

    if (cmu_us_kal16_diphone)
        return cmu_us_kal16_diphone;  /* Already registered */

    v = new_voice();
    v->name = "kal16";

    usenglish_init(v);

    /* Set up basic values for synthesizing with this voice */
    flite_feat_set_string(v->features,"name","cmu_us_kal_diphone16");

    /* Lexicon */
    lex = cmu_lex_init();
    flite_feat_set(v->features,"lexicon",lexicon_val(lex));

    /* Intonation */
    flite_feat_set_float(v->features,"int_f0_target_mean",105.0);
    flite_feat_set_float(v->features,"int_f0_target_stddev",14.0);

    /* Post lexical rules */
    flite_feat_set(v->features,"postlex_func",uttfunc_val(&cmu_us_kal16_postlex));

    /* Duration */
    flite_feat_set_float(v->features,"duration_stretch",1.1);

    /* Waveform synthesis: diphone_synth */
    flite_feat_set(v->features,"wave_synth_func",uttfunc_val(&diphone_synth));
    flite_feat_set(v->features,"diphone_db",diphone_db_val(&cmu_us_kal16_db));
    flite_feat_set_int(v->features,"sample_rate",cmu_us_kal16_db.sts->sample_rate);
    flite_feat_set_string(v->features,"resynth_type","fixed");
    flite_feat_set_string(v->features,"join_type","modified_lpc");

    cmu_us_kal16_diphone = v;

    return cmu_us_kal16_diphone;
}

void unregister_cmu_us_kal16(cst_voice *v)
{
    if (v != cmu_us_kal16_diphone)
	return;
    delete_voice(v);
    cmu_us_kal16_diphone = NULL;
}

static void fix_ah(cst_utterance *u)
{
    /* This should really be done in the index itself */
    const cst_item *s;

    for (s=relation_head(utt_relation(u,"Segment")); s; s=item_next(s))
	if (cst_streq(item_feat_string(s,"name"),"ah"))
	    item_set_string(s,"name","aa");
}

static cst_utterance *cmu_us_kal16_postlex(cst_utterance *u)
{

    cmu_postlex(u);
    fix_ah(u);

    return u;
}
