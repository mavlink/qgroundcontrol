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
/*               Date:  December 2001                                    */
/*************************************************************************/
/*                                                                       */
/*  Check is symbol is a pronouncable word or not                        */
/*                                                                       */
/*  Uses FSMs for start to Vowel and end back to vowel                   */
/*                                                                       */
/*************************************************************************/
#include <ctype.h>
#include "flite.h"
#include "usenglish.h"
#include "us_text.h"

/* should be cst_fsm.c somewhere */

typedef struct fsm_struct {
    short vocab_size;
    short num_transitions;
    const unsigned short *transitions;
} cst_fsm;

int fsm_transition(const cst_fsm *fsm,int state, int symbol)
{
    int i;

    for (i=state; fsm->transitions[i]; i++)
    {
/*	printf("trans %c %d\n",fsm->transitions[i] % fsm->vocab_size,
	fsm->transitions[i] / fsm->vocab_size); */
	if ((fsm->transitions[i] % fsm->vocab_size) == symbol)
	    return fsm->transitions[i] / fsm->vocab_size;
    }

    return -1;
}

#define fsm_aswdP_state_0 0
#define fsm_aswdP_trans_0 ((fsm_aswdP_state_1 * 128) + 35)
#define fsm_aswdP_trans_1 0
#define fsm_aswdP_state_1 2
#define fsm_aswdP_trans_2 ((fsm_aswdP_state_2 * 128) + 120)
#define fsm_aswdP_trans_3 ((fsm_aswdP_state_2 * 128) + 113)
#define fsm_aswdP_trans_4 ((fsm_aswdP_state_3 * 128) + 122)
#define fsm_aswdP_trans_5 ((fsm_aswdP_state_2 * 128) + 106)
#define fsm_aswdP_trans_6 ((fsm_aswdP_state_4 * 128) + 118)
#define fsm_aswdP_trans_7 ((fsm_aswdP_state_5 * 128) + 107)
#define fsm_aswdP_trans_8 ((fsm_aswdP_state_6 * 128) + 116)
#define fsm_aswdP_trans_9 ((fsm_aswdP_state_7 * 128) + 119)
#define fsm_aswdP_trans_10 ((fsm_aswdP_state_8 * 128) + 102)
#define fsm_aswdP_trans_11 ((fsm_aswdP_state_9 * 128) + 103)
#define fsm_aswdP_trans_12 ((fsm_aswdP_state_10 * 128) + 112)
#define fsm_aswdP_trans_13 ((fsm_aswdP_state_11 * 128) + 108)
#define fsm_aswdP_trans_14 ((fsm_aswdP_state_12 * 128) + 115)
#define fsm_aswdP_trans_15 ((fsm_aswdP_state_13 * 128) + 104)
#define fsm_aswdP_trans_16 ((fsm_aswdP_state_14 * 128) + 114)
#define fsm_aswdP_trans_17 ((fsm_aswdP_state_15 * 128) + 100)
#define fsm_aswdP_trans_18 ((fsm_aswdP_state_16 * 128) + 98)
#define fsm_aswdP_trans_19 ((fsm_aswdP_state_17 * 128) + 99)
#define fsm_aswdP_trans_20 ((fsm_aswdP_state_18 * 128) + 78)
#define fsm_aswdP_trans_21 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_22 0
#define fsm_aswdP_state_2 23
#define fsm_aswdP_trans_23 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_24 0
#define fsm_aswdP_state_3 25
#define fsm_aswdP_trans_25 ((fsm_aswdP_state_2 * 128) + 118)
#define fsm_aswdP_trans_26 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_27 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_28 ((fsm_aswdP_state_20 * 128) + 115)
#define fsm_aswdP_trans_29 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_30 ((fsm_aswdP_state_21 * 128) + 100)
#define fsm_aswdP_trans_31 ((fsm_aswdP_state_2 * 128) + 98)
#define fsm_aswdP_trans_32 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_33 0
#define fsm_aswdP_state_4 34
#define fsm_aswdP_trans_34 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_35 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_36 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_37 0
#define fsm_aswdP_state_5 38
#define fsm_aswdP_trans_38 ((fsm_aswdP_state_2 * 128) + 106)
#define fsm_aswdP_trans_39 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_40 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_41 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_42 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_43 ((fsm_aswdP_state_2 * 128) + 78)
#define fsm_aswdP_trans_44 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_45 0
#define fsm_aswdP_state_6 46
#define fsm_aswdP_trans_46 ((fsm_aswdP_state_2 * 128) + 106)
#define fsm_aswdP_trans_47 ((fsm_aswdP_state_2 * 128) + 118)
#define fsm_aswdP_trans_48 ((fsm_aswdP_state_2 * 128) + 107)
#define fsm_aswdP_trans_49 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_50 ((fsm_aswdP_state_22 * 128) + 115)
#define fsm_aswdP_trans_51 ((fsm_aswdP_state_23 * 128) + 104)
#define fsm_aswdP_trans_52 ((fsm_aswdP_state_24 * 128) + 114)
#define fsm_aswdP_trans_53 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_54 0
#define fsm_aswdP_state_7 55
#define fsm_aswdP_trans_55 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_56 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_57 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_58 0
#define fsm_aswdP_state_8 59
#define fsm_aswdP_trans_59 ((fsm_aswdP_state_2 * 128) + 106)
#define fsm_aswdP_trans_60 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_61 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_62 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_63 0
#define fsm_aswdP_state_9 64
#define fsm_aswdP_trans_64 ((fsm_aswdP_state_2 * 128) + 106)
#define fsm_aswdP_trans_65 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_66 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_67 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_68 ((fsm_aswdP_state_24 * 128) + 114)
#define fsm_aswdP_trans_69 ((fsm_aswdP_state_2 * 128) + 78)
#define fsm_aswdP_trans_70 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_71 0
#define fsm_aswdP_state_10 72
#define fsm_aswdP_trans_72 ((fsm_aswdP_state_2 * 128) + 116)
#define fsm_aswdP_trans_73 ((fsm_aswdP_state_25 * 128) + 102)
#define fsm_aswdP_trans_74 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_75 ((fsm_aswdP_state_2 * 128) + 115)
#define fsm_aswdP_trans_76 ((fsm_aswdP_state_4 * 128) + 104)
#define fsm_aswdP_trans_77 ((fsm_aswdP_state_24 * 128) + 114)
#define fsm_aswdP_trans_78 ((fsm_aswdP_state_2 * 128) + 78)
#define fsm_aswdP_trans_79 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_80 0
#define fsm_aswdP_state_11 81
#define fsm_aswdP_trans_81 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_82 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_83 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_84 0
#define fsm_aswdP_state_12 85
#define fsm_aswdP_trans_85 ((fsm_aswdP_state_2 * 128) + 113)
#define fsm_aswdP_trans_86 ((fsm_aswdP_state_26 * 128) + 122)
#define fsm_aswdP_trans_87 ((fsm_aswdP_state_2 * 128) + 106)
#define fsm_aswdP_trans_88 ((fsm_aswdP_state_2 * 128) + 118)
#define fsm_aswdP_trans_89 ((fsm_aswdP_state_4 * 128) + 107)
#define fsm_aswdP_trans_90 ((fsm_aswdP_state_27 * 128) + 116)
#define fsm_aswdP_trans_91 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_92 ((fsm_aswdP_state_2 * 128) + 102)
#define fsm_aswdP_trans_93 ((fsm_aswdP_state_21 * 128) + 103)
#define fsm_aswdP_trans_94 ((fsm_aswdP_state_28 * 128) + 112)
#define fsm_aswdP_trans_95 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_96 ((fsm_aswdP_state_29 * 128) + 104)
#define fsm_aswdP_trans_97 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_98 ((fsm_aswdP_state_30 * 128) + 99)
#define fsm_aswdP_trans_99 ((fsm_aswdP_state_2 * 128) + 78)
#define fsm_aswdP_trans_100 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_101 0
#define fsm_aswdP_state_13 102
#define fsm_aswdP_trans_102 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_103 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_104 ((fsm_aswdP_state_2 * 128) + 115)
#define fsm_aswdP_trans_105 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_106 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_107 0
#define fsm_aswdP_state_14 108
#define fsm_aswdP_trans_108 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_109 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_110 0
#define fsm_aswdP_state_15 111
#define fsm_aswdP_trans_111 ((fsm_aswdP_state_2 * 128) + 122)
#define fsm_aswdP_trans_112 ((fsm_aswdP_state_2 * 128) + 106)
#define fsm_aswdP_trans_113 ((fsm_aswdP_state_2 * 128) + 118)
#define fsm_aswdP_trans_114 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_115 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_116 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_117 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_118 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_119 0
#define fsm_aswdP_state_16 120
#define fsm_aswdP_trans_120 ((fsm_aswdP_state_2 * 128) + 106)
#define fsm_aswdP_trans_121 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_122 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_123 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_124 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_125 0
#define fsm_aswdP_state_17 126
#define fsm_aswdP_trans_126 ((fsm_aswdP_state_2 * 128) + 122)
#define fsm_aswdP_trans_127 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_128 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_129 ((fsm_aswdP_state_4 * 128) + 104)
#define fsm_aswdP_trans_130 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_131 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_132 0
#define fsm_aswdP_state_18 133
#define fsm_aswdP_trans_133 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_134 ((fsm_aswdP_state_31 * 128) + 99)
#define fsm_aswdP_trans_135 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_136 0
#define fsm_aswdP_state_19 137
#define fsm_aswdP_trans_137 0
#define fsm_aswdP_state_20 138
#define fsm_aswdP_trans_138 ((fsm_aswdP_state_32 * 128) + 99)
#define fsm_aswdP_trans_139 0
#define fsm_aswdP_state_21 140
#define fsm_aswdP_trans_140 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_141 0
#define fsm_aswdP_state_22 142
#define fsm_aswdP_trans_142 ((fsm_aswdP_state_32 * 128) + 99)
#define fsm_aswdP_trans_143 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_144 0
#define fsm_aswdP_state_23 145
#define fsm_aswdP_trans_145 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_146 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_147 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_148 0
#define fsm_aswdP_state_24 149
#define fsm_aswdP_trans_149 ((fsm_aswdP_state_2 * 128) + 122)
#define fsm_aswdP_trans_150 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_151 0
#define fsm_aswdP_state_25 152
#define fsm_aswdP_trans_152 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_153 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_154 0
#define fsm_aswdP_state_26 155
#define fsm_aswdP_trans_155 ((fsm_aswdP_state_33 * 128) + 99)
#define fsm_aswdP_trans_156 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_157 0
#define fsm_aswdP_state_27 158
#define fsm_aswdP_trans_158 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_159 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_160 0
#define fsm_aswdP_state_28 161
#define fsm_aswdP_trans_161 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_162 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_163 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_164 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_165 0
#define fsm_aswdP_state_29 166
#define fsm_aswdP_trans_166 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_167 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_168 ((fsm_aswdP_state_32 * 128) + 99)
#define fsm_aswdP_trans_169 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_170 0
#define fsm_aswdP_state_30 171
#define fsm_aswdP_trans_171 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_172 ((fsm_aswdP_state_34 * 128) + 104)
#define fsm_aswdP_trans_173 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_174 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_175 0
#define fsm_aswdP_state_31 176
#define fsm_aswdP_trans_176 ((fsm_aswdP_state_2 * 128) + 113)
#define fsm_aswdP_trans_177 ((fsm_aswdP_state_2 * 128) + 118)
#define fsm_aswdP_trans_178 ((fsm_aswdP_state_2 * 128) + 107)
#define fsm_aswdP_trans_179 ((fsm_aswdP_state_2 * 128) + 116)
#define fsm_aswdP_trans_180 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_181 ((fsm_aswdP_state_2 * 128) + 102)
#define fsm_aswdP_trans_182 ((fsm_aswdP_state_4 * 128) + 103)
#define fsm_aswdP_trans_183 ((fsm_aswdP_state_14 * 128) + 112)
#define fsm_aswdP_trans_184 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_185 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_186 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_187 ((fsm_aswdP_state_2 * 128) + 100)
#define fsm_aswdP_trans_188 ((fsm_aswdP_state_27 * 128) + 98)
#define fsm_aswdP_trans_189 ((fsm_aswdP_state_4 * 128) + 99)
#define fsm_aswdP_trans_190 ((fsm_aswdP_state_2 * 128) + 78)
#define fsm_aswdP_trans_191 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_192 0
#define fsm_aswdP_state_32 193
#define fsm_aswdP_trans_193 ((fsm_aswdP_state_2 * 128) + 104)
#define fsm_aswdP_trans_194 0
#define fsm_aswdP_state_33 195
#define fsm_aswdP_trans_195 ((fsm_aswdP_state_2 * 128) + 122)
#define fsm_aswdP_trans_196 0
#define fsm_aswdP_state_34 197
#define fsm_aswdP_trans_197 ((fsm_aswdP_state_2 * 128) + 119)
#define fsm_aswdP_trans_198 ((fsm_aswdP_state_2 * 128) + 108)
#define fsm_aswdP_trans_199 ((fsm_aswdP_state_2 * 128) + 114)
#define fsm_aswdP_trans_200 ((fsm_aswdP_state_2 * 128) + 78)
#define fsm_aswdP_trans_201 ((fsm_aswdP_state_19 * 128) + 86)
#define fsm_aswdP_trans_202 0
static const unsigned short fsm_aswdP_trans[203] = {
   fsm_aswdP_trans_0,
   fsm_aswdP_trans_1,
   fsm_aswdP_trans_2,
   fsm_aswdP_trans_3,
   fsm_aswdP_trans_4,
   fsm_aswdP_trans_5,
   fsm_aswdP_trans_6,
   fsm_aswdP_trans_7,
   fsm_aswdP_trans_8,
   fsm_aswdP_trans_9,
   fsm_aswdP_trans_10,
   fsm_aswdP_trans_11,
   fsm_aswdP_trans_12,
   fsm_aswdP_trans_13,
   fsm_aswdP_trans_14,
   fsm_aswdP_trans_15,
   fsm_aswdP_trans_16,
   fsm_aswdP_trans_17,
   fsm_aswdP_trans_18,
   fsm_aswdP_trans_19,
   fsm_aswdP_trans_20,
   fsm_aswdP_trans_21,
   fsm_aswdP_trans_22,
   fsm_aswdP_trans_23,
   fsm_aswdP_trans_24,
   fsm_aswdP_trans_25,
   fsm_aswdP_trans_26,
   fsm_aswdP_trans_27,
   fsm_aswdP_trans_28,
   fsm_aswdP_trans_29,
   fsm_aswdP_trans_30,
   fsm_aswdP_trans_31,
   fsm_aswdP_trans_32,
   fsm_aswdP_trans_33,
   fsm_aswdP_trans_34,
   fsm_aswdP_trans_35,
   fsm_aswdP_trans_36,
   fsm_aswdP_trans_37,
   fsm_aswdP_trans_38,
   fsm_aswdP_trans_39,
   fsm_aswdP_trans_40,
   fsm_aswdP_trans_41,
   fsm_aswdP_trans_42,
   fsm_aswdP_trans_43,
   fsm_aswdP_trans_44,
   fsm_aswdP_trans_45,
   fsm_aswdP_trans_46,
   fsm_aswdP_trans_47,
   fsm_aswdP_trans_48,
   fsm_aswdP_trans_49,
   fsm_aswdP_trans_50,
   fsm_aswdP_trans_51,
   fsm_aswdP_trans_52,
   fsm_aswdP_trans_53,
   fsm_aswdP_trans_54,
   fsm_aswdP_trans_55,
   fsm_aswdP_trans_56,
   fsm_aswdP_trans_57,
   fsm_aswdP_trans_58,
   fsm_aswdP_trans_59,
   fsm_aswdP_trans_60,
   fsm_aswdP_trans_61,
   fsm_aswdP_trans_62,
   fsm_aswdP_trans_63,
   fsm_aswdP_trans_64,
   fsm_aswdP_trans_65,
   fsm_aswdP_trans_66,
   fsm_aswdP_trans_67,
   fsm_aswdP_trans_68,
   fsm_aswdP_trans_69,
   fsm_aswdP_trans_70,
   fsm_aswdP_trans_71,
   fsm_aswdP_trans_72,
   fsm_aswdP_trans_73,
   fsm_aswdP_trans_74,
   fsm_aswdP_trans_75,
   fsm_aswdP_trans_76,
   fsm_aswdP_trans_77,
   fsm_aswdP_trans_78,
   fsm_aswdP_trans_79,
   fsm_aswdP_trans_80,
   fsm_aswdP_trans_81,
   fsm_aswdP_trans_82,
   fsm_aswdP_trans_83,
   fsm_aswdP_trans_84,
   fsm_aswdP_trans_85,
   fsm_aswdP_trans_86,
   fsm_aswdP_trans_87,
   fsm_aswdP_trans_88,
   fsm_aswdP_trans_89,
   fsm_aswdP_trans_90,
   fsm_aswdP_trans_91,
   fsm_aswdP_trans_92,
   fsm_aswdP_trans_93,
   fsm_aswdP_trans_94,
   fsm_aswdP_trans_95,
   fsm_aswdP_trans_96,
   fsm_aswdP_trans_97,
   fsm_aswdP_trans_98,
   fsm_aswdP_trans_99,
   fsm_aswdP_trans_100,
   fsm_aswdP_trans_101,
   fsm_aswdP_trans_102,
   fsm_aswdP_trans_103,
   fsm_aswdP_trans_104,
   fsm_aswdP_trans_105,
   fsm_aswdP_trans_106,
   fsm_aswdP_trans_107,
   fsm_aswdP_trans_108,
   fsm_aswdP_trans_109,
   fsm_aswdP_trans_110,
   fsm_aswdP_trans_111,
   fsm_aswdP_trans_112,
   fsm_aswdP_trans_113,
   fsm_aswdP_trans_114,
   fsm_aswdP_trans_115,
   fsm_aswdP_trans_116,
   fsm_aswdP_trans_117,
   fsm_aswdP_trans_118,
   fsm_aswdP_trans_119,
   fsm_aswdP_trans_120,
   fsm_aswdP_trans_121,
   fsm_aswdP_trans_122,
   fsm_aswdP_trans_123,
   fsm_aswdP_trans_124,
   fsm_aswdP_trans_125,
   fsm_aswdP_trans_126,
   fsm_aswdP_trans_127,
   fsm_aswdP_trans_128,
   fsm_aswdP_trans_129,
   fsm_aswdP_trans_130,
   fsm_aswdP_trans_131,
   fsm_aswdP_trans_132,
   fsm_aswdP_trans_133,
   fsm_aswdP_trans_134,
   fsm_aswdP_trans_135,
   fsm_aswdP_trans_136,
   fsm_aswdP_trans_137,
   fsm_aswdP_trans_138,
   fsm_aswdP_trans_139,
   fsm_aswdP_trans_140,
   fsm_aswdP_trans_141,
   fsm_aswdP_trans_142,
   fsm_aswdP_trans_143,
   fsm_aswdP_trans_144,
   fsm_aswdP_trans_145,
   fsm_aswdP_trans_146,
   fsm_aswdP_trans_147,
   fsm_aswdP_trans_148,
   fsm_aswdP_trans_149,
   fsm_aswdP_trans_150,
   fsm_aswdP_trans_151,
   fsm_aswdP_trans_152,
   fsm_aswdP_trans_153,
   fsm_aswdP_trans_154,
   fsm_aswdP_trans_155,
   fsm_aswdP_trans_156,
   fsm_aswdP_trans_157,
   fsm_aswdP_trans_158,
   fsm_aswdP_trans_159,
   fsm_aswdP_trans_160,
   fsm_aswdP_trans_161,
   fsm_aswdP_trans_162,
   fsm_aswdP_trans_163,
   fsm_aswdP_trans_164,
   fsm_aswdP_trans_165,
   fsm_aswdP_trans_166,
   fsm_aswdP_trans_167,
   fsm_aswdP_trans_168,
   fsm_aswdP_trans_169,
   fsm_aswdP_trans_170,
   fsm_aswdP_trans_171,
   fsm_aswdP_trans_172,
   fsm_aswdP_trans_173,
   fsm_aswdP_trans_174,
   fsm_aswdP_trans_175,
   fsm_aswdP_trans_176,
   fsm_aswdP_trans_177,
   fsm_aswdP_trans_178,
   fsm_aswdP_trans_179,
   fsm_aswdP_trans_180,
   fsm_aswdP_trans_181,
   fsm_aswdP_trans_182,
   fsm_aswdP_trans_183,
   fsm_aswdP_trans_184,
   fsm_aswdP_trans_185,
   fsm_aswdP_trans_186,
   fsm_aswdP_trans_187,
   fsm_aswdP_trans_188,
   fsm_aswdP_trans_189,
   fsm_aswdP_trans_190,
   fsm_aswdP_trans_191,
   fsm_aswdP_trans_192,
   fsm_aswdP_trans_193,
   fsm_aswdP_trans_194,
   fsm_aswdP_trans_195,
   fsm_aswdP_trans_196,
   fsm_aswdP_trans_197,
   fsm_aswdP_trans_198,
   fsm_aswdP_trans_199,
   fsm_aswdP_trans_200,
   fsm_aswdP_trans_201,
   fsm_aswdP_trans_202
};
static const cst_fsm fsm_aswdP = {
   128, /* vocab size */
   203,  /* num_transitions */
   fsm_aswdP_trans
};
#define fsm_aswdS_state_0 0
#define fsm_aswdS_trans_0 ((fsm_aswdS_state_1 * 128) + 35)
#define fsm_aswdS_trans_1 0
#define fsm_aswdS_state_1 2
#define fsm_aswdS_trans_2 ((fsm_aswdS_state_2 * 128) + 106)
#define fsm_aswdS_trans_3 ((fsm_aswdS_state_3 * 128) + 113)
#define fsm_aswdS_trans_4 ((fsm_aswdS_state_4 * 128) + 118)
#define fsm_aswdS_trans_5 ((fsm_aswdS_state_5 * 128) + 98)
#define fsm_aswdS_trans_6 ((fsm_aswdS_state_6 * 128) + 122)
#define fsm_aswdS_trans_7 ((fsm_aswdS_state_7 * 128) + 102)
#define fsm_aswdS_trans_8 ((fsm_aswdS_state_8 * 128) + 120)
#define fsm_aswdS_trans_9 ((fsm_aswdS_state_9 * 128) + 112)
#define fsm_aswdS_trans_10 ((fsm_aswdS_state_10 * 128) + 104)
#define fsm_aswdS_trans_11 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_12 ((fsm_aswdS_state_11 * 128) + 99)
#define fsm_aswdS_trans_13 ((fsm_aswdS_state_12 * 128) + 107)
#define fsm_aswdS_trans_14 ((fsm_aswdS_state_13 * 128) + 116)
#define fsm_aswdS_trans_15 ((fsm_aswdS_state_14 * 128) + 108)
#define fsm_aswdS_trans_16 ((fsm_aswdS_state_15 * 128) + 103)
#define fsm_aswdS_trans_17 ((fsm_aswdS_state_16 * 128) + 100)
#define fsm_aswdS_trans_18 ((fsm_aswdS_state_17 * 128) + 115)
#define fsm_aswdS_trans_19 ((fsm_aswdS_state_18 * 128) + 114)
#define fsm_aswdS_trans_20 ((fsm_aswdS_state_19 * 128) + 78)
#define fsm_aswdS_trans_21 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_22 0
#define fsm_aswdS_state_2 23
#define fsm_aswdS_trans_23 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_24 0
#define fsm_aswdS_state_3 25
#define fsm_aswdS_trans_25 ((fsm_aswdS_state_4 * 128) + 99)
#define fsm_aswdS_trans_26 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_27 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_28 0
#define fsm_aswdS_state_4 29
#define fsm_aswdS_trans_29 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_30 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_31 0
#define fsm_aswdS_state_5 32
#define fsm_aswdS_trans_32 ((fsm_aswdS_state_2 * 128) + 98)
#define fsm_aswdS_trans_33 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_34 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_35 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_36 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_37 0
#define fsm_aswdS_state_6 38
#define fsm_aswdS_trans_38 ((fsm_aswdS_state_2 * 128) + 122)
#define fsm_aswdS_trans_39 ((fsm_aswdS_state_2 * 128) + 102)
#define fsm_aswdS_trans_40 ((fsm_aswdS_state_21 * 128) + 99)
#define fsm_aswdS_trans_41 ((fsm_aswdS_state_22 * 128) + 116)
#define fsm_aswdS_trans_42 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_43 ((fsm_aswdS_state_23 * 128) + 100)
#define fsm_aswdS_trans_44 ((fsm_aswdS_state_24 * 128) + 115)
#define fsm_aswdS_trans_45 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_46 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_47 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_48 0
#define fsm_aswdS_state_7 49
#define fsm_aswdS_trans_49 ((fsm_aswdS_state_25 * 128) + 102)
#define fsm_aswdS_trans_50 ((fsm_aswdS_state_26 * 128) + 112)
#define fsm_aswdS_trans_51 ((fsm_aswdS_state_27 * 128) + 108)
#define fsm_aswdS_trans_52 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_53 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_54 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_55 0
#define fsm_aswdS_state_8 56
#define fsm_aswdS_trans_56 ((fsm_aswdS_state_2 * 128) + 120)
#define fsm_aswdS_trans_57 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_58 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_59 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_60 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_61 0
#define fsm_aswdS_state_9 62
#define fsm_aswdS_trans_62 ((fsm_aswdS_state_24 * 128) + 112)
#define fsm_aswdS_trans_63 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_64 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_65 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_66 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_67 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_68 0
#define fsm_aswdS_state_10 69
#define fsm_aswdS_trans_69 ((fsm_aswdS_state_2 * 128) + 98)
#define fsm_aswdS_trans_70 ((fsm_aswdS_state_28 * 128) + 112)
#define fsm_aswdS_trans_71 ((fsm_aswdS_state_29 * 128) + 99)
#define fsm_aswdS_trans_72 ((fsm_aswdS_state_2 * 128) + 107)
#define fsm_aswdS_trans_73 ((fsm_aswdS_state_30 * 128) + 116)
#define fsm_aswdS_trans_74 ((fsm_aswdS_state_28 * 128) + 103)
#define fsm_aswdS_trans_75 ((fsm_aswdS_state_24 * 128) + 100)
#define fsm_aswdS_trans_76 ((fsm_aswdS_state_31 * 128) + 115)
#define fsm_aswdS_trans_77 ((fsm_aswdS_state_32 * 128) + 114)
#define fsm_aswdS_trans_78 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_79 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_80 0
#define fsm_aswdS_state_11 81
#define fsm_aswdS_trans_81 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_82 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_83 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_84 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_85 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_86 0
#define fsm_aswdS_state_12 87
#define fsm_aswdS_trans_87 ((fsm_aswdS_state_2 * 128) + 122)
#define fsm_aswdS_trans_88 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_89 ((fsm_aswdS_state_28 * 128) + 99)
#define fsm_aswdS_trans_90 ((fsm_aswdS_state_2 * 128) + 107)
#define fsm_aswdS_trans_91 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_92 ((fsm_aswdS_state_33 * 128) + 115)
#define fsm_aswdS_trans_93 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_94 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_95 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_96 0
#define fsm_aswdS_state_13 97
#define fsm_aswdS_trans_97 ((fsm_aswdS_state_2 * 128) + 98)
#define fsm_aswdS_trans_98 ((fsm_aswdS_state_34 * 128) + 122)
#define fsm_aswdS_trans_99 ((fsm_aswdS_state_35 * 128) + 102)
#define fsm_aswdS_trans_100 ((fsm_aswdS_state_2 * 128) + 120)
#define fsm_aswdS_trans_101 ((fsm_aswdS_state_28 * 128) + 112)
#define fsm_aswdS_trans_102 ((fsm_aswdS_state_36 * 128) + 104)
#define fsm_aswdS_trans_103 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_104 ((fsm_aswdS_state_24 * 128) + 99)
#define fsm_aswdS_trans_105 ((fsm_aswdS_state_4 * 128) + 107)
#define fsm_aswdS_trans_106 ((fsm_aswdS_state_26 * 128) + 116)
#define fsm_aswdS_trans_107 ((fsm_aswdS_state_37 * 128) + 108)
#define fsm_aswdS_trans_108 ((fsm_aswdS_state_24 * 128) + 103)
#define fsm_aswdS_trans_109 ((fsm_aswdS_state_38 * 128) + 100)
#define fsm_aswdS_trans_110 ((fsm_aswdS_state_39 * 128) + 115)
#define fsm_aswdS_trans_111 ((fsm_aswdS_state_27 * 128) + 114)
#define fsm_aswdS_trans_112 ((fsm_aswdS_state_40 * 128) + 78)
#define fsm_aswdS_trans_113 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_114 0
#define fsm_aswdS_state_14 115
#define fsm_aswdS_trans_115 ((fsm_aswdS_state_2 * 128) + 106)
#define fsm_aswdS_trans_116 ((fsm_aswdS_state_2 * 128) + 118)
#define fsm_aswdS_trans_117 ((fsm_aswdS_state_2 * 128) + 98)
#define fsm_aswdS_trans_118 ((fsm_aswdS_state_41 * 128) + 122)
#define fsm_aswdS_trans_119 ((fsm_aswdS_state_24 * 128) + 112)
#define fsm_aswdS_trans_120 ((fsm_aswdS_state_42 * 128) + 104)
#define fsm_aswdS_trans_121 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_122 ((fsm_aswdS_state_43 * 128) + 99)
#define fsm_aswdS_trans_123 ((fsm_aswdS_state_44 * 128) + 107)
#define fsm_aswdS_trans_124 ((fsm_aswdS_state_45 * 128) + 116)
#define fsm_aswdS_trans_125 ((fsm_aswdS_state_4 * 128) + 108)
#define fsm_aswdS_trans_126 ((fsm_aswdS_state_24 * 128) + 103)
#define fsm_aswdS_trans_127 ((fsm_aswdS_state_24 * 128) + 100)
#define fsm_aswdS_trans_128 ((fsm_aswdS_state_46 * 128) + 115)
#define fsm_aswdS_trans_129 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_130 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_131 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_132 0
#define fsm_aswdS_state_15 133
#define fsm_aswdS_trans_133 ((fsm_aswdS_state_2 * 128) + 104)
#define fsm_aswdS_trans_134 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_135 ((fsm_aswdS_state_24 * 128) + 103)
#define fsm_aswdS_trans_136 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_137 ((fsm_aswdS_state_27 * 128) + 78)
#define fsm_aswdS_trans_138 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_139 0
#define fsm_aswdS_state_16 140
#define fsm_aswdS_trans_140 ((fsm_aswdS_state_2 * 128) + 122)
#define fsm_aswdS_trans_141 ((fsm_aswdS_state_2 * 128) + 104)
#define fsm_aswdS_trans_142 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_143 ((fsm_aswdS_state_47 * 128) + 108)
#define fsm_aswdS_trans_144 ((fsm_aswdS_state_2 * 128) + 100)
#define fsm_aswdS_trans_145 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_146 ((fsm_aswdS_state_4 * 128) + 78)
#define fsm_aswdS_trans_147 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_148 0
#define fsm_aswdS_state_17 149
#define fsm_aswdS_trans_149 ((fsm_aswdS_state_2 * 128) + 118)
#define fsm_aswdS_trans_150 ((fsm_aswdS_state_5 * 128) + 98)
#define fsm_aswdS_trans_151 ((fsm_aswdS_state_2 * 128) + 122)
#define fsm_aswdS_trans_152 ((fsm_aswdS_state_48 * 128) + 102)
#define fsm_aswdS_trans_153 ((fsm_aswdS_state_49 * 128) + 112)
#define fsm_aswdS_trans_154 ((fsm_aswdS_state_50 * 128) + 104)
#define fsm_aswdS_trans_155 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_156 ((fsm_aswdS_state_11 * 128) + 99)
#define fsm_aswdS_trans_157 ((fsm_aswdS_state_51 * 128) + 107)
#define fsm_aswdS_trans_158 ((fsm_aswdS_state_52 * 128) + 116)
#define fsm_aswdS_trans_159 ((fsm_aswdS_state_53 * 128) + 108)
#define fsm_aswdS_trans_160 ((fsm_aswdS_state_54 * 128) + 103)
#define fsm_aswdS_trans_161 ((fsm_aswdS_state_55 * 128) + 100)
#define fsm_aswdS_trans_162 ((fsm_aswdS_state_4 * 128) + 115)
#define fsm_aswdS_trans_163 ((fsm_aswdS_state_56 * 128) + 114)
#define fsm_aswdS_trans_164 ((fsm_aswdS_state_57 * 128) + 78)
#define fsm_aswdS_trans_165 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_166 0
#define fsm_aswdS_state_18 167
#define fsm_aswdS_trans_167 ((fsm_aswdS_state_2 * 128) + 118)
#define fsm_aswdS_trans_168 ((fsm_aswdS_state_2 * 128) + 104)
#define fsm_aswdS_trans_169 ((fsm_aswdS_state_2 * 128) + 116)
#define fsm_aswdS_trans_170 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_171 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_172 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_173 0
#define fsm_aswdS_state_19 174
#define fsm_aswdS_trans_174 ((fsm_aswdS_state_2 * 128) + 106)
#define fsm_aswdS_trans_175 ((fsm_aswdS_state_2 * 128) + 122)
#define fsm_aswdS_trans_176 ((fsm_aswdS_state_58 * 128) + 104)
#define fsm_aswdS_trans_177 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_178 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_179 ((fsm_aswdS_state_2 * 128) + 103)
#define fsm_aswdS_trans_180 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_181 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_182 ((fsm_aswdS_state_4 * 128) + 78)
#define fsm_aswdS_trans_183 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_184 0
#define fsm_aswdS_state_20 185
#define fsm_aswdS_trans_185 0
#define fsm_aswdS_state_21 186
#define fsm_aswdS_trans_186 ((fsm_aswdS_state_59 * 128) + 122)
#define fsm_aswdS_trans_187 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_188 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_189 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_190 0
#define fsm_aswdS_state_22 191
#define fsm_aswdS_trans_191 ((fsm_aswdS_state_2 * 128) + 116)
#define fsm_aswdS_trans_192 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_193 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_194 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_195 ((fsm_aswdS_state_4 * 128) + 78)
#define fsm_aswdS_trans_196 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_197 0
#define fsm_aswdS_state_23 198
#define fsm_aswdS_trans_198 ((fsm_aswdS_state_2 * 128) + 122)
#define fsm_aswdS_trans_199 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_200 0
#define fsm_aswdS_state_24 201
#define fsm_aswdS_trans_201 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_202 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_203 0
#define fsm_aswdS_state_25 204
#define fsm_aswdS_trans_204 ((fsm_aswdS_state_43 * 128) + 112)
#define fsm_aswdS_trans_205 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_206 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_207 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_208 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_209 0
#define fsm_aswdS_state_26 210
#define fsm_aswdS_trans_210 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_211 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_212 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_213 0
#define fsm_aswdS_state_27 214
#define fsm_aswdS_trans_214 ((fsm_aswdS_state_2 * 128) + 104)
#define fsm_aswdS_trans_215 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_216 0
#define fsm_aswdS_state_28 217
#define fsm_aswdS_trans_217 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_218 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_219 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_220 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_221 0
#define fsm_aswdS_state_29 222
#define fsm_aswdS_trans_222 ((fsm_aswdS_state_4 * 128) + 116)
#define fsm_aswdS_trans_223 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_224 ((fsm_aswdS_state_60 * 128) + 115)
#define fsm_aswdS_trans_225 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_226 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_227 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_228 0
#define fsm_aswdS_state_30 229
#define fsm_aswdS_trans_229 ((fsm_aswdS_state_61 * 128) + 118)
#define fsm_aswdS_trans_230 ((fsm_aswdS_state_62 * 128) + 102)
#define fsm_aswdS_trans_231 ((fsm_aswdS_state_2 * 128) + 120)
#define fsm_aswdS_trans_232 ((fsm_aswdS_state_2 * 128) + 112)
#define fsm_aswdS_trans_233 ((fsm_aswdS_state_63 * 128) + 104)
#define fsm_aswdS_trans_234 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_235 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_236 ((fsm_aswdS_state_43 * 128) + 103)
#define fsm_aswdS_trans_237 ((fsm_aswdS_state_2 * 128) + 100)
#define fsm_aswdS_trans_238 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_239 ((fsm_aswdS_state_4 * 128) + 78)
#define fsm_aswdS_trans_240 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_241 0
#define fsm_aswdS_state_31 242
#define fsm_aswdS_trans_242 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_243 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_244 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_245 0
#define fsm_aswdS_state_32 246
#define fsm_aswdS_trans_246 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_247 0
#define fsm_aswdS_state_33 248
#define fsm_aswdS_trans_248 ((fsm_aswdS_state_2 * 128) + 118)
#define fsm_aswdS_trans_249 ((fsm_aswdS_state_2 * 128) + 116)
#define fsm_aswdS_trans_250 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_251 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_252 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_253 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_254 0
#define fsm_aswdS_state_34 255
#define fsm_aswdS_trans_255 ((fsm_aswdS_state_32 * 128) + 116)
#define fsm_aswdS_trans_256 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_257 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_258 0
#define fsm_aswdS_state_35 259
#define fsm_aswdS_trans_259 ((fsm_aswdS_state_2 * 128) + 102)
#define fsm_aswdS_trans_260 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_261 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_262 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_263 0
#define fsm_aswdS_state_36 264
#define fsm_aswdS_trans_264 ((fsm_aswdS_state_64 * 128) + 99)
#define fsm_aswdS_trans_265 ((fsm_aswdS_state_2 * 128) + 103)
#define fsm_aswdS_trans_266 0
#define fsm_aswdS_state_37 267
#define fsm_aswdS_trans_267 ((fsm_aswdS_state_2 * 128) + 104)
#define fsm_aswdS_trans_268 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_269 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_270 0
#define fsm_aswdS_state_38 271
#define fsm_aswdS_trans_271 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_272 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_273 ((fsm_aswdS_state_4 * 128) + 78)
#define fsm_aswdS_trans_274 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_275 0
#define fsm_aswdS_state_39 276
#define fsm_aswdS_trans_276 ((fsm_aswdS_state_4 * 128) + 98)
#define fsm_aswdS_trans_277 ((fsm_aswdS_state_4 * 128) + 112)
#define fsm_aswdS_trans_278 ((fsm_aswdS_state_2 * 128) + 107)
#define fsm_aswdS_trans_279 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_280 ((fsm_aswdS_state_24 * 128) + 103)
#define fsm_aswdS_trans_281 ((fsm_aswdS_state_2 * 128) + 100)
#define fsm_aswdS_trans_282 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_283 ((fsm_aswdS_state_65 * 128) + 78)
#define fsm_aswdS_trans_284 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_285 0
#define fsm_aswdS_state_40 286
#define fsm_aswdS_trans_286 ((fsm_aswdS_state_66 * 128) + 116)
#define fsm_aswdS_trans_287 ((fsm_aswdS_state_62 * 128) + 100)
#define fsm_aswdS_trans_288 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_289 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_290 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_291 0
#define fsm_aswdS_state_41 292
#define fsm_aswdS_trans_292 ((fsm_aswdS_state_2 * 128) + 116)
#define fsm_aswdS_trans_293 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_294 0
#define fsm_aswdS_state_42 295
#define fsm_aswdS_trans_295 ((fsm_aswdS_state_67 * 128) + 99)
#define fsm_aswdS_trans_296 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_297 0
#define fsm_aswdS_state_43 298
#define fsm_aswdS_trans_298 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_299 0
#define fsm_aswdS_state_44 300
#define fsm_aswdS_trans_300 ((fsm_aswdS_state_2 * 128) + 99)
#define fsm_aswdS_trans_301 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_302 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_303 0
#define fsm_aswdS_state_45 304
#define fsm_aswdS_trans_304 ((fsm_aswdS_state_2 * 128) + 116)
#define fsm_aswdS_trans_305 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_306 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_307 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_308 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_309 0
#define fsm_aswdS_state_46 310
#define fsm_aswdS_trans_310 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_311 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_312 0
#define fsm_aswdS_state_47 313
#define fsm_aswdS_trans_313 ((fsm_aswdS_state_2 * 128) + 99)
#define fsm_aswdS_trans_314 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_315 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_316 0
#define fsm_aswdS_state_48 317
#define fsm_aswdS_trans_317 ((fsm_aswdS_state_2 * 128) + 102)
#define fsm_aswdS_trans_318 ((fsm_aswdS_state_27 * 128) + 108)
#define fsm_aswdS_trans_319 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_320 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_321 0
#define fsm_aswdS_state_49 322
#define fsm_aswdS_trans_322 ((fsm_aswdS_state_2 * 128) + 112)
#define fsm_aswdS_trans_323 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_324 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_325 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_326 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_327 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_328 0
#define fsm_aswdS_state_50 329
#define fsm_aswdS_trans_329 ((fsm_aswdS_state_28 * 128) + 112)
#define fsm_aswdS_trans_330 ((fsm_aswdS_state_4 * 128) + 99)
#define fsm_aswdS_trans_331 ((fsm_aswdS_state_2 * 128) + 107)
#define fsm_aswdS_trans_332 ((fsm_aswdS_state_68 * 128) + 116)
#define fsm_aswdS_trans_333 ((fsm_aswdS_state_2 * 128) + 103)
#define fsm_aswdS_trans_334 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_335 0
#define fsm_aswdS_state_51 336
#define fsm_aswdS_trans_336 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_337 ((fsm_aswdS_state_26 * 128) + 99)
#define fsm_aswdS_trans_338 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_339 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_340 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_341 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_342 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_343 0
#define fsm_aswdS_state_52 344
#define fsm_aswdS_trans_344 ((fsm_aswdS_state_2 * 128) + 98)
#define fsm_aswdS_trans_345 ((fsm_aswdS_state_2 * 128) + 122)
#define fsm_aswdS_trans_346 ((fsm_aswdS_state_2 * 128) + 102)
#define fsm_aswdS_trans_347 ((fsm_aswdS_state_2 * 128) + 120)
#define fsm_aswdS_trans_348 ((fsm_aswdS_state_28 * 128) + 112)
#define fsm_aswdS_trans_349 ((fsm_aswdS_state_69 * 128) + 104)
#define fsm_aswdS_trans_350 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_351 ((fsm_aswdS_state_24 * 128) + 99)
#define fsm_aswdS_trans_352 ((fsm_aswdS_state_2 * 128) + 116)
#define fsm_aswdS_trans_353 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_354 ((fsm_aswdS_state_2 * 128) + 103)
#define fsm_aswdS_trans_355 ((fsm_aswdS_state_70 * 128) + 100)
#define fsm_aswdS_trans_356 ((fsm_aswdS_state_4 * 128) + 115)
#define fsm_aswdS_trans_357 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_358 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_359 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_360 0
#define fsm_aswdS_state_53 361
#define fsm_aswdS_trans_361 ((fsm_aswdS_state_2 * 128) + 118)
#define fsm_aswdS_trans_362 ((fsm_aswdS_state_2 * 128) + 104)
#define fsm_aswdS_trans_363 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_364 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_365 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_366 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_367 0
#define fsm_aswdS_state_54 368
#define fsm_aswdS_trans_368 ((fsm_aswdS_state_2 * 128) + 103)
#define fsm_aswdS_trans_369 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_370 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_371 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_372 0
#define fsm_aswdS_state_55 373
#define fsm_aswdS_trans_373 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_374 ((fsm_aswdS_state_65 * 128) + 108)
#define fsm_aswdS_trans_375 ((fsm_aswdS_state_2 * 128) + 100)
#define fsm_aswdS_trans_376 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_377 ((fsm_aswdS_state_71 * 128) + 78)
#define fsm_aswdS_trans_378 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_379 0
#define fsm_aswdS_state_56 380
#define fsm_aswdS_trans_380 ((fsm_aswdS_state_2 * 128) + 104)
#define fsm_aswdS_trans_381 ((fsm_aswdS_state_66 * 128) + 115)
#define fsm_aswdS_trans_382 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_383 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_384 0
#define fsm_aswdS_state_57 385
#define fsm_aswdS_trans_385 ((fsm_aswdS_state_58 * 128) + 104)
#define fsm_aswdS_trans_386 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_387 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_388 ((fsm_aswdS_state_2 * 128) + 103)
#define fsm_aswdS_trans_389 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_390 ((fsm_aswdS_state_27 * 128) + 114)
#define fsm_aswdS_trans_391 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_392 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_393 0
#define fsm_aswdS_state_58 394
#define fsm_aswdS_trans_394 ((fsm_aswdS_state_2 * 128) + 116)
#define fsm_aswdS_trans_395 ((fsm_aswdS_state_2 * 128) + 103)
#define fsm_aswdS_trans_396 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_397 0
#define fsm_aswdS_state_59 398
#define fsm_aswdS_trans_398 ((fsm_aswdS_state_32 * 128) + 115)
#define fsm_aswdS_trans_399 0
#define fsm_aswdS_state_60 400
#define fsm_aswdS_trans_400 ((fsm_aswdS_state_72 * 128) + 122)
#define fsm_aswdS_trans_401 ((fsm_aswdS_state_2 * 128) + 107)
#define fsm_aswdS_trans_402 ((fsm_aswdS_state_73 * 128) + 116)
#define fsm_aswdS_trans_403 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_404 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_405 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_406 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_407 0
#define fsm_aswdS_state_61 408
#define fsm_aswdS_trans_408 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_409 0
#define fsm_aswdS_state_62 410
#define fsm_aswdS_trans_410 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_411 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_412 0
#define fsm_aswdS_state_63 413
#define fsm_aswdS_trans_413 ((fsm_aswdS_state_2 * 128) + 103)
#define fsm_aswdS_trans_414 0
#define fsm_aswdS_state_64 415
#define fsm_aswdS_trans_415 ((fsm_aswdS_state_32 * 128) + 115)
#define fsm_aswdS_trans_416 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_417 0
#define fsm_aswdS_state_65 418
#define fsm_aswdS_trans_418 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_419 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_420 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_421 0
#define fsm_aswdS_state_66 422
#define fsm_aswdS_trans_422 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_423 0
#define fsm_aswdS_state_67 424
#define fsm_aswdS_trans_424 ((fsm_aswdS_state_2 * 128) + 115)
#define fsm_aswdS_trans_425 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_426 0
#define fsm_aswdS_state_68 427
#define fsm_aswdS_trans_427 ((fsm_aswdS_state_2 * 128) + 102)
#define fsm_aswdS_trans_428 ((fsm_aswdS_state_2 * 128) + 120)
#define fsm_aswdS_trans_429 ((fsm_aswdS_state_2 * 128) + 112)
#define fsm_aswdS_trans_430 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_431 ((fsm_aswdS_state_2 * 128) + 108)
#define fsm_aswdS_trans_432 ((fsm_aswdS_state_43 * 128) + 103)
#define fsm_aswdS_trans_433 ((fsm_aswdS_state_24 * 128) + 100)
#define fsm_aswdS_trans_434 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_435 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_436 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_437 0
#define fsm_aswdS_state_69 438
#define fsm_aswdS_trans_438 ((fsm_aswdS_state_2 * 128) + 99)
#define fsm_aswdS_trans_439 ((fsm_aswdS_state_2 * 128) + 103)
#define fsm_aswdS_trans_440 0
#define fsm_aswdS_state_70 441
#define fsm_aswdS_trans_441 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_442 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_443 0
#define fsm_aswdS_state_71 444
#define fsm_aswdS_trans_444 ((fsm_aswdS_state_2 * 128) + 119)
#define fsm_aswdS_trans_445 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_446 0
#define fsm_aswdS_state_72 447
#define fsm_aswdS_trans_447 ((fsm_aswdS_state_43 * 128) + 116)
#define fsm_aswdS_trans_448 0
#define fsm_aswdS_state_73 449
#define fsm_aswdS_trans_449 ((fsm_aswdS_state_2 * 128) + 116)
#define fsm_aswdS_trans_450 ((fsm_aswdS_state_2 * 128) + 114)
#define fsm_aswdS_trans_451 ((fsm_aswdS_state_2 * 128) + 78)
#define fsm_aswdS_trans_452 ((fsm_aswdS_state_20 * 128) + 86)
#define fsm_aswdS_trans_453 0
static const unsigned short fsm_aswdS_trans[454] = {
   fsm_aswdS_trans_0,
   fsm_aswdS_trans_1,
   fsm_aswdS_trans_2,
   fsm_aswdS_trans_3,
   fsm_aswdS_trans_4,
   fsm_aswdS_trans_5,
   fsm_aswdS_trans_6,
   fsm_aswdS_trans_7,
   fsm_aswdS_trans_8,
   fsm_aswdS_trans_9,
   fsm_aswdS_trans_10,
   fsm_aswdS_trans_11,
   fsm_aswdS_trans_12,
   fsm_aswdS_trans_13,
   fsm_aswdS_trans_14,
   fsm_aswdS_trans_15,
   fsm_aswdS_trans_16,
   fsm_aswdS_trans_17,
   fsm_aswdS_trans_18,
   fsm_aswdS_trans_19,
   fsm_aswdS_trans_20,
   fsm_aswdS_trans_21,
   fsm_aswdS_trans_22,
   fsm_aswdS_trans_23,
   fsm_aswdS_trans_24,
   fsm_aswdS_trans_25,
   fsm_aswdS_trans_26,
   fsm_aswdS_trans_27,
   fsm_aswdS_trans_28,
   fsm_aswdS_trans_29,
   fsm_aswdS_trans_30,
   fsm_aswdS_trans_31,
   fsm_aswdS_trans_32,
   fsm_aswdS_trans_33,
   fsm_aswdS_trans_34,
   fsm_aswdS_trans_35,
   fsm_aswdS_trans_36,
   fsm_aswdS_trans_37,
   fsm_aswdS_trans_38,
   fsm_aswdS_trans_39,
   fsm_aswdS_trans_40,
   fsm_aswdS_trans_41,
   fsm_aswdS_trans_42,
   fsm_aswdS_trans_43,
   fsm_aswdS_trans_44,
   fsm_aswdS_trans_45,
   fsm_aswdS_trans_46,
   fsm_aswdS_trans_47,
   fsm_aswdS_trans_48,
   fsm_aswdS_trans_49,
   fsm_aswdS_trans_50,
   fsm_aswdS_trans_51,
   fsm_aswdS_trans_52,
   fsm_aswdS_trans_53,
   fsm_aswdS_trans_54,
   fsm_aswdS_trans_55,
   fsm_aswdS_trans_56,
   fsm_aswdS_trans_57,
   fsm_aswdS_trans_58,
   fsm_aswdS_trans_59,
   fsm_aswdS_trans_60,
   fsm_aswdS_trans_61,
   fsm_aswdS_trans_62,
   fsm_aswdS_trans_63,
   fsm_aswdS_trans_64,
   fsm_aswdS_trans_65,
   fsm_aswdS_trans_66,
   fsm_aswdS_trans_67,
   fsm_aswdS_trans_68,
   fsm_aswdS_trans_69,
   fsm_aswdS_trans_70,
   fsm_aswdS_trans_71,
   fsm_aswdS_trans_72,
   fsm_aswdS_trans_73,
   fsm_aswdS_trans_74,
   fsm_aswdS_trans_75,
   fsm_aswdS_trans_76,
   fsm_aswdS_trans_77,
   fsm_aswdS_trans_78,
   fsm_aswdS_trans_79,
   fsm_aswdS_trans_80,
   fsm_aswdS_trans_81,
   fsm_aswdS_trans_82,
   fsm_aswdS_trans_83,
   fsm_aswdS_trans_84,
   fsm_aswdS_trans_85,
   fsm_aswdS_trans_86,
   fsm_aswdS_trans_87,
   fsm_aswdS_trans_88,
   fsm_aswdS_trans_89,
   fsm_aswdS_trans_90,
   fsm_aswdS_trans_91,
   fsm_aswdS_trans_92,
   fsm_aswdS_trans_93,
   fsm_aswdS_trans_94,
   fsm_aswdS_trans_95,
   fsm_aswdS_trans_96,
   fsm_aswdS_trans_97,
   fsm_aswdS_trans_98,
   fsm_aswdS_trans_99,
   fsm_aswdS_trans_100,
   fsm_aswdS_trans_101,
   fsm_aswdS_trans_102,
   fsm_aswdS_trans_103,
   fsm_aswdS_trans_104,
   fsm_aswdS_trans_105,
   fsm_aswdS_trans_106,
   fsm_aswdS_trans_107,
   fsm_aswdS_trans_108,
   fsm_aswdS_trans_109,
   fsm_aswdS_trans_110,
   fsm_aswdS_trans_111,
   fsm_aswdS_trans_112,
   fsm_aswdS_trans_113,
   fsm_aswdS_trans_114,
   fsm_aswdS_trans_115,
   fsm_aswdS_trans_116,
   fsm_aswdS_trans_117,
   fsm_aswdS_trans_118,
   fsm_aswdS_trans_119,
   fsm_aswdS_trans_120,
   fsm_aswdS_trans_121,
   fsm_aswdS_trans_122,
   fsm_aswdS_trans_123,
   fsm_aswdS_trans_124,
   fsm_aswdS_trans_125,
   fsm_aswdS_trans_126,
   fsm_aswdS_trans_127,
   fsm_aswdS_trans_128,
   fsm_aswdS_trans_129,
   fsm_aswdS_trans_130,
   fsm_aswdS_trans_131,
   fsm_aswdS_trans_132,
   fsm_aswdS_trans_133,
   fsm_aswdS_trans_134,
   fsm_aswdS_trans_135,
   fsm_aswdS_trans_136,
   fsm_aswdS_trans_137,
   fsm_aswdS_trans_138,
   fsm_aswdS_trans_139,
   fsm_aswdS_trans_140,
   fsm_aswdS_trans_141,
   fsm_aswdS_trans_142,
   fsm_aswdS_trans_143,
   fsm_aswdS_trans_144,
   fsm_aswdS_trans_145,
   fsm_aswdS_trans_146,
   fsm_aswdS_trans_147,
   fsm_aswdS_trans_148,
   fsm_aswdS_trans_149,
   fsm_aswdS_trans_150,
   fsm_aswdS_trans_151,
   fsm_aswdS_trans_152,
   fsm_aswdS_trans_153,
   fsm_aswdS_trans_154,
   fsm_aswdS_trans_155,
   fsm_aswdS_trans_156,
   fsm_aswdS_trans_157,
   fsm_aswdS_trans_158,
   fsm_aswdS_trans_159,
   fsm_aswdS_trans_160,
   fsm_aswdS_trans_161,
   fsm_aswdS_trans_162,
   fsm_aswdS_trans_163,
   fsm_aswdS_trans_164,
   fsm_aswdS_trans_165,
   fsm_aswdS_trans_166,
   fsm_aswdS_trans_167,
   fsm_aswdS_trans_168,
   fsm_aswdS_trans_169,
   fsm_aswdS_trans_170,
   fsm_aswdS_trans_171,
   fsm_aswdS_trans_172,
   fsm_aswdS_trans_173,
   fsm_aswdS_trans_174,
   fsm_aswdS_trans_175,
   fsm_aswdS_trans_176,
   fsm_aswdS_trans_177,
   fsm_aswdS_trans_178,
   fsm_aswdS_trans_179,
   fsm_aswdS_trans_180,
   fsm_aswdS_trans_181,
   fsm_aswdS_trans_182,
   fsm_aswdS_trans_183,
   fsm_aswdS_trans_184,
   fsm_aswdS_trans_185,
   fsm_aswdS_trans_186,
   fsm_aswdS_trans_187,
   fsm_aswdS_trans_188,
   fsm_aswdS_trans_189,
   fsm_aswdS_trans_190,
   fsm_aswdS_trans_191,
   fsm_aswdS_trans_192,
   fsm_aswdS_trans_193,
   fsm_aswdS_trans_194,
   fsm_aswdS_trans_195,
   fsm_aswdS_trans_196,
   fsm_aswdS_trans_197,
   fsm_aswdS_trans_198,
   fsm_aswdS_trans_199,
   fsm_aswdS_trans_200,
   fsm_aswdS_trans_201,
   fsm_aswdS_trans_202,
   fsm_aswdS_trans_203,
   fsm_aswdS_trans_204,
   fsm_aswdS_trans_205,
   fsm_aswdS_trans_206,
   fsm_aswdS_trans_207,
   fsm_aswdS_trans_208,
   fsm_aswdS_trans_209,
   fsm_aswdS_trans_210,
   fsm_aswdS_trans_211,
   fsm_aswdS_trans_212,
   fsm_aswdS_trans_213,
   fsm_aswdS_trans_214,
   fsm_aswdS_trans_215,
   fsm_aswdS_trans_216,
   fsm_aswdS_trans_217,
   fsm_aswdS_trans_218,
   fsm_aswdS_trans_219,
   fsm_aswdS_trans_220,
   fsm_aswdS_trans_221,
   fsm_aswdS_trans_222,
   fsm_aswdS_trans_223,
   fsm_aswdS_trans_224,
   fsm_aswdS_trans_225,
   fsm_aswdS_trans_226,
   fsm_aswdS_trans_227,
   fsm_aswdS_trans_228,
   fsm_aswdS_trans_229,
   fsm_aswdS_trans_230,
   fsm_aswdS_trans_231,
   fsm_aswdS_trans_232,
   fsm_aswdS_trans_233,
   fsm_aswdS_trans_234,
   fsm_aswdS_trans_235,
   fsm_aswdS_trans_236,
   fsm_aswdS_trans_237,
   fsm_aswdS_trans_238,
   fsm_aswdS_trans_239,
   fsm_aswdS_trans_240,
   fsm_aswdS_trans_241,
   fsm_aswdS_trans_242,
   fsm_aswdS_trans_243,
   fsm_aswdS_trans_244,
   fsm_aswdS_trans_245,
   fsm_aswdS_trans_246,
   fsm_aswdS_trans_247,
   fsm_aswdS_trans_248,
   fsm_aswdS_trans_249,
   fsm_aswdS_trans_250,
   fsm_aswdS_trans_251,
   fsm_aswdS_trans_252,
   fsm_aswdS_trans_253,
   fsm_aswdS_trans_254,
   fsm_aswdS_trans_255,
   fsm_aswdS_trans_256,
   fsm_aswdS_trans_257,
   fsm_aswdS_trans_258,
   fsm_aswdS_trans_259,
   fsm_aswdS_trans_260,
   fsm_aswdS_trans_261,
   fsm_aswdS_trans_262,
   fsm_aswdS_trans_263,
   fsm_aswdS_trans_264,
   fsm_aswdS_trans_265,
   fsm_aswdS_trans_266,
   fsm_aswdS_trans_267,
   fsm_aswdS_trans_268,
   fsm_aswdS_trans_269,
   fsm_aswdS_trans_270,
   fsm_aswdS_trans_271,
   fsm_aswdS_trans_272,
   fsm_aswdS_trans_273,
   fsm_aswdS_trans_274,
   fsm_aswdS_trans_275,
   fsm_aswdS_trans_276,
   fsm_aswdS_trans_277,
   fsm_aswdS_trans_278,
   fsm_aswdS_trans_279,
   fsm_aswdS_trans_280,
   fsm_aswdS_trans_281,
   fsm_aswdS_trans_282,
   fsm_aswdS_trans_283,
   fsm_aswdS_trans_284,
   fsm_aswdS_trans_285,
   fsm_aswdS_trans_286,
   fsm_aswdS_trans_287,
   fsm_aswdS_trans_288,
   fsm_aswdS_trans_289,
   fsm_aswdS_trans_290,
   fsm_aswdS_trans_291,
   fsm_aswdS_trans_292,
   fsm_aswdS_trans_293,
   fsm_aswdS_trans_294,
   fsm_aswdS_trans_295,
   fsm_aswdS_trans_296,
   fsm_aswdS_trans_297,
   fsm_aswdS_trans_298,
   fsm_aswdS_trans_299,
   fsm_aswdS_trans_300,
   fsm_aswdS_trans_301,
   fsm_aswdS_trans_302,
   fsm_aswdS_trans_303,
   fsm_aswdS_trans_304,
   fsm_aswdS_trans_305,
   fsm_aswdS_trans_306,
   fsm_aswdS_trans_307,
   fsm_aswdS_trans_308,
   fsm_aswdS_trans_309,
   fsm_aswdS_trans_310,
   fsm_aswdS_trans_311,
   fsm_aswdS_trans_312,
   fsm_aswdS_trans_313,
   fsm_aswdS_trans_314,
   fsm_aswdS_trans_315,
   fsm_aswdS_trans_316,
   fsm_aswdS_trans_317,
   fsm_aswdS_trans_318,
   fsm_aswdS_trans_319,
   fsm_aswdS_trans_320,
   fsm_aswdS_trans_321,
   fsm_aswdS_trans_322,
   fsm_aswdS_trans_323,
   fsm_aswdS_trans_324,
   fsm_aswdS_trans_325,
   fsm_aswdS_trans_326,
   fsm_aswdS_trans_327,
   fsm_aswdS_trans_328,
   fsm_aswdS_trans_329,
   fsm_aswdS_trans_330,
   fsm_aswdS_trans_331,
   fsm_aswdS_trans_332,
   fsm_aswdS_trans_333,
   fsm_aswdS_trans_334,
   fsm_aswdS_trans_335,
   fsm_aswdS_trans_336,
   fsm_aswdS_trans_337,
   fsm_aswdS_trans_338,
   fsm_aswdS_trans_339,
   fsm_aswdS_trans_340,
   fsm_aswdS_trans_341,
   fsm_aswdS_trans_342,
   fsm_aswdS_trans_343,
   fsm_aswdS_trans_344,
   fsm_aswdS_trans_345,
   fsm_aswdS_trans_346,
   fsm_aswdS_trans_347,
   fsm_aswdS_trans_348,
   fsm_aswdS_trans_349,
   fsm_aswdS_trans_350,
   fsm_aswdS_trans_351,
   fsm_aswdS_trans_352,
   fsm_aswdS_trans_353,
   fsm_aswdS_trans_354,
   fsm_aswdS_trans_355,
   fsm_aswdS_trans_356,
   fsm_aswdS_trans_357,
   fsm_aswdS_trans_358,
   fsm_aswdS_trans_359,
   fsm_aswdS_trans_360,
   fsm_aswdS_trans_361,
   fsm_aswdS_trans_362,
   fsm_aswdS_trans_363,
   fsm_aswdS_trans_364,
   fsm_aswdS_trans_365,
   fsm_aswdS_trans_366,
   fsm_aswdS_trans_367,
   fsm_aswdS_trans_368,
   fsm_aswdS_trans_369,
   fsm_aswdS_trans_370,
   fsm_aswdS_trans_371,
   fsm_aswdS_trans_372,
   fsm_aswdS_trans_373,
   fsm_aswdS_trans_374,
   fsm_aswdS_trans_375,
   fsm_aswdS_trans_376,
   fsm_aswdS_trans_377,
   fsm_aswdS_trans_378,
   fsm_aswdS_trans_379,
   fsm_aswdS_trans_380,
   fsm_aswdS_trans_381,
   fsm_aswdS_trans_382,
   fsm_aswdS_trans_383,
   fsm_aswdS_trans_384,
   fsm_aswdS_trans_385,
   fsm_aswdS_trans_386,
   fsm_aswdS_trans_387,
   fsm_aswdS_trans_388,
   fsm_aswdS_trans_389,
   fsm_aswdS_trans_390,
   fsm_aswdS_trans_391,
   fsm_aswdS_trans_392,
   fsm_aswdS_trans_393,
   fsm_aswdS_trans_394,
   fsm_aswdS_trans_395,
   fsm_aswdS_trans_396,
   fsm_aswdS_trans_397,
   fsm_aswdS_trans_398,
   fsm_aswdS_trans_399,
   fsm_aswdS_trans_400,
   fsm_aswdS_trans_401,
   fsm_aswdS_trans_402,
   fsm_aswdS_trans_403,
   fsm_aswdS_trans_404,
   fsm_aswdS_trans_405,
   fsm_aswdS_trans_406,
   fsm_aswdS_trans_407,
   fsm_aswdS_trans_408,
   fsm_aswdS_trans_409,
   fsm_aswdS_trans_410,
   fsm_aswdS_trans_411,
   fsm_aswdS_trans_412,
   fsm_aswdS_trans_413,
   fsm_aswdS_trans_414,
   fsm_aswdS_trans_415,
   fsm_aswdS_trans_416,
   fsm_aswdS_trans_417,
   fsm_aswdS_trans_418,
   fsm_aswdS_trans_419,
   fsm_aswdS_trans_420,
   fsm_aswdS_trans_421,
   fsm_aswdS_trans_422,
   fsm_aswdS_trans_423,
   fsm_aswdS_trans_424,
   fsm_aswdS_trans_425,
   fsm_aswdS_trans_426,
   fsm_aswdS_trans_427,
   fsm_aswdS_trans_428,
   fsm_aswdS_trans_429,
   fsm_aswdS_trans_430,
   fsm_aswdS_trans_431,
   fsm_aswdS_trans_432,
   fsm_aswdS_trans_433,
   fsm_aswdS_trans_434,
   fsm_aswdS_trans_435,
   fsm_aswdS_trans_436,
   fsm_aswdS_trans_437,
   fsm_aswdS_trans_438,
   fsm_aswdS_trans_439,
   fsm_aswdS_trans_440,
   fsm_aswdS_trans_441,
   fsm_aswdS_trans_442,
   fsm_aswdS_trans_443,
   fsm_aswdS_trans_444,
   fsm_aswdS_trans_445,
   fsm_aswdS_trans_446,
   fsm_aswdS_trans_447,
   fsm_aswdS_trans_448,
   fsm_aswdS_trans_449,
   fsm_aswdS_trans_450,
   fsm_aswdS_trans_451,
   fsm_aswdS_trans_452,
   fsm_aswdS_trans_453
};
static const cst_fsm fsm_aswdS = {
   128, /* vocab size */
   454,  /* num_transitions */
   fsm_aswdS_trans
};


static int is_word_pre(const char *word)
{
    int i, state, symbol;

    state = fsm_transition(&fsm_aswdP,0,'#');

    for (i=0; word[i]; i++)
    {
	if ((word[i] == 'n') || ((word[i] == 'm')))
	    symbol = 'N';
	else if (strchr("aeiouy",word[i]) != NULL)
	    symbol = 'V';
	else
	    symbol = word[i];
	state = fsm_transition(&fsm_aswdP,state,symbol);
	if (state == -1)
	    return 0;
	else if (symbol == 'V')
	    return 1;
    }
    return 0;
}

static int is_word_suf(const char *word)
{
    int i, state, symbol;

    state = fsm_transition(&fsm_aswdP,0,'#');

    for (i=cst_strlen(word)-1; i >= 0 ; i--)
    {
	if ((word[i] == 'n') || ((word[i] == 'm')))
	    symbol = 'N';
	else if (strchr("aeiouy",word[i]) != NULL)
	    symbol = 'V';
	else
	    symbol = word[i];
	state = fsm_transition(&fsm_aswdS,state,symbol);
	if (state == -1)
	    return 0;
	else if (symbol == 'V')
	    return 1;
    }
    return 0;
}

int us_aswd(const char *word)
{
    /* returns 1 if this words looks like its pronouncable, 0 otherwise */
    char *dc;
    int i;

    dc = cst_downcase(word);

    i = is_word_pre(dc) && is_word_suf(dc);

    cst_free(dc);

    return i;
}
