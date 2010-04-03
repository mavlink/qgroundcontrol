/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 1999                             */
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
/*  Phoneset functions                                                   */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_PHONESET_H__
#define _CST_PHONESET_H__

#include "cst_file.h"
#include "cst_val.h"
#include "cst_features.h"
#include "cst_item.h"

struct cst_phoneset_struct {
    const char *name;
    const char * const * featnames;
    const cst_val * const *featvals;
    const char * const * phonenames;
    const char *silence;
    const int num_phones;
    const int * const * fvtable;
};
typedef struct cst_phoneset_struct cst_phoneset;

/* Constructor functions */
cst_phoneset *new_phoneset();
void delete_phoneset(cst_phoneset *u);

const cst_val *phone_feature(const cst_phoneset *ps,
			     const char* phonename,
			     const char *featname);
const char *phone_feature_string(const cst_phoneset *ps,
				 const char* phonename,
				 const char *featname);
int phone_id(const cst_phoneset *ps,const char* phonename);
int phone_feat_id(const cst_phoneset *ps,const char* featname);

const cst_phoneset *item_phoneset(const cst_item *i);

CST_VAL_USER_TYPE_DCLS(phoneset,cst_phoneset)

#endif
