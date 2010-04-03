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
/*               Date:  January 2000                                     */
/*************************************************************************/
/*                                                                       */
/*  CART tree support                                                    */
/*                                                                       */
/*************************************************************************/
#ifndef _CST_CART_H__
#define _CST_CART_H__

#include "cst_file.h"
#include "cst_val.h"
#include "cst_features.h"
#include "cst_item.h"
#include "cst_relation.h"

#define CST_CART_OP_NONE    255
#define CST_CART_OP_LEAF    255
#define CST_CART_OP_IS      0
#define CST_CART_OP_IN      1
#define CST_CART_OP_LESS    2
#define CST_CART_OP_GREATER 3
#define CST_CART_OP_MATCHES 4
#define CST_CART_OP_EQUALS  5

typedef struct cst_cart_node_struct {
    unsigned char feat;
    unsigned char op;
    /* yes is always the next node */
    unsigned short no_node;  /* or answer index */
    const cst_val *val;  
} cst_cart_node;

typedef struct cst_cart_struct {
    const cst_cart_node *rule_table;
    const char * const *feat_table;
} cst_cart;

void delete_cart(cst_cart *c);

CST_VAL_USER_TYPE_DCLS(cart,cst_cart)

const cst_val *cart_interpret(cst_item *item, const cst_cart *tree);

#endif
