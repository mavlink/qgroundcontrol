/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2000                             */
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
/*  clunits db                                                           */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_CLUNITS_H__
#define _CST_CLUNITS_H__

#include "cst_wave.h"
#include "cst_cart.h"
#include "cst_viterbi.h"
#include "cst_track.h"
#include "cst_sts.h"

#define CLUNIT_NONE (unsigned short)65535

typedef struct cst_clunit_struct {
    unsigned short type, phone;
    int start, end;
    int prev, next;
} cst_clunit;

typedef struct cst_clunit_type_struct {
    const char *name;
    int start, count;
} cst_clunit_type;

typedef struct cst_clunit_db_struct {
    const char *name;
    const cst_clunit_type *types;
    const cst_cart * const *trees; 
    const cst_clunit *units;
    int num_types, num_units;

    /* These may be set up at runtime (in file-mapped databases) */
    cst_sts_list *sts, *mcep;

    /* These are pre-scaled by 65536 to accomodate fixed-point machines */
    const int *join_weights;

    /* Misc. important parameters */
    int optimal_coupling;
    int extend_selections;
    int f0_weight;
    char *(*unit_name_func)(cst_item *s);
} cst_clunit_db;

CST_VAL_USER_TYPE_DCLS(clunit_db,cst_clunit_db)
CST_VAL_USER_TYPE_DCLS(vit_cand,cst_vit_cand)

cst_utterance *clunits_synth(cst_utterance *utt);
cst_utterance *clunits_dump_units(cst_utterance *utt);

char *clunits_ldom_phone_word(cst_item *s);
int clunit_get_unit_index(cst_clunit_db *cludb,
			  const char *unit_type,
			  int instance);
int clunit_get_unit_index_name(cst_clunit_db *cludb,
			       const char *name);

#define UNIT_TYPE(db,u) ((db)->types[(db)->units[(u)].type].name)
#define UNIT_INDEX(db,u) ((u) - (db)->types[(db)->units[(u)].type].start)

#endif
