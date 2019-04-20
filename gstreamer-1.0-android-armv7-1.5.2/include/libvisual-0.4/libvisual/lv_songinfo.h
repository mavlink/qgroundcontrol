/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_songinfo.h,v 1.14 2006/01/22 13:23:37 synap Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _LV_SONGINFO_H
#define _LV_SONGINFO_H

#include <libvisual/lv_time.h>
#include <libvisual/lv_video.h>

VISUAL_BEGIN_DECLS

#define VISUAL_SONGINFO(obj)				(VISUAL_CHECK_CAST ((obj), VisSongInfo))

/**
 * Used to define the type of song info being used.
 * There are two interfaces to notify libvisual of song
 * information, being a simple and a advanced interface.
 */
typedef enum {
	VISUAL_SONGINFO_TYPE_NULL,	/**< No song info is given. */
	VISUAL_SONGINFO_TYPE_SIMPLE,	/**< Simple interface only sets a songname. */
	VISUAL_SONGINFO_TYPE_ADVANCED	/**< Advanced interface splits all the stats
					  * in separated entries */
} VisSongInfoType;

typedef struct _VisSongInfo VisSongInfo;

/**
 * Song info data structure.
 *
 * Contains information about the current played song. Information like
 * artist, album, song, elapsed time and even coverart can be set using the
 * methods of the VisSongInfo system.
 */
struct _VisSongInfo {
	VisObject	 object;	/**< The VisObject data. */
	VisSongInfoType	 type;		/**< Sets the interface type. */

	/* In seconds */
	int		 length;	/**< Total length of the song playing. */
	int		 elapsed;	/**< Elapsed time of the song playing. */

	/* Simple type */
	char		*songname;	/**< A string containing the song name using
					  * the simple interface. */

	/* Advanced type */
	char		*artist;	/**< A string containing the artist name using
					  * the advanced interface. */
	char		*album;		/**< A string containing the album name using
					  * the advanced interface. */
	char		*song;		/**< A string containing the song name using
					  * the advanced interface. */

	/* Timing */
	VisTimer	 timer;		/**< Used to internal timing to keep track on the
					  * age of the record. */
	/* Cover art */
	VisVideo	*cover;		/**< Pointer to a VisVideo that contains the cover art. */
};

VisSongInfo *visual_songinfo_new (VisSongInfoType type);
int visual_songinfo_init (VisSongInfo *songinfo, VisSongInfoType type);

int visual_songinfo_free_strings (VisSongInfo *songinfo);

int visual_songinfo_set_type (VisSongInfo *songinfo, VisSongInfoType type);
int visual_songinfo_set_length (VisSongInfo *songinfo, int length);
int visual_songinfo_set_elapsed (VisSongInfo *songinfo, int elapsed);
int visual_songinfo_set_simple_name (VisSongInfo *songinfo, char *name);
int visual_songinfo_set_artist (VisSongInfo *songinfo, char *artist);
int visual_songinfo_set_album (VisSongInfo *songinfo, char *album);
int visual_songinfo_set_song (VisSongInfo *songinfo, char *song);
int visual_songinfo_set_cover (VisSongInfo *songinfo, VisVideo *cover);

int visual_songinfo_mark (VisSongInfo *songinfo);
long visual_songinfo_age (VisSongInfo *songinfo);
int visual_songinfo_copy (VisSongInfo *dest, VisSongInfo *src);
int visual_songinfo_compare (VisSongInfo *s1, VisSongInfo *s2);

VISUAL_END_DECLS

#endif /* _LV_SONGINFO_H */
