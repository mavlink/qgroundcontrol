/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                         Copyright (c) 2007                            */
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
/*               Date:  November 2007                                    */
/*************************************************************************/
/*  Language independant feature functions                               */
/*************************************************************************/

#ifndef _CST_FFEATURES_H
#define _CST_FFEATURES_H

#include "cst_val.h"
#include "cst_item.h"

const cst_val *ph_vc(const cst_item *p);
const cst_val *ph_vlng(const cst_item *p);
const cst_val *ph_vheight(const cst_item *p);
const cst_val *ph_vrnd(const cst_item *p);
const cst_val *ph_vfront(const cst_item *p);
const cst_val *ph_ctype(const cst_item *p);
const cst_val *ph_cplace(const cst_item *p);
const cst_val *ph_cvox(const cst_item *p);

const cst_val *cg_duration(const cst_item *p);
const cst_val *cg_state_pos(const cst_item *p);
const cst_val *cg_state_place(const cst_item *p);
const cst_val *cg_state_index(const cst_item *p);
const cst_val *cg_state_rindex(const cst_item *p);
const cst_val *cg_phone_place(const cst_item *p);
const cst_val *cg_phone_index(const cst_item *p);
const cst_val *cg_phone_rindex(const cst_item *p);

void basic_ff_register(cst_features *ffunctions);

#endif /* _CST_FFEATURES_H */
