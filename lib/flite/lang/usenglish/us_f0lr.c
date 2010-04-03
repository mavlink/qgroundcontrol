/*************************************************************************/
/*                                                                       */
/*                   Carnegie Mellon University and                      */
/*                Centre for Speech Technology Research                  */
/*                     University of Edinburgh, UK                       */
/*                       Copyright (c) 1998-2001                         */
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
/*  THE UNIVERSITY OF EDINBURGH, CARNEGIE MELLON UNIVERSITY AND THE      */
/*  CONTRIBUTORS TO THIS WORK DISCLAIM ALL WARRANTIES WITH REGARD TO     */
/*  THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY   */
/*  AND FITNESS, IN NO EVENT SHALL THE UNIVERSITY OF EDINBURGH, CARNEGIE */
/*  MELLON UNIVERSITY NOR THE CONTRIBUTORS BE LIABLE FOR ANY SPECIAL,    */
/*  INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER          */
/*  RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN  AN ACTION   */
/*  OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF     */
/*  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.       */
/*                                                                       */
/*************************************************************************/
/*             Author:  Alan W Black (awb@cs.cmu.edu)                    */
/*               Date:  January 2001                                     */
/*************************************************************************/
/*                                                                       */
/*  US F0 model LR features                                              */
/*                                                                       */
/*  Derived directly from the F0 LR model in University of Edinburgh's   */
/*  Festival Speech Synthesis Systems festival/lib/f2bf0lr.scm which in  */
/*  turn was trained from Boston University FM Radio Data Corpus         */
/*                                                                       */
/*************************************************************************/

#include "us_f0.h"

const us_f0_lr_term f0_lr_terms[] = {
    { "Intercept", 160.584961, 169.183380, 169.570374, 0 },
    { "p.p.accent", 10.081770, 4.923247, 3.594771, "H*" },
    { "p.p.accent", 3.358613, 0.955474, 0.432519, "!H*" },
    { "p.p.accent", 4.144342, 1.193597, 0.235664, "L+H*" },
    { "p.accent", 32.081028, 16.603350, 11.214208, "H*" },
    { "p.accent", 18.090033, 11.665814, 9.619350, "!H*" },
    { "p.accent", 23.255280, 13.063298, 9.084690, "L+H*" },
    { "accent", 5.221081, 34.517868, 25.217588, "H*" },
    { "accent", 10.159194, 22.349655, 13.759851, "!H*" },
    { "accent", 3.645511, 23.551548, 17.635193, "L+H*" },
    { "n.accent", -5.691933, -1.914945, 4.944848, "H*" },
    { "n.accent", 8.265606, 5.249441, 7.398383, "!H*" },
    { "n.accent", 0.861427, -1.929947, 1.683011, "L+H*" },
    { "n.n.accent", -3.785701, -6.147251, -4.335797, "H*" },
    { "n.n.accent", 7.013446, 8.408949, 5.656462, "!H*" },
    { "n.n.accent", 2.637494, 3.193500, 0.263288, "L+H*" },
    { "p.p.endtone", -3.531153, 4.255273, 10.274958, "L-L%" },
    { "p.p.endtone", 8.258756, 6.989573, 10.446935, "L-H%" },
    { "p.p.endtone", 5.836487, 2.598854, 6.104384, "H-" },
    { "p.p.endtone", 11.213440, 12.178307, 14.182688, "H-H%" },
    { "R:Syllable.p.endtone", -28.081360, -4.397973, 1.767454, "L-L%" },
    { "R:Syllable.p.endtone", -6.585836, 6.938086, 8.750018, "L-H%" },
    { "R:Syllable.p.endtone", 8.537044, 6.162763, 5.000340, "H-" },
    { "R:Syllable.p.endtone", 4.243342, 8.035727, 10.913437, "H-H%" },
    { "endtone", -9.333926, -19.357903, -12.637935, "L-L%" },
    { "endtone", -0.937483, -7.328882, 8.747483, "L-H%" },
    { "endtone", 9.472265, 12.694193, 15.165833, "H-" },
    { "endtone", 14.256898, 30.923397, 50.190327, "H-H%" },
    { "n.endtone", -13.084253, -17.727785, -16.965780, "L-L%" },
    { "n.endtone", -5.471592, -8.701685, -7.833168, "L-H%" },
    { "n.endtone", -0.095669, -1.006439, 4.701087, "H-" },
    { "n.endtone", 4.933708, 6.834498, 10.349902, "H-H%" },
    { "n.n.endtone", -14.993470, -15.407530, -15.369483, "L-L%" },
    { "n.n.endtone", -11.352400, -7.621437, -7.052374, "L-H%" },
    { "n.n.endtone", -5.551627, -0.458837, 2.207854, "H-" },
    { "n.n.endtone", -0.661581, 3.170632, 5.271546, "H-H%" },
    { "p.p.old_syl_break", -3.367677, -4.196950, -4.745862, 0 },
    { "p.old_syl_break", 0.641755, -5.176929, -5.685178, 0 },
    { "old_syl_break", -0.659002, 0.047922, -2.633291, 0 },
    { "n.old_syl_break", 1.217358, 2.153968, 1.678340, 0 },
    { "n.n.old_syl_break", 2.974502, 2.577074, 2.274729, 0 },
    { "p.p.stress", 1.588098, -2.368192, -2.747198, 0 },
    { "p.stress", 3.693430, 1.080493, 0.306724, 0 },
    { "stress", 2.009843, 1.135556, -0.565613, 0 },
    { "n.stress", 1.645560, 2.447219, 2.838327, 0 },
    { "n.n.stress", 1.926870, 1.318122, 1.285244, 0 },
    { "syl_in", 1.048362, 0.291663, 0.169955, 0 },
    { "syl_out", 0.315553, -0.411814, -1.045661, 0 },
    { "ssyl_in", -2.096079, -1.643456, -1.487774, 0 },
    { "ssyl_out", 0.303531, 0.580589, 0.752405, 0 },
    { "asyl_in", -4.257915, -5.649243, -5.081677, 0 },
    { "asyl_out", -2.422424, 0.489823, 3.016218, 0 },
    { "last_accent", -0.397647, 0.216634, 0.312900, 0 },
    { "next_accent", -0.418613, 0.244134, 0.837992, 0 },
    { "sub_phrases", -5.472055, -5.758156, -5.397805, 0 },
    { 0, 0, 0, 0, 0 }
};

