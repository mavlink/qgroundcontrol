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
/*  duration stats (means, stddev) from cmu_com_kal                      */
/*************************************************************************/

#include "cst_synth.h"

static const dur_stat dur_stat_pau = { "pau", 0.2, 0.1 }; 
static const dur_stat dur_stat_zh = { "zh", 0.152593, 0.092321 };
static const dur_stat dur_stat_oy = { "oy", 0.160374, 0.077629 };
static const dur_stat dur_stat_aw = { "aw", 0.159485, 0.064687 };
static const dur_stat dur_stat_ch = { "ch", 0.135828, 0.043586 };
static const dur_stat dur_stat_th = { "th", 0.116027, 0.054892 };
static const dur_stat dur_stat_uh = { "uh", 0.061596, 0.023654 };
static const dur_stat dur_stat_g = { "g", 0.077797, 0.027193 };
static const dur_stat dur_stat_ae = { "ae", 0.115669, 0.047921 };
static const dur_stat dur_stat_sh = { "sh", 0.126018, 0.023275 };
static const dur_stat dur_stat_v = { "v", 0.045676, 0.017954 };
static const dur_stat dur_stat_eh = { "eh", 0.109237, 0.046925 };
static const dur_stat dur_stat_w = { "w", 0.052598, 0.024618 };
static const dur_stat dur_stat_ey = { "ey", 0.165883, 0.0757 };
static const dur_stat dur_stat_dh = { "dh", 0.035688, 0.021493 };
static const dur_stat dur_stat_ng = { "ng", 0.065651, 0.022119 };
static const dur_stat dur_stat_uw = { "uw", 0.102018, 0.047394 };
static const dur_stat dur_stat_er = { "er", 0.100174, 0.044822 };
static const dur_stat dur_stat_b = { "b", 0.063457, 0.02702 };
static const dur_stat dur_stat_ah = { "ah", 0.062256, 0.029903 };
static const dur_stat dur_stat_n = { "n", 0.058944, 0.029727 };
static const dur_stat dur_stat_t = { "t", 0.074067, 0.037846 };
static const dur_stat dur_stat_jh = { "jh", 0.083748, 0.029496 };
static const dur_stat dur_stat_ih = { "ih", 0.062962, 0.030609 };
static const dur_stat dur_stat_d = { "d", 0.050917, 0.031666 };
static const dur_stat dur_stat_f = { "f", 0.096548, 0.028515 };
static const dur_stat dur_stat_ao = { "ao", 0.091841, 0.049984 };
static const dur_stat dur_stat_y = { "y", 0.056909, 0.02774 };
static const dur_stat dur_stat_k = { "k", 0.089048, 0.040764 };
static const dur_stat dur_stat_z = { "z", 0.088234, 0.03877 };
static const dur_stat dur_stat_p = { "p", 0.099085, 0.033806 };
static const dur_stat dur_stat_iy = { "iy", 0.126115, 0.063085 };
static const dur_stat dur_stat_r = { "r", 0.052082, 0.023499 };
static const dur_stat dur_stat_aa = { "aa", 0.10923, 0.045992 };
static const dur_stat dur_stat_s = { "s", 0.108565, 0.041973 };
static const dur_stat dur_stat_m = { "m", 0.074447, 0.044589 };
static const dur_stat dur_stat_ay = { "ay", 0.151095, 0.045892 };
static const dur_stat dur_stat_ow = { "ow", 0.146084, 0.052605 };
static const dur_stat dur_stat_l = { "l", 0.065292, 0.033114 };
static const dur_stat dur_stat_ax = { "ax", 0.053852, 0.033216 };
static const dur_stat dur_stat_hh = { "hh", 0.067775, 0.021633 };

const dur_stat * const us_dur_stats[] = {
    &dur_stat_uh,
    &dur_stat_hh,
    &dur_stat_ao,
    &dur_stat_v,
    &dur_stat_ih,
    &dur_stat_ey,
    &dur_stat_jh,
    &dur_stat_w,
    &dur_stat_uw,
    &dur_stat_ae,
    &dur_stat_k,
    &dur_stat_y,
    &dur_stat_l,
    &dur_stat_ng,
    &dur_stat_zh,
    &dur_stat_z,
    &dur_stat_m,
    &dur_stat_iy,
    &dur_stat_n,
    &dur_stat_ah,
    &dur_stat_er,
    &dur_stat_b,
    &dur_stat_pau,
    &dur_stat_aw,
    &dur_stat_p,
    &dur_stat_ch,
    &dur_stat_ow,
    &dur_stat_dh,
    &dur_stat_d,
    &dur_stat_ax,
    &dur_stat_r,
    &dur_stat_eh,
    &dur_stat_ay,
    &dur_stat_oy,
    &dur_stat_f,
    &dur_stat_sh,
    &dur_stat_s,
    &dur_stat_g,
    &dur_stat_th,
    &dur_stat_aa,
    &dur_stat_t,
    NULL
} ;


