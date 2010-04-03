/*************************************************************************/
/*                                                                       */
/*                  Language Technologies Institute                      */
/*                     Carnegie Mellon University                        */
/*                      Copyright (c) 2001-2008                          */
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
/*               Date:  August 2008                                      */
/*************************************************************************/
/*  Moved the CMU lexicon specific postlexical rules into cmulex itself  */
/*************************************************************************/

#include "flite.h"

static void apostrophe_s(cst_utterance *u)
{
    cst_item *s;
    cst_item *schwa;
    cst_phoneset *ps=0;
    const char *pname, *word;

    ps = val_phoneset(feat_val(u->features,"phoneset"));

    for (s=item_next(relation_head(utt_relation(u,"Segment"))); 
	 s; s=item_next(s))
    {
	word = val_string(ffeature(s, "R:SylStructure.parent.parent.name"));
	if (cst_streq("'s", word))
	{
	    pname = item_feat_string(item_prev(s),"name");
	    if ((strchr("fa",*phone_feature_string(ps,pname,"ctype")) != NULL)
		&& (strchr("dbg",
			   *phone_feature_string(ps,pname,"cplace")) == NULL))
		/* needs a schwa */
	    {
		schwa = item_prepend(s,NULL);
		item_set_string(schwa,"name","ax");
		item_prepend(item_as(s,"SylStructure"),schwa);
	    }
	    else if (cst_streq("-",phone_feature_string(ps,pname,"cvox")))
		item_set_string(s,"name","s");
	}
	else if (cst_streq("'ve", word)
		 || cst_streq("'ll", word)
		 || cst_streq("'d", word))
	{
	    if (cst_streq("-",ffeature_string(s,"p.ph_vc")))
	    {
		schwa = item_prepend(s,NULL);
		item_set_string(schwa,"name","ax");
		item_prepend(item_as(s,"SylStructure"),schwa);
	    }
	}
    }

}

static void the_iy_ax(cst_utterance *u)
{
    const cst_item *i;
    const char *word;

    for (i = relation_head(utt_relation(u, "Segment")); i; i = item_next(i))
    {
	if (cst_streq("ax", item_name(i)))
	{
	    word = ffeature_string(i,"R:SylStructure.parent.parent.name");
	    if (cst_streq("the", word)
		&& cst_streq("+", ffeature_string(i,"n.ph_vc")))
		    item_set_string(i, "name", "iy");
	}

    }
}

cst_utterance *cmu_postlex(cst_utterance *u)
{
    /* Post lexical rules */

    apostrophe_s(u);
    the_iy_ax(u);

    return u;
}
