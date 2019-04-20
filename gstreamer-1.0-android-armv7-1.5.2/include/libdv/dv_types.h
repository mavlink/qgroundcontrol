/*
 *  dv_types.h
 *
 *     Copyright (C) Charles 'Buck' Krasic - April 2000
 *     Copyright (C) Erik Walthinsen - April 2000
 *
 *  This file is part of libdv, a free DV (IEC 61834/SMPTE 314M)
 *  codec.
 *
 *  libdv is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU Lesser Public License as published by
 *  the Free Software Foundation; either version 2.1, or (at your
 *  option) any later version.
 *   
 *  libdv is distributed in the hope that it will be useful, but
 *  WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser Public License for more details.
 *   
 *  You should have received a copy of the GNU Lesser Public License
 *  along with libdv; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 *  The libdv homepage is http://libdv.sourceforge.net/.  
 */
#ifndef DV_TYPES_H
#define DV_TYPES_H

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_LIBPOPT
#include <popt.h>
#endif // HAVE_LIBPOPT

#include <stdlib.h>
#include <inttypes.h>
#include <stdio.h>
#include <time.h>
#if HAVE_ENDIAN_H
#include <endian.h>
#elif HAVE_MACHINE_ENDIAN_H
#include <machine/endian.h>
#elif HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

/* please tell me these are defined somewhere standard??? */
#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) ((a)<(b)?(b):(a))
#endif
#ifndef CLAMP
#define CLAMP(a,x,b) (MIN(b,MAX(a,x)))
#endif

// For now assume ARCH_X86 means GCC with hints.
#ifdef ARCH_X86
#define HAVE_GCC 1
#endif
#ifdef ARCH_X86_64
#define HAVE_GCC 1
#endif
//#define HAVE_GCC 0

#if HAVE_GCC
#define ALIGN64 __attribute__ ((aligned (64)))
#define ALIGN32 __attribute__ ((aligned (32)))
#define ALIGN8 __attribute__ ((aligned (8)))
#else
#define ALIGN64
#define ALIGN32
#define ALIGN8
#define __inline__ inline
#define __FUNCTION__ __FILE__ // Less specific info, but it's a string.
#endif

#define DV_AUDIO_MAX_SAMPLES 1944

#define DV_AUDIO_OPT_FREQUENCY    0
#define DV_AUDIO_OPT_QUANTIZATION 1
#define DV_AUDIO_OPT_EMPHASIS     2
#define DV_AUDIO_OPT_CHAN_MIX     3
#define DV_AUDIO_OPT_CALLBACK     4
#define DV_AUDIO_NUM_OPTS         5

#define DV_AUDIO_CORRECT_NONE     0
#define DV_AUDIO_CORRECT_SILENCE  1
#define DV_AUDIO_CORRECT_AVERAGE  2

#define DV_VIDEO_OPT_BLOCK_QUALITY 0
#define DV_VIDEO_OPT_MONOCHROME    1
#define DV_VIDEO_OPT_CALLBACK      2
#define DV_VIDEO_NUM_OPTS          3

#define DV_DECODER_OPT_SYSTEM        0
#define DV_DECODER_OPT_VIDEO_INCLUDE 1
#define DV_DECODER_OPT_AUDIO_INCLUDE 2
#define DV_DECODER_OPT_CALLBACK      3
#define DV_DECODER_OPT_NTSCSETUP     4
#define DV_DECODER_NUM_OPTS          5

#define DV_OSS_OPT_DEVICE 0
#define DV_OSS_OPT_FILE   1
#define DV_OSS_NUM_OPTS   2

#define DV_DCT_AUTO	(-1)
#define DV_DCT_248	(1)
#define DV_DCT_88	(0)

#define DV_SCT_HEADER    (0x0)
#define DV_SCT_SUBCODE   (0x1)
#define DV_SCT_VAUX      (0x2)
#define DV_SCT_AUDIO     (0x3)
#define DV_SCT_VIDEO     (0x4)

#define DV_FSC_0         (0)
#define DV_FSC_1         (1)

#if ARCH_X86 || ARCH_X86_64
#define DV_WEIGHT_BIAS	 6
#else
#define DV_WEIGHT_BIAS	 0
#endif

#define DV_QUALITY_COLOR       1     /* Clear this bit to make monochrome */

#define DV_QUALITY_AC_MASK     (0x3 << 1)
#define DV_QUALITY_DC          (0x0 << 1)
#define DV_QUALITY_AC_1        (0x1 << 1)
#define DV_QUALITY_AC_2        (0x2 << 1)

#define DV_QUALITY_BEST       (DV_QUALITY_COLOR | DV_QUALITY_AC_2)
#define DV_QUALITY_FASTEST     0     /* Monochrome, DC coeffs only */

typedef enum color_space_e { 
  e_dv_color_yuv, 
  e_dv_color_rgb, 
  e_dv_color_bgr0, 
} dv_color_space_t;

typedef enum sample_e { 
  e_dv_sample_none = 0,
  e_dv_sample_411,
  e_dv_sample_420,
  e_dv_sample_422,
} dv_sample_t;

typedef enum system_e { 
  e_dv_system_none = 0,
  e_dv_system_525_60,    // NTSC
  e_dv_system_625_50,    // PAL
} dv_system_t;

typedef enum std_e { 
  e_dv_std_none = 0,
  e_dv_std_smpte_314m,    
  e_dv_std_iec_61834,    
} dv_std_t;

typedef int16_t dv_coeff_t;
typedef int32_t dv_248_coeff_t;

typedef struct bitstream_s {
  uint32_t current_word;
  uint32_t next_word;
  uint16_t bits_left;
  uint16_t next_bits;

  uint8_t *buf;
  uint32_t buflen;
  int32_t  bufoffset;

  uint32_t (*bitstream_next_buffer) (uint8_t **,void *);
  void *priv;

  int32_t bitsread;
} bitstream_t;

typedef struct {
  int8_t sct;      // Section type (header,subcode,aux,audio,video)
  int8_t dsn;      // DIF sequence number (0-12)
  int fsc;   // First (0)/Second channel (1)
  int8_t dbn;      // DIF block number (0-134)
} dv_id_t;

typedef struct {
  int dsf;   // DIF sequence flag: 525/60 (0) or 625,50 (1)
  int8_t apt;
  int tf1;
  int8_t ap1;
  int tf2;
  int8_t ap2;
  int tf3;
  int8_t ap3;
} dv_header_t;

typedef struct {
  dv_coeff_t   coeffs[64] ALIGN8;
  int         dct_mode;
  int         class_no;
  int8_t        *reorder;
  int8_t        *reorder_sentinel;
  int         offset;   // bitstream offset of first unused bit
  int         end;      // bitstream offset of last bit + 1
  int         eob;
  int     mark;     // used during passes 2 & 3 for tracking fragmented vlcs
} dv_block_t;

typedef struct {
  int       i,j;   // superblock row/column,
  int       k;     // macroblock no. within superblock */
  int       x, y;  // top-left corner position
  dv_block_t b[6];
  int       qno;
  int       sta;
  int       vlc_error;
  int       eob_count;
} dv_macroblock_t;

typedef struct {
  int            i, k;
  bitstream_t    *bs;
  dv_macroblock_t mb[5];
  int        isPAL;
} dv_videosegment_t;

typedef struct {
  dv_videosegment_t seg[27];
} dv_dif_sequence_t;

// Frame
typedef struct {
  int           placement_done;
  dv_dif_sequence_t  ds[12];
} dv_frame_t;

/* From Section 8.1 of 61834-4: Audio auxiliary data source pack fields pc1-pc4.
 * Need this data to figure out what format audio is in the stream. */

/* About bitfield ordering: The C standard does not specify the order
   of bits within a unit of storage.  In the code here, I will use the
   definition of BYTE_ORDER to determine whether to set
   BIG_ENDIAN_BITFIELD or LITTLE_ENDIAN_BITFIELD.  There is nothing
   that guarantees this relationship to be correct, but I know of no
   counter examples.  If we do find out there is one, we'll have to
   fix it... */

#if (BYTE_ORDER == LITTLE_ENDIAN)
#define LITTLE_ENDIAN_BITFIELD
#else
#define BIG_ENDIAN_BITFIELD
#endif  /* (BYTE_ORDER == LITTLE_ENDIAN) */

typedef struct {
#if defined(LITTLE_ENDIAN_BITFIELD)
  uint8_t af_size : 6; /* Samples per frame:
		       * 32 kHz: 1053-1080
		       * 44.1: 1452-1489
		       * 48: 1580-1620 */
  uint8_t         : 1; // Should be 1
  uint8_t lf      : 1; // Locked mode flag (1 = unlocked)
#elif defined(BIG_ENDIAN_BITFIELD)
  uint8_t lf      : 1; // Locked mode flag (1 = unlocked)
  uint8_t         : 1; // Should be 1
  uint8_t af_size : 6; /* Samples per frame:
		       * 32 kHz: 1053-1080
		       * 44.1: 1452-1489
		       * 48: 1580-1620 */
#endif // BIG_ENDIAN_BITFIELD
} dv_aaux_as_pc1_t;

typedef struct {
#if defined(LITTLE_ENDIAN_BITFIELD)
  uint8_t audio_mode: 4; // See 8.1...
  uint8_t pa        : 1; // pair bit: 0 = one pair of channels, 1 = independent channel (for sm = 1, pa shall be 1)
  uint8_t chn       : 2; // number of audio channels per block: 0 = 1 channel, 1 = 2 channels, others reserved
  uint8_t sm        : 1; // stereo mode: 0 = Multi-stereo, 1 = Lumped
#elif defined(BIG_ENDIAN_BITFIELD)
  uint8_t sm        : 1; // stereo mode: 0 = Multi-stereo, 1 = Lumped
  uint8_t chn       : 2; // number of audio channels per block: 0 = 1 channel, 1 = 2 channels, others reserved
  uint8_t pa        : 1; // pair bit: 0 = one pair of channels, 1 = independent channel (for sm = 1, pa shall be 1)
  uint8_t audio_mode: 4; // See 8.1...
#endif // BIG_ENDIAN_BITFIELD
} dv_aaux_as_pc2_t;

typedef struct {
#if defined(LITTLE_ENDIAN_BITFIELD)
  uint8_t stype     :5; // 0x0 = SD (525/625), 0x2 = HD (1125,1250), others reserved
  uint8_t system    :1; // 0 = 60 fields, 1 = 50 field
  uint8_t ml        :1; // Multi-languag flag
  uint8_t           :1;
#elif defined(BIG_ENDIAN_BITFIELD)
  uint8_t           :1;
  uint8_t ml        :1; // Multi-languag flag
  uint8_t system    :1; // 0 = 60 fields, 1 = 50 field
  uint8_t stype     :5; // 0x0 = SD (525/625), 0x2 = HD (1125,1250), others reserved
#endif // BIG_ENDIAN_BITFIELD
} dv_aaux_as_pc3_t;

typedef struct {
#if defined(LITTLE_ENDIAN_BITFIELD)
  uint8_t qu        :3; // quantization: 0=16bits linear, 1=12bits non-linear, 2=20bits linear, others reserved
  uint8_t smp       :3; // sampling frequency: 0=48kHz, 1=44,1 kHz, 2=32 kHz
  uint8_t tc        :1; // time constant of emphasis: 1=50/15us, 0=reserved
  uint8_t ef        :1; // emphasis: 0=on, 1=off
#elif defined(BIG_ENDIAN_BITFIELD)
  uint8_t ef        :1; // emphasis: 0=on, 1=off
  uint8_t tc        :1; // time constant of emphasis: 1=50/15us, 0=reserved
  uint8_t smp       :3; // sampling frequency: 0=48kHz, 1=44,1 kHz, 2=32 kHz
  uint8_t qu        :3; // quantization: 0=16bits linear, 1=12bits non-linear, 2=20bits linear, others reserved
#endif // BIG_ENDIAN_BITFIELD
} dv_aaux_as_pc4_t;

// AAUX source pack (AS)
typedef struct {
  uint8_t pc0; // value is 0x50;
  dv_aaux_as_pc1_t pc1;
  dv_aaux_as_pc2_t pc2;
  dv_aaux_as_pc3_t pc3;
  dv_aaux_as_pc4_t pc4;
} dv_aaux_as_t;

// From 61834-4 (section 8.2), and SMPE 314M (section 4.6.2.3.2)
typedef struct {
#if defined(LITTLE_ENDIAN_BITFIELD)
  uint8_t ss        :2; /* 61834 says "Source and recorded situation...", SMPTE says EFC (emphasis audio channel flag)
			  0=emphasis off, 1=emphasis on, others reserved.  EFC shall be set for each audio block. */
  uint8_t cmp       :2; /* number of times compression: 0=once, 1=twice, 2=three or more, 3=no information */
  uint8_t isr       :2; /* 0=analog input, 1=digital input, 2=reserved, 3=no information */
  uint8_t cgms      :2; /* Copy generation management system:
			  0=unrestricted, 1=not used, 2=one generation only, 3=no copy */
#elif defined(BIG_ENDIAN_BITFIELD)
  uint8_t cgms      :2; /* Copy generation management system:
			  0=unrestricted, 1=not used, 2=one generation only, 3=no copy */
  uint8_t isr       :2; /* 0=analog input, 1=digital input, 2=reserved, 3=no information */
  uint8_t cmp       :2; /* number of times compression: 0=once, 1=twice, 2=three or more, 3=no information */
  uint8_t ss        :2; /* 61834 says "Source and recorded situation...", SMPTE says EFC (emphasis audio channel flag)
			  0=emphasis off, 1=emphasis on, others reserved.  EFC shall be set for each audio block. */
#endif // BIG_ENDIAN_BITFIELD
} dv_aaux_asc_pc1_t;

typedef struct {
#if defined(LITTLE_ENDIAN_BITFIELD)
  uint8_t insert_ch :3; /* see 61834-4... */
  uint8_t rec_mode  :3; /* recording mode: 1=original, others=various dubs... (see 68134-4) */
  uint8_t rec_end   :1; /* recording end point: same as starting... */
  uint8_t rec_st    :1; /* recording start point: 0=yes,1=no */
#elif defined(BIG_ENDIAN_BITFIELD)
  uint8_t rec_st    :1; /* recording start point: 0=yes,1=no */
  uint8_t rec_end   :1; /* recording end point: same as starting... */
  uint8_t rec_mode  :3; /* recording mode: 1=original, others=various dubs... (see 68134-4) */
  uint8_t insert_ch :3; /* see 61834-4... */
#endif // BIG_ENDIAN_BITFIELD
} dv_aaux_asc_pc2_t;

typedef struct {
#if defined(LITTLE_ENDIAN_BITFIELD)
  uint8_t speed     :7; /* speed: see tables in 314M and 61834-4 (they differ), except 0xff = invalid/unkown */
  uint8_t drf       :1; /* direction: 1=forward, 0=reverse */
#elif defined(BIG_ENDIAN_BITFIELD)
  uint8_t drf       :1; /* direction: 1=forward, 0=reverse */
  uint8_t speed     :7; /* speed: see tables in 314M and 61834-4 (they differ), except 0xff = invalid/unkown */
#endif // BIG_ENDIAN_BITFIELD
} dv_aaux_asc_pc3_t;

typedef struct {
#if defined(LITTLE_ENDIAN_BITFIELD)
  uint8_t genre_category: 7;
  uint8_t               : 1;
#elif defined(BIG_ENDIAN_BITFIELD)
  uint8_t               : 1;
  uint8_t genre_category: 7;
#endif // BIG_ENDIAN_BITFIELD
} dv_aaux_asc_pc4_t;

// AAUX source control pack (ASC)
typedef struct {
  uint8_t pc0; // value is 0x51;
  dv_aaux_asc_pc1_t pc1;
  dv_aaux_asc_pc2_t pc2;
  dv_aaux_asc_pc3_t pc3;
  dv_aaux_asc_pc4_t pc4;
} dv_aaux_asc_t;

typedef struct dv_decoder_s  *dv_decoder_tp;

typedef struct {
  dv_aaux_as_t      aaux_as, /* low-level audio format info direct from the stream */
                    aaux_as1;
  dv_aaux_asc_t     aaux_asc,
                    aaux_asc1;
  int              samples_this_frame;
  int              raw_samples_this_frame [2];
  int              quantization;
  int              max_samples;
  int              frequency;
  int              num_channels;
  int              raw_num_channels;
  int              emphasis;
  int              arg_audio_emphasis;
  int              arg_audio_frequency;
  int              arg_audio_quantization;

  int              new_recording_on_next_frame;
  int              new_recording_current_time_stamp [4],
                   arg_mixing_level;

  /* -------------------------------------------------------------------------
   * audio error correction handling
   */
  int              correction_method,
                   block_failure,
                   sample_failure,
                   real_samples_this_frame,
                   fail_samples_this_frame;
  /* -------------------------------------------------------------------------
   * memorize deemphasis values
   */
  short            lastin [4];
  double           lastout [4];

  FILE             *error_log;
  dv_decoder_tp    dv_decoder;
#if HAVE_LIBPOPT
  struct poptOption option_table[DV_AUDIO_NUM_OPTS+1];
#endif // HAVE_LIBPOPT
} dv_audio_t;

typedef struct {
  unsigned int              quality;
  int               arg_block_quality; // default 3
  int               arg_monochrome;
  /* -------------------------------------------------------------------------
   * video error log file
   */
  FILE              *error_log;
  dv_decoder_tp     dv_decoder;
#if HAVE_LIBPOPT
  struct poptOption  option_table[DV_VIDEO_NUM_OPTS+1];
#endif // HAVE_LIBPOPT

} dv_video_t;

typedef struct dv_decoder_s {
  unsigned int       quality;
  dv_system_t        system;
  dv_std_t           std;
  dv_sample_t        sampling;
  int                num_dif_seqs; // DIF sequences per frame
  int                height, width;
  size_t             frame_size;
  dv_header_t        header;
  dv_audio_t        *audio;
  dv_video_t        *video;
  int                arg_video_system;
  int                add_ntsc_setup;
  int                clamp_luma;
  int                clamp_chroma;
  int                prev_frame_decoded;
  /* -------------------------------------------------------------------------
   * per dif sequence! there are 45 vaux data packs
   * 1 byte header 4 byte data.
   */
  uint8_t             vaux_next;
  uint8_t             vaux_pack [256];
  uint8_t             vaux_data [45][4];

  uint8_t             ssyb_next;
  uint8_t             ssyb_pack [256];
  uint8_t             ssyb_data [45][4];
  bitstream_t        *bs;

#if HAVE_LIBPOPT
  struct poptOption option_table[DV_DECODER_NUM_OPTS+1];
#endif // HAVE_LIBPOPT
} dv_decoder_t;

typedef struct {
  int                 fd;
  int16_t             *buffer;
  uint8_t             *arg_audio_file;
  char                *arg_audio_device;
#if HAVE_LIBPOPT
  struct poptOption option_table[DV_OSS_NUM_OPTS+1];
#endif // HAVE_LIBPOPT
} dv_oss_t;

typedef struct {
  int    isPAL;
  int    is16x9;
  int    vlc_encode_passes;
  int    static_qno;
  int    force_dct; 
  int    rem_ntsc_setup;
  int    clamp_luma;
  int    clamp_chroma;
  int    frame_count;
  int    samples_this_frame;
  /* the below are private */
  short *img_y;
  short *img_cr;
  short *img_cb;
} dv_encoder_t;

#if ARCH_X86
extern int dv_use_mmx;
#elif ARCH_X86_64
extern int dv_use_mmx;
#endif

#endif // DV_TYPES_H
