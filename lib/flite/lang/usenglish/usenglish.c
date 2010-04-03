/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2000                            */
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
/*  Generic models for US English                                        */
/*************************************************************************/

#include "flite.h"
#include "usenglish.h"
#include "us_f0.h"
#include "us_text.h"
#include "us_ffeatures.h"

static const char * const us_english_punctuation = "\"'`.,:;!?(){}[]";
static const char * const us_english_prepunctuation = "\"'`({[";
static const char * const us_english_singlecharsymbols = "";
static const char * const us_english_whitespace = " \t\n\r";

void usenglish_init(cst_voice *v)
{
    us_text_init();

    /* utterance break function */
    feat_set(v->features,"utt_break",breakfunc_val(&default_utt_break));

    /* Phoneset */
    feat_set(v->features,"phoneset",phoneset_val(&us_phoneset));
    feat_set_string(v->features,"silence",us_phoneset.silence);

    /* Text analyser */
    feat_set_string(v->features,"text_whitespace",us_english_whitespace);
    feat_set_string(v->features,"text_postpunctuation",us_english_punctuation);
    feat_set_string(v->features,"text_prepunctuation",
		    us_english_prepunctuation);
    feat_set_string(v->features,"text_singlecharsymbols",
		    us_english_singlecharsymbols);

    feat_set(v->features,"tokentowords_func",itemfunc_val(&us_tokentowords));

    /* very simple POS tagger */
    feat_set(v->features,"pos_tagger_cart",cart_val(&us_pos_cart));

    /* Phrasing */
    feat_set(v->features,"phrasing_cart",cart_val(&us_phrasing_cart));

    /* Intonation */
    feat_set(v->features,"int_cart_accents",cart_val(&us_int_accent_cart));
    feat_set(v->features,"int_cart_tones",cart_val(&us_int_tone_cart));

    /* Duration */
    feat_set(v->features,"dur_cart",cart_val(&us_durz_cart));
    feat_set(v->features,"dur_stats",dur_stats_val((dur_stats *)us_dur_stats));

    /* f0 model */
    feat_set(v->features,"f0_model_func",uttfunc_val(&us_f0_model));

    us_ff_register(v->ffunctions);
}
