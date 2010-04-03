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
/*  Poor mans part of speech tagger                                      */
/*************************************************************************/

#include "cst_val.h"

DEF_STATIC_CONST_VAL_STRING(gpos_in,"in");
DEF_STATIC_CONST_VAL_STRING(gpos_of,"of");
DEF_STATIC_CONST_VAL_STRING(gpos_for,"for");
DEF_STATIC_CONST_VAL_STRING(gpos_on,"on");
DEF_STATIC_CONST_VAL_STRING(gpos_that,"that");
DEF_STATIC_CONST_VAL_STRING(gpos_with,"with");
DEF_STATIC_CONST_VAL_STRING(gpos_by,"by");
DEF_STATIC_CONST_VAL_STRING(gpos_at,"at");
DEF_STATIC_CONST_VAL_STRING(gpos_from,"from");
DEF_STATIC_CONST_VAL_STRING(gpos_as,"as");
DEF_STATIC_CONST_VAL_STRING(gpos_if,"if");
DEF_STATIC_CONST_VAL_STRING(gpos_against,"against");
DEF_STATIC_CONST_VAL_STRING(gpos_about,"about");
DEF_STATIC_CONST_VAL_STRING(gpos_before,"before");
DEF_STATIC_CONST_VAL_STRING(gpos_because,"because");
DEF_STATIC_CONST_VAL_STRING(gpos_under,"under");
DEF_STATIC_CONST_VAL_STRING(gpos_after,"after");
DEF_STATIC_CONST_VAL_STRING(gpos_over,"over");
DEF_STATIC_CONST_VAL_STRING(gpos_into,"into");
DEF_STATIC_CONST_VAL_STRING(gpos_while,"while");
DEF_STATIC_CONST_VAL_STRING(gpos_without,"without");
DEF_STATIC_CONST_VAL_STRING(gpos_through,"through");
DEF_STATIC_CONST_VAL_STRING(gpos_new,"new"); /* ??? */
DEF_STATIC_CONST_VAL_STRING(gpos_between,"between");
DEF_STATIC_CONST_VAL_STRING(gpos_among,"among");
DEF_STATIC_CONST_VAL_STRING(gpos_until,"until");
DEF_STATIC_CONST_VAL_STRING(gpos_per,"per");
DEF_STATIC_CONST_VAL_STRING(gpos_up,"up");
DEF_STATIC_CONST_VAL_STRING(gpos_down,"down");

static const cst_val * const gpos_in_list[] = {
    (cst_val *)&gpos_in,
    (cst_val *)&gpos_of,
    (cst_val *)&gpos_for,
    (cst_val *)&gpos_in,
    (cst_val *)&gpos_on,
    (cst_val *)&gpos_that,
    (cst_val *)&gpos_with,
    (cst_val *)&gpos_by,
    (cst_val *)&gpos_at,
    (cst_val *)&gpos_from,
    (cst_val *)&gpos_as,
    (cst_val *)&gpos_if,
    (cst_val *)&gpos_that,
    (cst_val *)&gpos_against,
    (cst_val *)&gpos_about,
    (cst_val *)&gpos_before,
    (cst_val *)&gpos_because,
    (cst_val *)&gpos_under,
    (cst_val *)&gpos_after,
    (cst_val *)&gpos_over,
    (cst_val *)&gpos_into,
    (cst_val *)&gpos_while,
    (cst_val *)&gpos_without,
    (cst_val *)&gpos_through,
    (cst_val *)&gpos_new,
    (cst_val *)&gpos_between,
    (cst_val *)&gpos_among,
    (cst_val *)&gpos_until,
    (cst_val *)&gpos_per,
    (cst_val *)&gpos_up,
    (cst_val *)&gpos_down,
    0 };

DEF_STATIC_CONST_VAL_STRING(gpos_to,"to");

static const cst_val * const gpos_to_list[] = {
    (cst_val *)&gpos_to,
    (cst_val *)&gpos_to,
    0 };

DEF_STATIC_CONST_VAL_STRING(gpos_det,"det");
DEF_STATIC_CONST_VAL_STRING(gpos_the,"the");
DEF_STATIC_CONST_VAL_STRING(gpos_a,"a");
DEF_STATIC_CONST_VAL_STRING(gpos_an,"an");
DEF_STATIC_CONST_VAL_STRING(gpos_some,"some");
DEF_STATIC_CONST_VAL_STRING(gpos_this,"this");
DEF_STATIC_CONST_VAL_STRING(gpos_each,"each");
DEF_STATIC_CONST_VAL_STRING(gpos_another,"another");
DEF_STATIC_CONST_VAL_STRING(gpos_those,"those");
DEF_STATIC_CONST_VAL_STRING(gpos_every,"every");
DEF_STATIC_CONST_VAL_STRING(gpos_all,"all");
DEF_STATIC_CONST_VAL_STRING(gpos_any,"any");
DEF_STATIC_CONST_VAL_STRING(gpos_these,"these");
DEF_STATIC_CONST_VAL_STRING(gpos_both,"both");
DEF_STATIC_CONST_VAL_STRING(gpos_neither,"neither");
DEF_STATIC_CONST_VAL_STRING(gpos_no,"no");
DEF_STATIC_CONST_VAL_STRING(gpos_many,"many");

static const cst_val * const gpos_det_list[] = {
    (cst_val *)&gpos_det,
    (cst_val *)&gpos_the,
    (cst_val *)&gpos_a,
    (cst_val *)&gpos_an,
    (cst_val *)&gpos_no,
    (cst_val *)&gpos_some,
    (cst_val *)&gpos_this,
    (cst_val *)&gpos_each,
    (cst_val *)&gpos_another,
    (cst_val *)&gpos_those,
    (cst_val *)&gpos_every,
    (cst_val *)&gpos_all,
    (cst_val *)&gpos_any,
    (cst_val *)&gpos_these,
    (cst_val *)&gpos_both,
    (cst_val *)&gpos_neither,
    (cst_val *)&gpos_no,
    (cst_val *)&gpos_many,
    0 };

DEF_STATIC_CONST_VAL_STRING(gpos_md,"md");
DEF_STATIC_CONST_VAL_STRING(gpos_will,"will");
DEF_STATIC_CONST_VAL_STRING(gpos_may,"may");
DEF_STATIC_CONST_VAL_STRING(gpos_would,"would");
DEF_STATIC_CONST_VAL_STRING(gpos_can,"can");
DEF_STATIC_CONST_VAL_STRING(gpos_could,"could");
DEF_STATIC_CONST_VAL_STRING(gpos_should,"should");
DEF_STATIC_CONST_VAL_STRING(gpos_must,"must");
DEF_STATIC_CONST_VAL_STRING(gpos_ought,"ought");
DEF_STATIC_CONST_VAL_STRING(gpos_might,"might");

static const cst_val * const gpos_md_list[] = {
    (cst_val *)&gpos_md,
    (cst_val *)&gpos_will,
    (cst_val *)&gpos_may,
    (cst_val *)&gpos_would,
    (cst_val *)&gpos_can,
    (cst_val *)&gpos_could,
    (cst_val *)&gpos_should,
    (cst_val *)&gpos_must,
    (cst_val *)&gpos_ought,
    (cst_val *)&gpos_might,
    0 };

DEF_STATIC_CONST_VAL_STRING(gpos_cc,"cc");
DEF_STATIC_CONST_VAL_STRING(gpos_and,"and");
DEF_STATIC_CONST_VAL_STRING(gpos_but,"but");
DEF_STATIC_CONST_VAL_STRING(gpos_or,"or");
DEF_STATIC_CONST_VAL_STRING(gpos_plus,"plus");
DEF_STATIC_CONST_VAL_STRING(gpos_yet,"yet");
DEF_STATIC_CONST_VAL_STRING(gpos_nor,"nor");

static const cst_val * const gpos_cc_list[] = {
    (cst_val *)&gpos_cc,
    (cst_val *)&gpos_and,
    (cst_val *)&gpos_but,
    (cst_val *)&gpos_or,
    (cst_val *)&gpos_plus,
    (cst_val *)&gpos_yet,
    (cst_val *)&gpos_nor,
    0 };

DEF_STATIC_CONST_VAL_STRING(gpos_wp,"wp");
DEF_STATIC_CONST_VAL_STRING(gpos_who,"who");
DEF_STATIC_CONST_VAL_STRING(gpos_what,"what");
DEF_STATIC_CONST_VAL_STRING(gpos_where,"where");
DEF_STATIC_CONST_VAL_STRING(gpos_how,"how");
DEF_STATIC_CONST_VAL_STRING(gpos_when,"when");

static const cst_val * const gpos_wp_list[] = {
    (cst_val *)&gpos_wp,
    (cst_val *)&gpos_who,
    (cst_val *)&gpos_what,
    (cst_val *)&gpos_where,
    (cst_val *)&gpos_how,
    (cst_val *)&gpos_when,
    0 };

DEF_STATIC_CONST_VAL_STRING(gpos_pps,"pps");
DEF_STATIC_CONST_VAL_STRING(gpos_her,"her");
DEF_STATIC_CONST_VAL_STRING(gpos_his,"his");
DEF_STATIC_CONST_VAL_STRING(gpos_their,"their");
DEF_STATIC_CONST_VAL_STRING(gpos_its,"its");
DEF_STATIC_CONST_VAL_STRING(gpos_our,"our");
DEF_STATIC_CONST_VAL_STRING(gpos_mine,"mine");

static const cst_val * const gpos_pps_list[] = {
    (cst_val *)&gpos_pps,
    (cst_val *)&gpos_her,
    (cst_val *)&gpos_his,
    (cst_val *)&gpos_their,
    (cst_val *)&gpos_its,
    (cst_val *)&gpos_our,
    (cst_val *)&gpos_mine,
    0 };

DEF_STATIC_CONST_VAL_STRING(gpos_aux,"aux");
DEF_STATIC_CONST_VAL_STRING(gpos_is,"is");
DEF_STATIC_CONST_VAL_STRING(gpos_am,"am");
DEF_STATIC_CONST_VAL_STRING(gpos_are,"are");
DEF_STATIC_CONST_VAL_STRING(gpos_was,"was");
DEF_STATIC_CONST_VAL_STRING(gpos_were,"were");
DEF_STATIC_CONST_VAL_STRING(gpos_has,"has");
DEF_STATIC_CONST_VAL_STRING(gpos_have,"have");
DEF_STATIC_CONST_VAL_STRING(gpos_had,"had");
DEF_STATIC_CONST_VAL_STRING(gpos_be,"be");

static const cst_val * const gpos_aux_list[] = {
    (cst_val *)&gpos_aux,
    (cst_val *)&gpos_is,
    (cst_val *)&gpos_am,
    (cst_val *)&gpos_are,
    (cst_val *)&gpos_was,
    (cst_val *)&gpos_were,
    (cst_val *)&gpos_has,
    (cst_val *)&gpos_have,
    (cst_val *)&gpos_had,
    (cst_val *)&gpos_be,
    0 };

DEF_STATIC_CONST_VAL_STRING(gpos_punc,"punc");
DEF_STATIC_CONST_VAL_STRING(gpos_dot,".");
DEF_STATIC_CONST_VAL_STRING(gpos_comma,",");
DEF_STATIC_CONST_VAL_STRING(gpos_colon,":");
DEF_STATIC_CONST_VAL_STRING(gpos_semicolon,";");
DEF_STATIC_CONST_VAL_STRING(gpos_dquote,"\"");
DEF_STATIC_CONST_VAL_STRING(gpos_squote,"'");
DEF_STATIC_CONST_VAL_STRING(gpos_leftparen,"(");
DEF_STATIC_CONST_VAL_STRING(gpos_qmark,"?");
DEF_STATIC_CONST_VAL_STRING(gpos_rightparen,")");
DEF_STATIC_CONST_VAL_STRING(gpos_emark,"!");

static const cst_val * const gpos_punc_list[] = {
    (cst_val *)&gpos_punc,
    (cst_val *)&gpos_dot,
    (cst_val *)&gpos_comma,
    (cst_val *)&gpos_colon,
    (cst_val *)&gpos_semicolon,
    (cst_val *)&gpos_dquote,
    (cst_val *)&gpos_squote,
    (cst_val *)&gpos_leftparen,
    (cst_val *)&gpos_qmark,
    (cst_val *)&gpos_rightparen,
    (cst_val *)&gpos_emark,
    0 };

const cst_val * const * const us_gpos[] = {
    gpos_in_list,
    gpos_to_list,
    gpos_det_list,
    gpos_md_list,
    gpos_cc_list,
    gpos_wp_list,
    gpos_pps_list,
    gpos_aux_list,
    gpos_punc_list,
    0 };

    
	
