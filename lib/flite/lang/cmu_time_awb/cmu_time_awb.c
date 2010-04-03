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
/*               Date:  April 2001                                       */
/*************************************************************************/
/*                                                                       */
/*  A simple clunits/ldom voice defintion			         */
/*                                                                       */
/*************************************************************************/

#include <string.h>
#include "flite.h"
#include "cst_clunits.h"
#include "usenglish.h"
#include "cmu_lex.h"

int cmu_syl_boundary(const cst_item *i,const cst_val *rest);

static char *cmu_time_awb_unit_name(cst_item *s);

extern cst_clunit_db cmu_time_awb_db;

extern const int cmu_time_awb_lex_entry[];
extern const unsigned char cmu_time_awb_lex_data[];
extern const int cmu_time_awb_num_entries;
extern const int cmu_time_awb_num_bytes;
extern const char * const cmu_time_awb_lex_phone_table[54];
extern const char * const cmu_time_awb_lex_phones_huff_table[];
extern const char * const cmu_time_awb_lex_entries_huff_table[];

cst_lexicon cmu_time_awb_lex;

cst_voice *cmu_time_awb_ldom = NULL;

cst_voice *register_cmu_time_awb(const char *voxdir)
{
    cst_voice *v = new_voice();

    v->name = "awb_time";

    /* Sets up language specific parameters in the voice. */
    usenglish_init(v);

    /* Things that weren't filled in already. */
    flite_feat_set_string(v->features,"name","awb");

    /* Lexicon */
    cmu_time_awb_lex.name = "cmu_time_awb";
    cmu_time_awb_lex.num_entries = cmu_time_awb_num_entries;
    cmu_time_awb_lex.num_bytes = cmu_time_awb_num_bytes;
    cmu_time_awb_lex.data = cmu_time_awb_lex_data;
    cmu_time_awb_lex.phone_table = cmu_time_awb_lex_phone_table;
    cmu_time_awb_lex.syl_boundary = cmu_syl_boundary;
    cmu_time_awb_lex.lts_rule_set = NULL;
    cmu_time_awb_lex.phone_hufftable = cmu_time_awb_lex_phones_huff_table;
    cmu_time_awb_lex.entry_hufftable = cmu_time_awb_lex_entries_huff_table;
    
    flite_feat_set(v->features,"lexicon",lexicon_val(&cmu_time_awb_lex));

    /* Waveform synthesis */
    flite_feat_set(v->features,"wave_synth_func",uttfunc_val(&clunits_synth));
    flite_feat_set(v->features,"clunit_db",clunit_db_val(&cmu_time_awb_db));
    flite_feat_set_int(v->features,"sample_rate",cmu_time_awb_db.sts->sample_rate);
    flite_feat_set_string(v->features,"join_type","simple_join");
    flite_feat_set_string(v->features,"resynth_type","fixed");

    /* Unit selection */
    cmu_time_awb_db.unit_name_func = cmu_time_awb_unit_name;

    cmu_time_awb_ldom = v;

    return cmu_time_awb_ldom;
}

void unregister_cmu_time_awb(cst_voice *vox)
{
    if (vox != cmu_time_awb_ldom)
	return;
    delete_voice(vox);
    cmu_time_awb_ldom = NULL;
}

static char *cmu_time_awb_unit_name(cst_item *s)
{
	return clunits_ldom_phone_word(s);
}
