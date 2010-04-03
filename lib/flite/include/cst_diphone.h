/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                        Copyright (c) 2001                             */
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
/*  Diphone databases                                                    */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_DIPHONE_H__
#define _CST_DIPHONE_H__

#include "cst_file.h"
#include "cst_val.h"
#include "cst_features.h"
#include "cst_wave.h"
#include "cst_track.h"
#include "cst_sts.h"
#include "cst_hrg.h"

struct cst_diphone_entry_struct {
    char *name;
    unsigned short start_pm;
    unsigned char pb_pm;
    unsigned char end_pm;
};
typedef struct cst_diphone_entry_struct cst_diphone_entry;

struct cst_diphone_db_struct {
    const char *name;
    int  num_entries;
    const cst_diphone_entry *diphones;
    const cst_sts_list *sts;
};
typedef struct cst_diphone_db_struct cst_diphone_db;

CST_VAL_USER_TYPE_DCLS(diphone_db,cst_diphone_db)

cst_utterance* diphone_synth(cst_utterance *utt);
cst_utterance *get_diphone_units(cst_utterance *utt);

#endif
