/* GStreamer Matroska muxer/demuxer
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * matroska-ids.h: matroska file/stream data IDs
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __GST_MATROSKA_IDS_H__
#define __GST_MATROSKA_IDS_H__

#include <gst/gst.h>
#include <gst/video/video-info.h>

#include "ebml-ids.h"

/*
 * EBML DocType.
 */

#define GST_MATROSKA_DOCTYPE_MATROSKA              "matroska"
#define GST_MATROSKA_DOCTYPE_WEBM                  "webm"

/*
 * Matroska element IDs. max. 32-bit.
 */

/* toplevel Segment */
#define GST_MATROSKA_ID_SEGMENT                    0x18538067

/* matroska top-level master IDs, childs of Segment */
#define GST_MATROSKA_ID_SEGMENTINFO                0x1549A966
#define GST_MATROSKA_ID_TRACKS                     0x1654AE6B
#define GST_MATROSKA_ID_CUES                       0x1C53BB6B
#define GST_MATROSKA_ID_TAGS                       0x1254C367
#define GST_MATROSKA_ID_SEEKHEAD                   0x114D9B74
#define GST_MATROSKA_ID_CLUSTER                    0x1F43B675
#define GST_MATROSKA_ID_ATTACHMENTS                0x1941A469
#define GST_MATROSKA_ID_CHAPTERS                   0x1043A770

/* IDs in the SegmentInfo master */
#define GST_MATROSKA_ID_TIMECODESCALE              0x2AD7B1
#define GST_MATROSKA_ID_DURATION                   0x4489
#define GST_MATROSKA_ID_WRITINGAPP                 0x5741
#define GST_MATROSKA_ID_MUXINGAPP                  0x4D80
#define GST_MATROSKA_ID_DATEUTC                    0x4461
#define GST_MATROSKA_ID_SEGMENTUID                 0x73A4
#define GST_MATROSKA_ID_SEGMENTFILENAME            0x7384
#define GST_MATROSKA_ID_PREVUID                    0x3CB923
#define GST_MATROSKA_ID_PREVFILENAME               0x3C83AB
#define GST_MATROSKA_ID_NEXTUID                    0x3EB923
#define GST_MATROSKA_ID_NEXTFILENAME               0x3E83BB
#define GST_MATROSKA_ID_TITLE                      0x7BA9
#define GST_MATROSKA_ID_SEGMENTFAMILY              0x4444
#define GST_MATROSKA_ID_CHAPTERTRANSLATE           0x6924

/* IDs in the ChapterTranslate master */
#define GST_MATROSKA_ID_CHAPTERTRANSLATEEDITIONUID 0x69FC
#define GST_MATROSKA_ID_CHAPTERTRANSLATECODEC      0x69BF
#define GST_MATROSKA_ID_CHAPTERTRANSLATEID         0x69A5

/* ID in the Tracks master */
#define GST_MATROSKA_ID_TRACKENTRY                 0xAE

/* IDs in the TrackEntry master */
#define GST_MATROSKA_ID_TRACKNUMBER                0xD7
#define GST_MATROSKA_ID_TRACKUID                   0x73C5
#define GST_MATROSKA_ID_TRACKTYPE                  0x83
#define GST_MATROSKA_ID_TRACKAUDIO                 0xE1
#define GST_MATROSKA_ID_TRACKVIDEO                 0xE0
#define GST_MATROSKA_ID_CONTENTENCODINGS           0x6D80
#define GST_MATROSKA_ID_CODECID                    0x86
#define GST_MATROSKA_ID_CODECPRIVATE               0x63A2
#define GST_MATROSKA_ID_CODECNAME                  0x258688
#define GST_MATROSKA_ID_TRACKNAME                  0x536E
#define GST_MATROSKA_ID_TRACKLANGUAGE              0x22B59C
#define GST_MATROSKA_ID_TRACKFLAGENABLED           0xB9
#define GST_MATROSKA_ID_TRACKFLAGDEFAULT           0x88
#define GST_MATROSKA_ID_TRACKFLAGFORCED            0x55AA
#define GST_MATROSKA_ID_TRACKFLAGLACING            0x9C
#define GST_MATROSKA_ID_TRACKMINCACHE              0x6DE7
#define GST_MATROSKA_ID_TRACKMAXCACHE              0x6DF8
#define GST_MATROSKA_ID_TRACKDEFAULTDURATION       0x23E383
#define GST_MATROSKA_ID_TRACKTIMECODESCALE         0x23314F
#define GST_MATROSKA_ID_MAXBLOCKADDITIONID         0x55EE
#define GST_MATROSKA_ID_TRACKATTACHMENTLINK        0x7446
#define GST_MATROSKA_ID_TRACKOVERLAY               0x6FAB
#define GST_MATROSKA_ID_TRACKTRANSLATE             0x6624
/* semi-draft */
#define GST_MATROSKA_ID_TRACKOFFSET                0x537F
/* semi-draft */
#define GST_MATROSKA_ID_CODECSETTINGS              0x3A9697
/* semi-draft */
#define GST_MATROSKA_ID_CODECINFOURL               0x3B4040
/* semi-draft */
#define GST_MATROSKA_ID_CODECDOWNLOADURL           0x26B240
/* semi-draft */
#define GST_MATROSKA_ID_CODECDECODEALL             0xAA
#define GST_MATROSKA_ID_SEEKPREROLL                0x56BB
#define GST_MATROSKA_ID_CODECDELAY                 0x56AA

/* IDs in the TrackTranslate master */
#define GST_MATROSKA_ID_TRACKTRANSLATEEDITIONUID   0x66FC
#define GST_MATROSKA_ID_TRACKTRANSLATECODEC        0x66BF
#define GST_MATROSKA_ID_TRACKTRANSLATETRACKID      0x66A5


/* IDs in the TrackVideo master */
/* NOTE: This one is here only for backward compatibility.
 * Use _TRACKDEFAULDURATION */
#define GST_MATROSKA_ID_VIDEOFRAMERATE             0x2383E3
#define GST_MATROSKA_ID_VIDEODISPLAYWIDTH          0x54B0
#define GST_MATROSKA_ID_VIDEODISPLAYHEIGHT         0x54BA
#define GST_MATROSKA_ID_VIDEODISPLAYUNIT           0x54B2
#define GST_MATROSKA_ID_VIDEOPIXELWIDTH            0xB0
#define GST_MATROSKA_ID_VIDEOPIXELHEIGHT           0xBA
#define GST_MATROSKA_ID_VIDEOPIXELCROPBOTTOM       0x54AA
#define GST_MATROSKA_ID_VIDEOPIXELCROPTOP          0x54BB
#define GST_MATROSKA_ID_VIDEOPIXELCROPLEFT         0x54CC
#define GST_MATROSKA_ID_VIDEOPIXELCROPRIGHT        0x54DD
#define GST_MATROSKA_ID_VIDEOFLAGINTERLACED        0x9A
#define GST_MATROSKA_ID_VIDEOFIELDORDER            0x9D
/* semi-draft */
#define GST_MATROSKA_ID_VIDEOSTEREOMODE            0x53B8
#define GST_MATROSKA_ID_VIDEOASPECTRATIOTYPE       0x54B3
#define GST_MATROSKA_ID_VIDEOCOLOURSPACE           0x2EB524
/* semi-draft */
#define GST_MATROSKA_ID_VIDEOGAMMAVALUE            0x2FB523

#define GST_MATROSKA_ID_VIDEOCOLOUR                0x55B0
/* IDs in the Colour master*/
#define GST_MATROSKA_ID_VIDEOMATRIXCOEFFICIENTS    0x55B1
#define GST_MATROSKA_ID_VIDEORANGE                 0x55B9
#define GST_MATROSKA_ID_VIDEOTRANSFERCHARACTERISTICS  0x55BA
#define GST_MATROSKA_ID_VIDEOPRIMARIES             0x55BB
#define GST_MATROSKA_ID_MAXCLL                     0x55BC
#define GST_MATROSKA_ID_MAXFALL                    0x55BD
#define GST_MATROSKA_ID_MASTERINGMETADATA          0x55D0
/* IDs in the MasteringMetadata */
#define GST_MATROSKA_ID_PRIMARYRCHROMATICITYX      0x55D1
#define GST_MATROSKA_ID_PRIMARYRCHROMATICITYY      0x55D2
#define GST_MATROSKA_ID_PRIMARYGCHROMATICITYX      0x55D3
#define GST_MATROSKA_ID_PRIMARYGCHROMATICITYY      0x55D4
#define GST_MATROSKA_ID_PRIMARYBCHROMATICITYX      0x55D5
#define GST_MATROSKA_ID_PRIMARYBCHROMATICITYY      0x55D6
#define GST_MATROSKA_ID_WHITEPOINTCHROMATICITYX    0x55D7
#define GST_MATROSKA_ID_WHITEPOINTCHROMATICITYY    0x55D8
#define GST_MATROSKA_ID_LUMINANCEMAX               0x55D9
#define GST_MATROSKA_ID_LUMINANCEMIN               0x55DA

/* IDs in the TrackAudio master */
#define GST_MATROSKA_ID_AUDIOSAMPLINGFREQ          0xB5
#define GST_MATROSKA_ID_AUDIOBITDEPTH              0x6264
#define GST_MATROSKA_ID_AUDIOCHANNELS              0x9F
/* semi-draft */
#define GST_MATROSKA_ID_AUDIOCHANNELPOSITIONS      0x7D7B
#define GST_MATROSKA_ID_AUDIOOUTPUTSAMPLINGFREQ    0x78B5

/* IDs in the TrackContentEncoding master */
#define GST_MATROSKA_ID_CONTENTENCODING            0x6240

/* IDs in the ContentEncoding master */
#define GST_MATROSKA_ID_CONTENTENCODINGORDER       0x5031
#define GST_MATROSKA_ID_CONTENTENCODINGSCOPE       0x5032
#define GST_MATROSKA_ID_CONTENTENCODINGTYPE        0x5033
#define GST_MATROSKA_ID_CONTENTCOMPRESSION         0x5034
#define GST_MATROSKA_ID_CONTENTENCRYPTION          0x5035

/* IDs in the ContentCompression master */
#define GST_MATROSKA_ID_CONTENTCOMPALGO            0x4254
#define GST_MATROSKA_ID_CONTENTCOMPSETTINGS        0x4255

/* IDs in the ContentEncryption master */
#define GST_MATROSKA_ID_CONTENTENCALGO             0x47E1
#define GST_MATROSKA_ID_CONTENTENCKEYID            0x47E2
#define GST_MATROSKA_ID_CONTENTSIGNATURE           0x47E3
#define GST_MATROSKA_ID_CONTENTSIGKEYID            0x47E4
#define GST_MATROSKA_ID_CONTENTSIGALGO             0x47E5
#define GST_MATROSKA_ID_CONTENTSIGHASHALGO         0x47E6
/* Added in WebM spec */
#define GST_MATROSKA_ID_CONTENTENCAESSETTINGS      0x47E7
#define GST_MATROSKA_ID_AESSETTINGSCIPHERMODE      0x47E8

/* ID in the CUEs master */
#define GST_MATROSKA_ID_POINTENTRY                 0xBB

/* IDs in the pointentry master */
#define GST_MATROSKA_ID_CUETIME                    0xB3
#define GST_MATROSKA_ID_CUETRACKPOSITIONS          0xB7

/* IDs in the CueTrackPositions master */
#define GST_MATROSKA_ID_CUETRACK                   0xF7
#define GST_MATROSKA_ID_CUECLUSTERPOSITION         0xF1
#define GST_MATROSKA_ID_CUEBLOCKNUMBER             0x5378
/* semi-draft */
#define GST_MATROSKA_ID_CUECODECSTATE              0xEA
/* semi-draft */
#define GST_MATROSKA_ID_CUEREFERENCE               0xDB

/* IDs in the CueReference master */
/* semi-draft */
#define GST_MATROSKA_ID_CUEREFTIME                 0x96
/* semi-draft */
#define GST_MATROSKA_ID_CUEREFCLUSTER              0x97
/* semi-draft */
#define GST_MATROSKA_ID_CUEREFNUMBER               0x535F
/* semi-draft */
#define GST_MATROSKA_ID_CUEREFCODECSTATE           0xEB

/* IDs in the Tags master */
#define GST_MATROSKA_ID_TAG                        0x7373

/* in the Tag master */
#define GST_MATROSKA_ID_SIMPLETAG                  0x67C8
#define GST_MATROSKA_ID_TARGETS                    0x63C0

/* in the SimpleTag master */
#define GST_MATROSKA_ID_TAGNAME                    0x45A3
#define GST_MATROSKA_ID_TAGSTRING                  0x4487
#define GST_MATROSKA_ID_TAGLANGUAGE                0x447A
#define GST_MATROSKA_ID_TAGDEFAULT                 0x4484
#define GST_MATROSKA_ID_TAGBINARY                  0x4485

/* in the Targets master */
#define GST_MATROSKA_ID_TARGETTYPEVALUE            0x68CA
#define GST_MATROSKA_ID_TARGETTYPE                 0x63CA
#define GST_MATROSKA_ID_TARGETTRACKUID             0x63C5
#define GST_MATROSKA_ID_TARGETEDITIONUID           0x63C9
#define GST_MATROSKA_ID_TARGETCHAPTERUID           0x63C4
#define GST_MATROSKA_ID_TARGETATTACHMENTUID        0x63C6

/* IDs in the SeekHead master */
#define GST_MATROSKA_ID_SEEKENTRY                  0x4DBB

/* IDs in the SeekEntry master */
#define GST_MATROSKA_ID_SEEKID                     0x53AB
#define GST_MATROSKA_ID_SEEKPOSITION               0x53AC

/* IDs in the Cluster master */
#define GST_MATROSKA_ID_CLUSTERTIMECODE            0xE7
#define GST_MATROSKA_ID_BLOCKGROUP                 0xA0
#define GST_MATROSKA_ID_SIMPLEBLOCK                0xA3
#define GST_MATROSKA_ID_REFERENCEBLOCK             0xFB
#define GST_MATROSKA_ID_POSITION                   0xA7
#define GST_MATROSKA_ID_PREVSIZE                   0xAB
/* semi-draft */
#define GST_MATROSKA_ID_ENCRYPTEDBLOCK             0xAF
#define GST_MATROSKA_ID_SILENTTRACKS               0x5854

/* IDs in the SilentTracks master */
#define GST_MATROSKA_ID_SILENTTRACKNUMBER          0x58D7

/* IDs in the BlockGroup master */
#define GST_MATROSKA_ID_BLOCK                      0xA1
#define GST_MATROSKA_ID_BLOCKDURATION              0x9B
/* semi-draft */
#define GST_MATROSKA_ID_BLOCKVIRTUAL               0xA2
#define GST_MATROSKA_ID_REFERENCEBLOCK             0xFB
#define GST_MATROSKA_ID_BLOCKADDITIONS             0x75A1
#define GST_MATROSKA_ID_REFERENCEPRIORITY          0xFA
/* semi-draft */
#define GST_MATROSKA_ID_REFERENCEVIRTUAL           0xFD
/* semi-draft */
#define GST_MATROSKA_ID_CODECSTATE                 0xA4
#define GST_MATROSKA_ID_SLICES                     0x8E
#define GST_MATROSKA_ID_DISCARDPADDING             0x75A2

/* IDs in the BlockAdditions master */
#define GST_MATROSKA_ID_BLOCKMORE                  0xA6

/* IDs in the BlockMore master */
#define GST_MATROSKA_ID_BLOCKADDID                 0xEE
#define GST_MATROSKA_ID_BLOCKADDITIONAL            0xA5

/* IDs in the Slices master */
#define GST_MATROSKA_ID_TIMESLICE                  0xE8

/* IDs in the TimeSlice master */
#define GST_MATROSKA_ID_LACENUMBER                 0xCC
/* semi-draft */
#define GST_MATROSKA_ID_FRAMENUMBER                0xCD
/* semi-draft */
#define GST_MATROSKA_ID_BLOCKADDITIONID            0xCB
/* semi-draft */
#define GST_MATROSKA_ID_TIMESLICEDELAY             0xCE
#define GST_MATROSKA_ID_TIMESLICEDURATION          0xCF

/* IDs in the Attachments master */
#define GST_MATROSKA_ID_ATTACHEDFILE               0x61A7

/* IDs in the AttachedFile master */
#define GST_MATROSKA_ID_FILEDESCRIPTION            0x467E
#define GST_MATROSKA_ID_FILENAME                   0x466E
#define GST_MATROSKA_ID_FILEMIMETYPE               0x4660
#define GST_MATROSKA_ID_FILEDATA                   0x465C
#define GST_MATROSKA_ID_FILEUID                    0x46AE
/* semi-draft */
#define GST_MATROSKA_ID_FILEREFERRAL               0x4675

/* IDs in the Chapters master */
#define GST_MATROSKA_ID_EDITIONENTRY               0x45B9

/* IDs in the EditionEntry master */
#define GST_MATROSKA_ID_EDITIONUID                 0x45BC
#define GST_MATROSKA_ID_EDITIONFLAGHIDDEN          0x45BD
#define GST_MATROSKA_ID_EDITIONFLAGDEFAULT         0x45DB
#define GST_MATROSKA_ID_EDITIONFLAGORDERED         0x45DD
#define GST_MATROSKA_ID_CHAPTERATOM                0xB6

/* IDs in the ChapterAtom master */
#define GST_MATROSKA_ID_CHAPTERUID                 0x73C4
#define GST_MATROSKA_ID_CHAPTERSTRINGUID           0x5654
#define GST_MATROSKA_ID_CHAPTERTIMESTART           0x91
#define GST_MATROSKA_ID_CHAPTERTIMESTOP            0x92
#define GST_MATROSKA_ID_CHAPTERFLAGHIDDEN          0x98
#define GST_MATROSKA_ID_CHAPTERFLAGENABLED         0x4598
#define GST_MATROSKA_ID_CHAPTERSEGMENTUID          0x6E67
#define GST_MATROSKA_ID_CHAPTERSEGMENTEDITIONUID   0x6EBC
#define GST_MATROSKA_ID_CHAPTERPHYSICALEQUIV       0x63C3
#define GST_MATROSKA_ID_CHAPTERTRACK               0x8F
#define GST_MATROSKA_ID_CHAPTERDISPLAY             0x80
#define GST_MATROSKA_ID_CHAPPROCESS                0x6944

/* IDs in the ChapProcess master */
#define GST_MATROSKA_ID_CHAPPROCESSCODECID         0x6955
#define GST_MATROSKA_ID_CHAPPROCESSPRIVATE         0x450D
#define GST_MATROSKA_ID_CHAPPROCESSCOMMAND         0x6911

/* IDs in the ChapProcessCommand master */
#define GST_MATROSKA_ID_CHAPPROCESSTIME            0x6922
#define GST_MATROSKA_ID_CHAPPROCESSDATA            0x6933

/* IDs in the ChapterDisplay master */
#define GST_MATROSKA_ID_CHAPSTRING                 0x85
#define GST_MATROSKA_ID_CHAPLANGUAGE               0x437C
#define GST_MATROSKA_ID_CHAPCOUNTRY                0x437E

/* IDs in the ChapterTrack master */
#define GST_MATROSKA_ID_CHAPTERTRACKNUMBER         0x89

/*
 * Matroska Codec IDs. Strings.
 */

#define GST_MATROSKA_CODEC_ID_VIDEO_VFW_FOURCC   "V_MS/VFW/FOURCC"
#define GST_MATROSKA_CODEC_ID_VIDEO_UNCOMPRESSED "V_UNCOMPRESSED"
#define GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_SP     "V_MPEG4/ISO/SP"
#define GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_ASP    "V_MPEG4/ISO/ASP"
#define GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_AP     "V_MPEG4/ISO/AP"
#define GST_MATROSKA_CODEC_ID_VIDEO_MPEG4_AVC    "V_MPEG4/ISO/AVC"
#define GST_MATROSKA_CODEC_ID_VIDEO_MSMPEG4V3    "V_MPEG4/MS/V3"
#define GST_MATROSKA_CODEC_ID_VIDEO_MPEG1        "V_MPEG1"
#define GST_MATROSKA_CODEC_ID_VIDEO_MPEG2        "V_MPEG2"
/* FIXME: not (yet) in the spec! */
#define GST_MATROSKA_CODEC_ID_VIDEO_MJPEG        "V_MJPEG"
#define GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO1   "V_REAL/RV10"
#define GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO2   "V_REAL/RV20"
#define GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO3   "V_REAL/RV30"
#define GST_MATROSKA_CODEC_ID_VIDEO_REALVIDEO4   "V_REAL/RV40"
#define GST_MATROSKA_CODEC_ID_VIDEO_THEORA       "V_THEORA"
#define GST_MATROSKA_CODEC_ID_VIDEO_QUICKTIME    "V_QUICKTIME"
#define GST_MATROSKA_CODEC_ID_VIDEO_SNOW         "V_SNOW"
#define GST_MATROSKA_CODEC_ID_VIDEO_DIRAC        "V_DIRAC"
#define GST_MATROSKA_CODEC_ID_VIDEO_VP8          "V_VP8"
#define GST_MATROSKA_CODEC_ID_VIDEO_VP9          "V_VP9"
#define GST_MATROSKA_CODEC_ID_VIDEO_AV1          "V_AV1"
#define GST_MATROSKA_CODEC_ID_VIDEO_MPEGH_HEVC   "V_MPEGH/ISO/HEVC"
#define GST_MATROSKA_CODEC_ID_VIDEO_PRORES       "V_PRORES"

#define GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L1       "A_MPEG/L1"
#define GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L2       "A_MPEG/L2"
#define GST_MATROSKA_CODEC_ID_AUDIO_MPEG1_L3       "A_MPEG/L3"
#define GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_BE     "A_PCM/INT/BIG"
#define GST_MATROSKA_CODEC_ID_AUDIO_PCM_INT_LE     "A_PCM/INT/LIT"
#define GST_MATROSKA_CODEC_ID_AUDIO_PCM_FLOAT      "A_PCM/FLOAT/IEEE"
#define GST_MATROSKA_CODEC_ID_AUDIO_AC3            "A_AC3"
#define GST_MATROSKA_CODEC_ID_AUDIO_AC3_BSID9      "A_AC3/BSID9"
#define GST_MATROSKA_CODEC_ID_AUDIO_AC3_BSID10     "A_AC3/BSID10"
#define GST_MATROSKA_CODEC_ID_AUDIO_EAC3           "A_EAC3"
#define GST_MATROSKA_CODEC_ID_AUDIO_TRUEHD         "A_TRUEHD"
#define GST_MATROSKA_CODEC_ID_AUDIO_DTS            "A_DTS"
#define GST_MATROSKA_CODEC_ID_AUDIO_VORBIS         "A_VORBIS"
#define GST_MATROSKA_CODEC_ID_AUDIO_FLAC           "A_FLAC"
/* FIXME: not yet in the spec */
#define GST_MATROSKA_CODEC_ID_AUDIO_SPEEX          "A_SPEEX"
#define GST_MATROSKA_CODEC_ID_AUDIO_ACM            "A_MS/ACM"
#define GST_MATROSKA_CODEC_ID_AUDIO_TTA            "A_TTA1"
#define GST_MATROSKA_CODEC_ID_AUDIO_WAVPACK4       "A_WAVPACK4"
#define GST_MATROSKA_CODEC_ID_AUDIO_REAL_14_4      "A_REAL/14_4"
#define GST_MATROSKA_CODEC_ID_AUDIO_REAL_28_8      "A_REAL/28_8"
#define GST_MATROSKA_CODEC_ID_AUDIO_REAL_COOK      "A_REAL/COOK"
#define GST_MATROSKA_CODEC_ID_AUDIO_REAL_SIPR      "A_REAL/SIPR"
#define GST_MATROSKA_CODEC_ID_AUDIO_REAL_RALF      "A_REAL/RALF"
#define GST_MATROSKA_CODEC_ID_AUDIO_REAL_ATRC      "A_REAL/ATRC"
#define GST_MATROSKA_CODEC_ID_AUDIO_AAC            "A_AAC"
#define GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG2      "A_AAC/MPEG2/"
#define GST_MATROSKA_CODEC_ID_AUDIO_AAC_MPEG4      "A_AAC/MPEG4/"
#define GST_MATROSKA_CODEC_ID_AUDIO_QUICKTIME_QDMC "A_QUICKTIME/QDMC"
#define GST_MATROSKA_CODEC_ID_AUDIO_QUICKTIME_QDM2 "A_QUICKTIME/QDM2"
#define GST_MATROSKA_CODEC_ID_AUDIO_OPUS           "A_OPUS"
/* Undefined for now:
#define GST_MATROSKA_CODEC_ID_AUDIO_MPC            "A_MPC"
*/

#define GST_MATROSKA_CODEC_ID_SUBTITLE_ASCII     "S_TEXT/ASCII"
#define GST_MATROSKA_CODEC_ID_SUBTITLE_UTF8      "S_TEXT/UTF8"
#define GST_MATROSKA_CODEC_ID_SUBTITLE_SSA       "S_TEXT/SSA"
#define GST_MATROSKA_CODEC_ID_SUBTITLE_ASS       "S_TEXT/ASS"
#define GST_MATROSKA_CODEC_ID_SUBTITLE_USF       "S_TEXT/USF"
#define GST_MATROSKA_CODEC_ID_SUBTITLE_VOBSUB    "S_VOBSUB"
#define GST_MATROSKA_CODEC_ID_SUBTITLE_HDMVPGS   "S_HDMV/PGS"
#define GST_MATROSKA_CODEC_ID_SUBTITLE_BMP       "S_IMAGE/BMP"
#define GST_MATROSKA_CODEC_ID_SUBTITLE_KATE      "S_KATE"

/*
 * Matroska tags. Strings.
 */

#define GST_MATROSKA_TAG_ID_TITLE    "TITLE"
#define GST_MATROSKA_TAG_ID_AUTHOR   "AUTHOR"
#define GST_MATROSKA_TAG_ID_ARTIST   "ARTIST"
#define GST_MATROSKA_TAG_ID_ALBUM    "ALBUM"
#define GST_MATROSKA_TAG_ID_COMMENTS "COMMENTS"
#define GST_MATROSKA_TAG_ID_COMMENT  "COMMENT"
#define GST_MATROSKA_TAG_ID_BITSPS   "BITSPS"
#define GST_MATROSKA_TAG_ID_BPS      "BPS"
#define GST_MATROSKA_TAG_ID_ENCODER  "ENCODER"
#define GST_MATROSKA_TAG_ID_ISRC     "ISRC"
#define GST_MATROSKA_TAG_ID_COPYRIGHT "COPYRIGHT"
#define GST_MATROSKA_TAG_ID_BPM       "BPM"
#define GST_MATROSKA_TAG_ID_TERMS_OF_USE "TERMS_OF_USE"
#define GST_MATROSKA_TAG_ID_DATE      "DATE"
#define GST_MATROSKA_TAG_ID_COMPOSER  "COMPOSER"
#define GST_MATROSKA_TAG_ID_LEAD_PERFORMER  "LEAD_PERFOMER"
#define GST_MATROSKA_TAG_ID_GENRE     "GENRE"
#define GST_MATROSKA_TAG_ID_TOTAL_PARTS "TOTAL_PARTS"
#define GST_MATROSKA_TAG_ID_PART_NUMBER "PART_NUMBER"
#define GST_MATROSKA_TAG_ID_SUBTITLE "SUBTITLE"
#define GST_MATROSKA_TAG_ID_ACCOMPANIMENT "ACCOMPANIMENT"
#define GST_MATROSKA_TAG_ID_LYRICS "LYRICS"
#define GST_MATROSKA_TAG_ID_CONDUCTOR "CONDUCTOR"
#define GST_MATROSKA_TAG_ID_ENCODED_BY "ENCODED_BY"
#define GST_MATROSKA_TAG_ID_DESCRIPTION "DESCRIPTION"
#define GST_MATROSKA_TAG_ID_KEYWORDS "KEYWORDS"
#define GST_MATROSKA_TAG_ID_DATE_RELEASED "DATE_RELEASED"
#define GST_MATROSKA_TAG_ID_DATE_RECORDED "DATE_RECORDED"
#define GST_MATROSKA_TAG_ID_DATE_ENCODED "DATE_ENCODED"
#define GST_MATROSKA_TAG_ID_DATE_TAGGED "DATE_TAGGED"
#define GST_MATROSKA_TAG_ID_DATE_DIGITIZED "DATE_DIGITIZED"
#define GST_MATROSKA_TAG_ID_DATE_WRITTEN "DATE_WRITTEN"
#define GST_MATROSKA_TAG_ID_DATE_PURCHASED "DATE_PURCHASED"
#define GST_MATROSKA_TAG_ID_RECORDING_LOCATION "RECORDING_LOCATION"
#define GST_MATROSKA_TAG_ID_PRODUCTION_COPYRIGHT "PRODUCTION_COPYRIGHT"
#define GST_MATROSKA_TAG_ID_LICENSE "LICENSE"

/*
 * TODO: add this tag & mappings
 * "REPLAYGAIN_GAIN" -> GST_TAG_*_GAIN   see http://replaygain.hydrogenaudio.org/rg_data_format.html
 * "REPLAYGAIN_PEAK" -> GST_TAG_*_PEAK   see http://replaygain.hydrogenaudio.org/peak_data_format.html
 * both are depending on the target (track, album?)
 *
 * "TOTAL_PARTS" -> GST_TAG_TRACK_COUNT    depending on target
 * "PART_NUMBER" -> GST_TAG_TRACK_NUMBER   depending on target
 *
 * "SORT_WITH" -> nested in other elements, GST_TAG_TITLE_SORTNAME, etc
 *
 * TODO: maybe add custom gstreamer tags for other standard matroska tags,
 * see http://matroska.org/technical/specs/tagging/index.html
 *
 * TODO: handle tag targets and nesting correctly
 */

/*
 * Enumerations for various types (mapping from binary
 * value to what it actually means).
 */

typedef enum {
  GST_MATROSKA_TRACK_TYPE_VIDEO    = 0x1,
  GST_MATROSKA_TRACK_TYPE_AUDIO    = 0x2,
  GST_MATROSKA_TRACK_TYPE_COMPLEX  = 0x3,
  GST_MATROSKA_TRACK_TYPE_LOGO     = 0x10,
  GST_MATROSKA_TRACK_TYPE_SUBTITLE = 0x11,
  GST_MATROSKA_TRACK_TYPE_BUTTONS  = 0x12,
  GST_MATROSKA_TRACK_TYPE_CONTROL  = 0x20,
} GstMatroskaTrackType;

typedef enum {
  GST_MATROSKA_ASPECT_RATIO_MODE_FREE  = 0x0,
  GST_MATROSKA_ASPECT_RATIO_MODE_KEEP  = 0x1,
  GST_MATROSKA_ASPECT_RATIO_MODE_FIXED = 0x2,
} GstMatroskaAspectRatioMode;

/*
 * These aren't in any way "matroska-form" things,
 * it's just something I use in the muxer/demuxer.
 */

typedef enum {
  GST_MATROSKA_TRACK_ENABLED = (1<<0),
  GST_MATROSKA_TRACK_DEFAULT = (1<<1),
  GST_MATROSKA_TRACK_LACING  = (1<<2),
  GST_MATROSKA_TRACK_FORCED  = (1<<3),
  GST_MATROSKA_TRACK_SHIFT   = (1<<16)
} GstMatroskaTrackFlags;

typedef enum {
  GST_MATROSKA_INTERLACE_MODE_UNKNOWN = 0,
  GST_MATROSKA_INTERLACE_MODE_INTERLACED = 1,
  GST_MATROSKA_INTERLACE_MODE_PROGRESSIVE = 2,
} GstMatroskaInterlaceMode;

typedef enum {
  GST_MATROSKA_STEREO_MODE_SBS_LR      = 0x1,
  GST_MATROSKA_STEREO_MODE_TB_RL       = 0x2,
  GST_MATROSKA_STEREO_MODE_TB_LR       = 0x3,
  GST_MATROSKA_STEREO_MODE_CHECKER_RL  = 0x4,
  GST_MATROSKA_STEREO_MODE_CHECKER_LR  = 0x5,
  GST_MATROSKA_STEREO_MODE_SBS_RL      = 0x9,
  GST_MATROSKA_STEREO_MODE_FBF_LR      = 0xD,
  GST_MATROSKA_STEREO_MODE_FBF_RL      = 0xE
} GstMatroskaStereoMode;

typedef enum {
  GST_MATROSKA_ENCODING_COMPRESSION = 0x00,
  GST_MATROSKA_ENCODING_ENCRYPTION  = 0x01
} GstMatroskaEncodingType;

/* WebM spec */
typedef enum {
  GST_MATROSKA_BLOCK_ENCRYPTED   = 0x01,
  GST_MATROSKA_BLOCK_PARTITIONED = 0x02
} GstMatroskaEncryptedBlockFlags;

typedef struct _GstMatroskaTrackContext GstMatroskaTrackContext;

/* TODO: check if all fields are used */
struct _GstMatroskaTrackContext {
  GstPad       *pad;
  GstCaps      *caps;
  guint         index;
  /* reverse playback */
  GstClockTime  from_time;
  gint64                   from_offset;
  gint64                   to_offset;

  GArray       *index_table;

  gint          index_writer_id;

  /* some often-used info */
  gchar        *codec_id, *codec_name, *name, *language;
  gpointer      codec_priv;
  gsize         codec_priv_size;
  gpointer      codec_state;
  gsize         codec_state_size;
  GstMatroskaTrackType type;
  guint64       uid, num;
  GstMatroskaTrackFlags flags;
  guint64       default_duration;
  guint64       pos;
  gdouble       timecodescale;
  guint64       seek_preroll;
  guint64       codec_delay;

  gboolean      set_discont; /* TRUE = set DISCONT flag on next buffer */

  /* Queue to save the GST_PROTECTION events which will be sent before the first source buffer */
  GQueue         protection_event_queue;
  /* Protection information structure which will be added in protection metadata for each encrypted buffer */
  GstStructure * protection_info;

  /* Stream header buffer, to put into caps and send before any other buffers */
  GstBufferList * stream_headers;
  gboolean        send_stream_headers;

  /* Special flag for VobSub, for which we have to send colour table info
   * (if available) first before sending any data, and just testing
   * for time == 0 is not enough to detect that. Used by demuxer */
  gboolean      send_dvd_event;

  /* Special counter for muxer to skip the first N vorbis/theora headers -
   * they are put into codec private data, not muxed into the stream */
  guint         xiph_headers_to_skip;

  /* Used for postprocessing a frame before it is pushed from the demuxer */
  GstFlowReturn (*postprocess_frame) (GstElement *element,
                                      GstMatroskaTrackContext *context,
				      GstBuffer **buffer);

  /* List of tags for this stream */
  GstTagList   *tags;
  /* Tags changed and should be pushed again */
  gboolean      tags_changed;

  /* A GArray of GstMatroskaTrackEncoding structures which contain the
   * encoding (compression/encryption) settings for this track, if any */
  GArray       *encodings;

  /* Whether the stream is EOS */
  gboolean      eos;

  /* any alignment we need our output buffers to have */
  gint          alignment;

  /* for compatibility with VFW files, where timestamp represents DTS */
  gboolean      dts_only;

  /* indicate that the track is raw (jpeg,raw variants) and so pts=dts */
  gboolean		intra_only;
};

typedef struct _GstMatroskaTrackVideoContext {
  GstMatroskaTrackContext parent;

  guint         pixel_width, pixel_height;
  guint         display_width, display_height;
  gdouble       default_fps;
  GstMatroskaAspectRatioMode asr_mode;
  guint32       fourcc;

  GstMatroskaInterlaceMode interlace_mode;
  GstVideoFieldOrder field_order;

  GstVideoMultiviewMode multiview_mode;
  GstVideoMultiviewFlags multiview_flags;

  /* QoS */
  GstClockTime  earliest_time;

  GstBuffer     *dirac_unit;
  GstVideoColorimetry colorimetry;

  GstVideoMasteringDisplayInfo mastering_display_info;
  gboolean mastering_display_info_present;

  GstVideoContentLightLevel content_light_level;
} GstMatroskaTrackVideoContext;

typedef struct _GstMatroskaTrackAudioContext {
  GstMatroskaTrackContext parent;

  guint         samplerate, channels, bitdepth;

  guint32       wvpk_block_index;
} GstMatroskaTrackAudioContext;

typedef struct _GstMatroskaTrackSubtitleContext {
  GstMatroskaTrackContext parent;

  gboolean    check_utf8;     /* buffers should be valid UTF-8 */
  gboolean    check_markup;   /* check if buffers contain markup
                               * or plaintext and escape characters */
  gboolean    invalid_utf8;   /* work around broken files      */
  gboolean    seen_markup_tag;  /* markup found in text */
} GstMatroskaTrackSubtitleContext;

typedef struct _GstMatroskaIndex {
  guint64        pos;      /* of the corresponding *cluster*! */
  GstClockTime   time;     /* in nanoseconds */
  guint32        block;    /* number of the block in the cluster */
  guint16        track;    /* reference to 'num' */
} GstMatroskaIndex;

typedef struct _Wavpack4Header {
  guchar  ck_id [4];     /* "wvpk"                                         */
  guint32 ck_size;       /* size of entire frame (minus 8, of course)      */
  guint16 version;       /* 0x403 for now                                  */
  guint8  track_no;      /* track number (0 if not used, like now)         */
  guint8  index_no;      /* remember these? (0 if not used, like now)      */
  guint32 total_samples; /* for entire file (-1 if unknown)                */
  guint32 block_index;   /* index of first sample in block (to file begin) */
  guint32 block_samples; /* # samples in this block                        */
  guint32 flags;         /* various flags for id and decoding              */
  guint32 crc;           /* crc for actual decoded data                    */
} Wavpack4Header;

typedef enum {
  GST_MATROSKA_TRACK_ENCODING_SCOPE_FRAME = (1<<0),
  GST_MATROSKA_TRACK_ENCODING_SCOPE_CODEC_DATA = (1<<1),
  GST_MATROSKA_TRACK_ENCODING_SCOPE_NEXT_CONTENT_ENCODING = (1<<2)
} GstMatroskaTrackEncodingScope;

#define MATROSKA_TRACK_ENCODING_SCOPE_TYPE (matroska_track_encoding_scope_get_type())
GType matroska_track_encoding_scope_get_type (void);

typedef enum {
  GST_MATROSKA_TRACK_COMPRESSION_ALGORITHM_ZLIB = 0,
  GST_MATROSKA_TRACK_COMPRESSION_ALGORITHM_BZLIB = 1,
  GST_MATROSKA_TRACK_COMPRESSION_ALGORITHM_LZO1X = 2,
  GST_MATROSKA_TRACK_COMPRESSION_ALGORITHM_HEADERSTRIP = 3
} GstMatroskaTrackCompressionAlgorithm;

/* The encryption algorithm used. The value '0' means that the contents
 * have not been encrypted but only signed.
 * Predefined values: 1 - DES; 2 - 3DES; 3 - Twofish; 4 - Blowfish; 5 - AES.
 * WebM only supports a value of 5 (AES).
 */
typedef enum {
  GST_MATROSKA_TRACK_ENCRYPTION_ALGORITHM_NONE     = 0,
  GST_MATROSKA_TRACK_ENCRYPTION_ALGORITHM_DES      = 1,
  GST_MATROSKA_TRACK_ENCRYPTION_ALGORITHM_3DES     = 2,
  GST_MATROSKA_TRACK_ENCRYPTION_ALGORITHM_TWOFISH  = 3,
  GST_MATROSKA_TRACK_ENCRYPTION_ALGORITHM_BLOWFISH = 4,
  GST_MATROSKA_TRACK_ENCRYPTION_ALGORITHM_AES      = 5
} GstMatroskaTrackEncryptionAlgorithm;

#define MATROSKA_TRACK_ENCRYPTION_ALGORITHM_TYPE (matroska_track_encryption_algorithm_get_type())
GType matroska_track_encryption_algorithm_get_type (void);

/* Defined only in WebM spec.
 * The cipher mode used in the encryption. Predefined values: 1 - CTR
 */
typedef enum {
  GST_MATROSKA_TRACK_ENCRYPTION_CIPHER_MODE_NONE    = 0,
  GST_MATROSKA_TRACK_ENCRYPTION_CIPHER_MODE_CTR     = 1
} GstMatroskaTrackEncryptionCipherMode;

#define MATROSKA_TRACK_ENCRYPTION_CIPHER_MODE_TYPE (matroska_track_encryption_cipher_mode_get_type())
GType matroska_track_encryption_cipher_mode_get_type (void);


typedef struct _GstMatroskaTrackEncoding {
  guint   order;
  guint   scope     : 3;
  guint   type      : 1;
  guint   comp_algo : 2;
  guint8 *comp_settings;
  guint   comp_settings_length;
  guint   enc_algo  : 3;
  guint   enc_cipher_mode : 2;
} GstMatroskaTrackEncoding;

gboolean gst_matroska_track_init_video_context    (GstMatroskaTrackContext ** p_context);
gboolean gst_matroska_track_init_audio_context    (GstMatroskaTrackContext ** p_context);
gboolean gst_matroska_track_init_subtitle_context (GstMatroskaTrackContext ** p_context);

void gst_matroska_register_tags (void);

GstBufferList * gst_matroska_parse_xiph_stream_headers  (gpointer codec_data,
                                                         gsize codec_data_size);

GstBufferList * gst_matroska_parse_speex_stream_headers (gpointer codec_data,
                                                         gsize codec_data_size);

GstBufferList * gst_matroska_parse_opus_stream_headers  (gpointer codec_data,
                                                         gsize codec_data_size);

GstBufferList * gst_matroska_parse_flac_stream_headers  (gpointer codec_data,
                                                         gsize codec_data_size);
void gst_matroska_track_free (GstMatroskaTrackContext * track);
GstClockTime gst_matroska_track_get_buffer_timestamp (GstMatroskaTrackContext * track, GstBuffer *buf);

#endif /* __GST_MATROSKA_IDS_H__ */
