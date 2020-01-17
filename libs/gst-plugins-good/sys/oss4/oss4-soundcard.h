/*
 * Purpose: The C/C++ header file that defines the OSS API.
 * Description:
 * This header file contains all the declarations required to compile OSS
 * programs. The latest version is always installed together with OSS
 * use of the latest version is strongly recommended.
 *
 * {!notice This header file contains many obsolete definitions
 * (for compatibility with older applications that still need them).
 * Do not use this file as a reference manual of OSS.
 * Please check the OSS Programmer's guide for descriptions
 * of the supported API details (http://manuals.opensound.com/developer).}
 */

#ifndef SOUNDCARD_H
#define SOUNDCARD_H

#define COPYING40 Copyright (C) 4Front Technologies 2000-2006. Released under the BSD license.

#if defined(__cplusplus)
#define EXTERNC extern "C"
#else
#define EXTERNC extern
#endif /* EXTERN_C_WRAPPERS */

#define OSS_VERSION	0x040090 // Pre 4.1

#define SOUND_VERSION	OSS_VERSION
#define OPEN_SOUND_SYSTEM

#if defined(__hpux) && !defined(_HPUX_SOURCE)
#	error "-D_HPUX_SOURCE must be used when compiling OSS applications"
#endif

#ifdef __hpux
#include <sys/ioctl.h>
#endif

#ifdef linux
/* In Linux we need to be prepared for cross compiling */
#include <linux/ioctl.h>
#else
# ifdef __FreeBSD__
#    include <sys/ioccom.h>
# else
#    include <sys/ioctl.h>
# endif
#endif

#ifndef __SIOWR
#if defined(__hpux) || (defined(_IOWR) && (defined(_AIX) || (!defined(sun) && !defined(sparc) && !defined(__INCioctlh) && !defined(__Lynx__))))

/* 
 * Make sure the ioctl macros are compatible with the ones already used
 * by this operating system.
 */
#define	SIOCPARM_MASK	IOCPARM_MASK
#define	SIOC_VOID	IOC_VOID
#define	SIOC_OUT	IOC_OUT
#define	SIOC_IN		IOC_IN
#define	SIOC_INOUT	IOC_INOUT
#define __SIOC_SIZE	_IOC_SIZE
#define __SIOC_DIR	_IOC_DIR
#define __SIOC_NONE	_IOC_NONE
#define __SIOC_READ	_IOC_READ
#define __SIOC_WRITE	_IOC_WRITE
#define	__SIO		_IO
#define	__SIOR		_IOR
#define	__SIOW		_IOW
#define	__SIOWR		_IOWR
#else

/* #define	SIOCTYPE		(0xff<<8) */
#define	SIOCPARM_MASK	0x1fff	/* parameters must be < 8192 bytes */
#define	SIOC_VOID	0x00000000	/* no parameters */
#define	SIOC_OUT	0x20000000	/* copy out parameters */
#define	SIOC_IN		0x40000000	/* copy in parameters */
#define	SIOC_INOUT	(SIOC_IN|SIOC_OUT)

#define	__SIO(x,y)	((int)(SIOC_VOID|(x<<8)|y))
#define	__SIOR(x,y,t)	((int)(SIOC_OUT|((sizeof(t)&SIOCPARM_MASK)<<16)|(x<<8)|y))
#define	__SIOW(x,y,t)	((int)(SIOC_IN|((sizeof(t)&SIOCPARM_MASK)<<16)|(x<<8)|y))
#define	__SIOWR(x,y,t)	((int)(SIOC_INOUT|((sizeof(t)&SIOCPARM_MASK)<<16)|(x<<8)|y))
#define __SIOC_SIZE(x)	((x>>16)&SIOCPARM_MASK)
#define __SIOC_DIR(x)	(x & 0xf0000000)
#define __SIOC_NONE	SIOC_VOID
#define __SIOC_READ	SIOC_OUT
#define __SIOC_WRITE	SIOC_IN
#  endif /* _IOWR */
#endif /* !__SIOWR */

#define OSS_LONGNAME_SIZE	64
#define OSS_LABEL_SIZE		16
#define OSS_DEVNODE_SIZE	32
typedef char oss_longname_t[OSS_LONGNAME_SIZE];
typedef char oss_label_t[OSS_LABEL_SIZE];
typedef char oss_devnode_t[OSS_DEVNODE_SIZE];

#ifndef DISABLE_SEQUENCER
/*
 ****************************************************************************
 * IOCTL Commands for /dev/sequencer and /dev/music (AKA /dev/sequencer2)
 *
 * Note that this interface is obsolete and no longer developed. New
 * applications should use /dev/midi instead.
 ****************************************************************************/
#define SNDCTL_SEQ_RESET		__SIO  ('Q', 0)
#define SNDCTL_SEQ_SYNC			__SIO  ('Q', 1)
#define SNDCTL_SYNTH_INFO		__SIOWR('Q', 2, struct synth_info)
#define SNDCTL_SEQ_CTRLRATE		__SIOWR('Q', 3, int)	/* Set/get timer resolution (HZ) */
#define SNDCTL_SEQ_GETOUTCOUNT		__SIOR ('Q', 4, int)
#define SNDCTL_SEQ_GETINCOUNT		__SIOR ('Q', 5, int)
#define SNDCTL_SEQ_PERCMODE		__SIOW ('Q', 6, int)
#define SNDCTL_FM_LOAD_INSTR		__SIOW ('Q', 7, struct sbi_instrument)	/* Obsolete. Don't use!!!!!! */
#define SNDCTL_SEQ_TESTMIDI		__SIOW ('Q', 8, int)
#define SNDCTL_SEQ_RESETSAMPLES		__SIOW ('Q', 9, int)
#define SNDCTL_SEQ_NRSYNTHS		__SIOR ('Q',10, int)
#define SNDCTL_SEQ_NRMIDIS		__SIOR ('Q',11, int)
#define SNDCTL_MIDI_INFO		__SIOWR('Q',12, struct midi_info)	/* OBSOLETE - use SNDCTL_MIDIINFO instead */
#define SNDCTL_SEQ_THRESHOLD		__SIOW ('Q',13, int)
#define SNDCTL_SYNTH_MEMAVL		__SIOWR('Q',14, int)	/* in=dev#, out=memsize */
#define SNDCTL_FM_4OP_ENABLE		__SIOW ('Q',15, int)	/* in=dev# */
#define SNDCTL_SEQ_PANIC		__SIO  ('Q',17)
#define SNDCTL_SEQ_OUTOFBAND		__SIOW ('Q',18, struct seq_event_rec)
#define SNDCTL_SEQ_GETTIME		__SIOR ('Q',19, int)
#define SNDCTL_SYNTH_ID			__SIOWR('Q',20, struct synth_info)
#define SNDCTL_SYNTH_CONTROL		__SIOWR('Q',21, struct synth_control)
#define SNDCTL_SYNTH_REMOVESAMPLE	__SIOWR('Q',22, struct remove_sample)	/* Reserved for future use */
#define SNDCTL_SEQ_TIMING_ENABLE	__SIO  ('Q', 23)	/* Enable incoming MIDI timing messages */
#define SNDCTL_SEQ_ACTSENSE_ENABLE	__SIO  ('Q', 24)	/* Enable incoming active sensing messages */
#define SNDCTL_SEQ_RT_ENABLE		__SIO  ('Q', 25)	/* Enable other incoming realtime messages */

typedef struct synth_control
{
  int devno;			/* Synthesizer # */
  char data[4000];		/* Device spesific command/data record */
} synth_control;

typedef struct remove_sample
{
  int devno;			/* Synthesizer # */
  int bankno;			/* MIDI bank # (0=General MIDI) */
  int instrno;			/* MIDI instrument number */
} remove_sample;

typedef struct seq_event_rec
{
  unsigned char arr[8];
} seq_event_rec;

#define SNDCTL_TMR_TIMEBASE		__SIOWR('T', 1, int)
#define SNDCTL_TMR_START		__SIO  ('T', 2)
#define SNDCTL_TMR_STOP			__SIO  ('T', 3)
#define SNDCTL_TMR_CONTINUE		__SIO  ('T', 4)
#define SNDCTL_TMR_TEMPO		__SIOWR('T', 5, int)
#define SNDCTL_TMR_SOURCE		__SIOWR('T', 6, int)
#	define TMR_INTERNAL		0x00000001
#	define TMR_EXTERNAL		0x00000002
#		define TMR_MODE_MIDI	0x00000010
#		define TMR_MODE_FSK	0x00000020
#		define TMR_MODE_CLS	0x00000040
#		define TMR_MODE_SMPTE	0x00000080
#define SNDCTL_TMR_METRONOME		__SIOW ('T', 7, int)
#define SNDCTL_TMR_SELECT		__SIOW ('T', 8, int)

/*
 * Sample loading mechanism for internal synthesizers (/dev/sequencer)
 * (for the .PAT format).
 */

struct patch_info
{
  unsigned short key;		/* Use WAVE_PATCH here */
#define WAVE_PATCH	_PATCHKEY(0x04)
#define GUS_PATCH	WAVE_PATCH
#define WAVEFRONT_PATCH _PATCHKEY(0x06)

  short device_no;		/* Synthesizer number */
  short instr_no;		/* Midi pgm# */

  unsigned int mode;
/*
 * The least significant byte has the same format than the GUS .PAT
 * files
 */
#define WAVE_16_BITS	0x01	/* bit 0 = 8 or 16 bit wave data. */
#define WAVE_UNSIGNED	0x02	/* bit 1 = Signed - Unsigned data. */
#define WAVE_LOOPING	0x04	/* bit 2 = looping enabled-1. */
#define WAVE_BIDIR_LOOP	0x08	/* bit 3 = Set is bidirectional looping. */
#define WAVE_LOOP_BACK	0x10	/* bit 4 = Set is looping backward. */
#define WAVE_SUSTAIN_ON	0x20	/* bit 5 = Turn sustaining on. (Env. pts. 3) */
#define WAVE_ENVELOPES	0x40	/* bit 6 = Enable envelopes - 1 */
#define WAVE_FAST_RELEASE 0x80	/* bit 7 = Shut off immediately after note off */
  /*  (use the env_rate/env_offs fields). */
/* Linux specific bits */
#define WAVE_VIBRATO	0x00010000	/* The vibrato info is valid */
#define WAVE_TREMOLO	0x00020000	/* The tremolo info is valid */
#define WAVE_SCALE	0x00040000	/* The scaling info is valid */
#define WAVE_FRACTIONS	0x00080000	/* Fraction information is valid */
/* Reserved bits */
#define WAVE_ROM	0x40000000	/* For future use */
#define WAVE_MULAW	0x20000000	/* For future use */
/* Other bits must be zeroed */

  int len;			/* Size of the wave data in bytes */
  int loop_start, loop_end;	/* Byte offsets from the beginning */

/* 
 * The base_freq and base_note fields are used when computing the
 * playback speed for a note. The base_note defines the tone frequency
 * which is heard if the sample is played using the base_freq as the
 * playback speed.
 *
 * The low_note and high_note fields define the minimum and maximum note
 * frequencies for which this sample is valid. It is possible to define
 * more than one samples for an instrument number at the same time. The
 * low_note and high_note fields are used to select the most suitable one.
 *
 * The fields base_note, high_note and low_note should contain
 * the note frequency multiplied by 1000. For example value for the
 * middle A is 440*1000.
 */

  unsigned int base_freq;
  unsigned int base_note;
  unsigned int high_note;
  unsigned int low_note;
  int panning;			/* -128=left, 127=right */
  int detuning;

  /* Envelope. Enabled by mode bit WAVE_ENVELOPES  */
  unsigned char env_rate[6];	/* GUS HW ramping rate */
  unsigned char env_offset[6];	/* 255 == 100% */

  /* 
   * The tremolo, vibrato and scale info are not supported yet.
   * Enable by setting the mode bits WAVE_TREMOLO, WAVE_VIBRATO or
   * WAVE_SCALE
   */

  unsigned char tremolo_sweep;
  unsigned char tremolo_rate;
  unsigned char tremolo_depth;

  unsigned char vibrato_sweep;
  unsigned char vibrato_rate;
  unsigned char vibrato_depth;

  int scale_frequency;
  unsigned int scale_factor;	/* from 0 to 2048 or 0 to 2 */

  int volume;
  int fractions;
  int reserved1;
  int spare[2];
  char data[1];			/* The waveform data starts here */
};

struct sysex_info
{
  short key;			/* Use SYSEX_PATCH or MAUI_PATCH here */
#define SYSEX_PATCH	_PATCHKEY(0x05)
#define MAUI_PATCH	_PATCHKEY(0x06)
  short device_no;		/* Synthesizer number */
  int len;			/* Size of the sysex data in bytes */
  unsigned char data[1];	/* Sysex data starts here */
};

/*
 * /dev/sequencer input events.
 *
 * The data written to the /dev/sequencer is a stream of events. Events
 * are records of 4 or 8 bytes. The first byte defines the size. 
 * Any number of events can be written with a write call. There
 * is a set of macros for sending these events. Use these macros if you
 * want to maximize portability of your program.
 *
 * Events SEQ_WAIT, SEQ_MIDIPUTC and SEQ_ECHO. Are also input events.
 * (All input events are currently 4 bytes long. Be prepared to support
 * 8 byte events also. If you receive any event having first byte >= 128,
 * it's a 8 byte event.
 *
 * The events are documented at the end of this file.
 *
 * Normal events (4 bytes)
 * There is also a 8 byte version of most of the 4 byte events. The
 * 8 byte one is recommended.
 *
 * NOTE! All 4 byte events are now obsolete. Applications should not write
 *       them. However 4 byte events are still used as inputs from
 *       /dev/sequencer (/dev/music uses only 8 byte ones).
 */
#define SEQ_NOTEOFF		0
#define SEQ_FMNOTEOFF		SEQ_NOTEOFF	/* Just old name */
#define SEQ_NOTEON		1
#define	SEQ_FMNOTEON		SEQ_NOTEON
#define SEQ_WAIT		TMR_WAIT_ABS
#define SEQ_PGMCHANGE		3
#define SEQ_FMPGMCHANGE		SEQ_PGMCHANGE
#define SEQ_SYNCTIMER		TMR_START
#define SEQ_MIDIPUTC		5
#define SEQ_DRUMON		6		/*** OBSOLETE ***/
#define SEQ_DRUMOFF		7		/*** OBSOLETE ***/
#define SEQ_ECHO		TMR_ECHO	/* For syncing programs with output */
#define SEQ_AFTERTOUCH		9
#define SEQ_CONTROLLER		10
#define SEQ_BALANCE		11
#define SEQ_VOLMODE             12

/************************************
 *	Midi controller numbers	    *
 ************************************/
/*
 * Controllers 0 to 31 (0x00 to 0x1f) and
 * 32 to 63 (0x20 to 0x3f) are continuous
 * controllers.
 * In the MIDI 1.0 these controllers are sent using
 * two messages. Controller numbers 0 to 31 are used
 * to send the MSB and the controller numbers 32 to 63
 * are for the LSB. Note that just 7 bits are used in MIDI bytes.
 */

#define	   CTL_BANK_SELECT		0x00
#define	   CTL_MODWHEEL			0x01
#define    CTL_BREATH			0x02
/*		undefined		0x03 */
#define    CTL_FOOT			0x04
#define    CTL_PORTAMENTO_TIME		0x05
#define    CTL_DATA_ENTRY		0x06
#define    CTL_MAIN_VOLUME		0x07
#define    CTL_BALANCE			0x08
/*		undefined		0x09 */
#define    CTL_PAN			0x0a
#define    CTL_EXPRESSION		0x0b
/*		undefined		0x0c */
/*		undefined		0x0d */
/*		undefined		0x0e */
/*		undefined		0x0f */
#define    CTL_GENERAL_PURPOSE1		0x10
#define    CTL_GENERAL_PURPOSE2		0x11
#define    CTL_GENERAL_PURPOSE3		0x12
#define    CTL_GENERAL_PURPOSE4		0x13
/*		undefined		0x14 - 0x1f */

/*		undefined		0x20 */
/* The controller numbers 0x21 to 0x3f are reserved for the */
/* least significant bytes of the controllers 0x00 to 0x1f. */
/* These controllers are not recognised by the driver. */

/* Controllers 64 to 69 (0x40 to 0x45) are on/off switches. */
/* 0=OFF and 127=ON (intermediate values are possible) */
#define    CTL_DAMPER_PEDAL		0x40
#define    CTL_SUSTAIN			0x40	/* Alias */
#define    CTL_HOLD			0x40	/* Alias */
#define    CTL_PORTAMENTO		0x41
#define    CTL_SOSTENUTO		0x42
#define    CTL_SOFT_PEDAL		0x43
/*		undefined		0x44 */
#define    CTL_HOLD2			0x45
/*		undefined		0x46 - 0x4f */

#define    CTL_GENERAL_PURPOSE5		0x50
#define    CTL_GENERAL_PURPOSE6		0x51
#define    CTL_GENERAL_PURPOSE7		0x52
#define    CTL_GENERAL_PURPOSE8		0x53
/*		undefined		0x54 - 0x5a */
#define    CTL_EXT_EFF_DEPTH		0x5b
#define    CTL_TREMOLO_DEPTH		0x5c
#define    CTL_CHORUS_DEPTH		0x5d
#define    CTL_DETUNE_DEPTH		0x5e
#define    CTL_CELESTE_DEPTH		0x5e	/* Alias for the above one */
#define    CTL_PHASER_DEPTH		0x5f
#define    CTL_DATA_INCREMENT		0x60
#define    CTL_DATA_DECREMENT		0x61
#define    CTL_NONREG_PARM_NUM_LSB	0x62
#define    CTL_NONREG_PARM_NUM_MSB	0x63
#define    CTL_REGIST_PARM_NUM_LSB	0x64
#define    CTL_REGIST_PARM_NUM_MSB	0x65
/*		undefined		0x66 - 0x78 */
/*		reserved		0x79 - 0x7f */

/* Pseudo controllers (not midi compatible) */
#define    CTRL_PITCH_BENDER		255
#define    CTRL_PITCH_BENDER_RANGE	254
#define    CTRL_EXPRESSION		253	/* Obsolete */
#define    CTRL_MAIN_VOLUME		252	/* Obsolete */

/*
 * Volume mode defines how volumes are used
 */

#define VOL_METHOD_ADAGIO	1
#define VOL_METHOD_LINEAR	2

/*
 * Note! SEQ_WAIT, SEQ_MIDIPUTC and SEQ_ECHO are used also as
 *	 input events.
 */

/*
 * Event codes 0xf0 to 0xfc are reserved for future extensions.
 */

#define SEQ_FULLSIZE		0xfd	/* Long events */
/*
 * SEQ_FULLSIZE events are used for loading patches/samples to the
 * synthesizer devices. These events are passed directly to the driver
 * of the associated synthesizer device. There is no limit to the size
 * of the extended events. These events are not queued but executed
 * immediately when the write() is called (execution can take several
 * seconds of time). 
 *
 * When a SEQ_FULLSIZE message is written to the device, it must
 * be written using exactly one write() call. Other events cannot
 * be mixed to the same write.
 *	
 * For FM synths (YM3812/OPL3) use struct sbi_instrument and write it to the 
 * /dev/sequencer. Don't write other data together with the instrument structure
 * Set the key field of the structure to FM_PATCH. The device field is used to
 * route the patch to the corresponding device.
 *
 * For wave table use struct patch_info. Initialize the key field
 * to WAVE_PATCH.
 */
#define SEQ_PRIVATE		0xfe	/* Low level HW dependent events (8 bytes) */
#define SEQ_EXTENDED		0xff	/* Extended events (8 bytes) OBSOLETE */

/*
 * Record for FM patches
 */

typedef unsigned char sbi_instr_data[32];

struct sbi_instrument
{
  unsigned short key;		/* FM_PATCH or OPL3_PATCH */
#define FM_PATCH	_PATCHKEY(0x01)
#define OPL3_PATCH	_PATCHKEY(0x03)
  short device;			/*  Synth# (0-4)    */
  int channel;			/*  Program# to be initialized  */
  sbi_instr_data operators;	/*  Register settings for operator cells (.SBI format)  */
};

struct synth_info
{				/* Read only */
  char name[30];
  int device;			/* 0-N. INITIALIZE BEFORE CALLING */
  int synth_type;
#define SYNTH_TYPE_FM			0
#define SYNTH_TYPE_SAMPLE		1
#define SYNTH_TYPE_MIDI			2	/* Midi interface */

  int synth_subtype;
#define FM_TYPE_ADLIB			0x00
#define FM_TYPE_OPL3			0x01
#define MIDI_TYPE_MPU401		0x401

#define SAMPLE_TYPE_BASIC		0x10
#define SAMPLE_TYPE_GUS			SAMPLE_TYPE_BASIC
#define SAMPLE_TYPE_WAVEFRONT   	0x11

  int perc_mode;		/* No longer supported */
  int nr_voices;
  int nr_drums;			/* Obsolete field */
  int instr_bank_size;
  unsigned int capabilities;
#define SYNTH_CAP_PERCMODE	0x00000001	/* No longer used */
#define SYNTH_CAP_OPL3		0x00000002	/* Set if OPL3 supported */
#define SYNTH_CAP_INPUT		0x00000004	/* Input (MIDI) device */
  int dummies[19];		/* Reserve space */
};

struct sound_timer_info
{
  char name[32];
  int caps;
};

struct midi_info		/* OBSOLETE */
{
  char name[30];
  int device;			/* 0-N. INITIALIZE BEFORE CALLING */
  unsigned int capabilities;	/* To be defined later */
  int dev_type;
  int dummies[18];		/* Reserve space */
};

/*
 * Level 2 event types for /dev/sequencer
 */

/*
 * The 4 most significant bits of byte 0 specify the class of
 * the event: 
 *
 *	0x8X = system level events,
 *	0x9X = device/port specific events, event[1] = device/port,
 *		The last 4 bits give the subtype:
 *			0x02	= Channel event (event[3] = chn).
 *			0x01	= note event (event[4] = note).
 *			(0x01 is not used alone but always with bit 0x02).
 *	       event[2] = MIDI message code (0x80=note off etc.)
 *
 */

#define EV_SEQ_LOCAL		0x80
#define EV_TIMING		0x81
#define EV_CHN_COMMON		0x92
#define EV_CHN_VOICE		0x93
#define EV_SYSEX		0x94
#define EV_SYSTEM		0x95	/* MIDI system and real time messages (input only) */
/*
 * Event types 200 to 220 are reserved for application use.
 * These numbers will not be used by the driver.
 */

/*
 * Events for event type EV_CHN_VOICE
 */

#define MIDI_NOTEOFF		0x80
#define MIDI_NOTEON		0x90
#define MIDI_KEY_PRESSURE	0xA0

/*
 * Events for event type EV_CHN_COMMON
 */

#define MIDI_CTL_CHANGE		0xB0
#define MIDI_PGM_CHANGE		0xC0
#define MIDI_CHN_PRESSURE	0xD0
#define MIDI_PITCH_BEND		0xE0

#define MIDI_SYSTEM_PREFIX	0xF0

/*
 * Timer event types
 */
#define TMR_WAIT_REL		1	/* Time relative to the prev time */
#define TMR_WAIT_ABS		2	/* Absolute time since TMR_START */
#define TMR_STOP		3
#define TMR_START		4
#define TMR_CONTINUE		5
#define TMR_TEMPO		6
#define TMR_ECHO		8
#define TMR_CLOCK		9	/* MIDI clock */
#define TMR_SPP			10	/* Song position pointer */
#define TMR_TIMESIG		11	/* Time signature */

/*
 *	Local event types
 */
#define LOCL_STARTAUDIO		1
#define LOCL_STARTAUDIO2	2
#define LOCL_STARTAUDIO3	3
#define LOCL_STARTAUDIO4	4

#if (!defined(__KERNEL__) && !defined(KERNEL) && !defined(INKERNEL) && !defined(_KERNEL)) || defined(USE_SEQ_MACROS)
/*
 * Some convenience macros to simplify programming of the
 * /dev/sequencer interface
 *
 * These macros define the API which should be used when possible.
 */
#define SEQ_DECLAREBUF()		SEQ_USE_EXTBUF()

void seqbuf_dump (void);	/* This function must be provided by programs */

EXTERNC int OSS_init (int seqfd, int buflen);
EXTERNC void OSS_seqbuf_dump (int fd, unsigned char *buf, int buflen);
EXTERNC void OSS_seq_advbuf (int len, int fd, unsigned char *buf, int buflen);
EXTERNC void OSS_seq_needbuf (int len, int fd, unsigned char *buf,
			      int buflen);
EXTERNC void OSS_patch_caching (int dev, int chn, int patch, int fd,
				unsigned char *buf, int buflen);
EXTERNC void OSS_drum_caching (int dev, int chn, int patch, int fd,
			       unsigned char *buf, int buflen);
EXTERNC void OSS_write_patch (int fd, unsigned char *buf, int len);
EXTERNC int OSS_write_patch2 (int fd, unsigned char *buf, int len);

#define SEQ_PM_DEFINES int __foo_bar___
#ifdef OSSLIB
#  define SEQ_USE_EXTBUF() \
		EXTERNC unsigned char *_seqbuf; \
		EXTERNC int _seqbuflen;EXTERNC int _seqbufptr
#  define SEQ_DEFINEBUF(len) SEQ_USE_EXTBUF();static int _requested_seqbuflen=len
#  define _SEQ_ADVBUF(len) OSS_seq_advbuf(len, seqfd, _seqbuf, _seqbuflen)
#  define _SEQ_NEEDBUF(len) OSS_seq_needbuf(len, seqfd, _seqbuf, _seqbuflen)
#  define SEQ_DUMPBUF() OSS_seqbuf_dump(seqfd, _seqbuf, _seqbuflen)

#  define SEQ_LOAD_GMINSTR(dev, instr) \
		OSS_patch_caching(dev, -1, instr, seqfd, _seqbuf, _seqbuflen)
#  define SEQ_LOAD_GMDRUM(dev, drum) \
		OSS_drum_caching(dev, -1, drum, seqfd, _seqbuf, _seqbuflen)
#else /* !OSSLIB */

#  define SEQ_LOAD_GMINSTR(dev, instr)
#  define SEQ_LOAD_GMDRUM(dev, drum)

#  define SEQ_USE_EXTBUF() \
		EXTERNC unsigned char _seqbuf[]; \
		EXTERNC int _seqbuflen;EXTERNC int _seqbufptr

#ifndef USE_SIMPLE_MACROS
/* Sample seqbuf_dump() implementation:
 *
 *	SEQ_DEFINEBUF (2048);	-- Defines a buffer for 2048 bytes
 *
 *	int seqfd;		-- The file descriptor for /dev/sequencer.
 *
 *	void
 *	seqbuf_dump ()
 *	{
 *	  if (_seqbufptr)
 *	    if (write (seqfd, _seqbuf, _seqbufptr) == -1)
 *	      {
 *		perror ("write /dev/sequencer");
 *		exit (-1);
 *	      }
 *	  _seqbufptr = 0;
 *	}
 */

#define SEQ_DEFINEBUF(len) \
	unsigned char _seqbuf[len]; int _seqbuflen = len;int _seqbufptr = 0
#define _SEQ_NEEDBUF(len) \
	if ((_seqbufptr+(len)) > _seqbuflen) seqbuf_dump()
#define _SEQ_ADVBUF(len) _seqbufptr += len
#define SEQ_DUMPBUF seqbuf_dump
#else
/*
 * This variation of the sequencer macros is used just to format one event
 * using fixed buffer.
 * 
 * The program using the macro library must define the following macros before
 * using this library.
 *
 * #define _seqbuf 		 name of the buffer (unsigned char[]) 
 * #define _SEQ_ADVBUF(len)	 If the applic needs to know the exact
 *				 size of the event, this macro can be used.
 *				 Otherwise this must be defined as empty.
 * #define _seqbufptr		 Define the name of index variable or 0 if
 *				 not required. 
 */
#define _SEQ_NEEDBUF(len)	/* empty */
#endif
#endif /* !OSSLIB */

#define SEQ_VOLUME_MODE(dev, mode) \
				{_SEQ_NEEDBUF(8);\
				_seqbuf[_seqbufptr] = SEQ_EXTENDED;\
				_seqbuf[_seqbufptr+1] = SEQ_VOLMODE;\
				_seqbuf[_seqbufptr+2] = (dev);\
				_seqbuf[_seqbufptr+3] = (mode);\
				_seqbuf[_seqbufptr+4] = 0;\
				_seqbuf[_seqbufptr+5] = 0;\
				_seqbuf[_seqbufptr+6] = 0;\
				_seqbuf[_seqbufptr+7] = 0;\
				_SEQ_ADVBUF(8);}

/*
 * Midi voice messages
 */

#define _CHN_VOICE(dev, event, chn, note, parm) \
				{_SEQ_NEEDBUF(8);\
				_seqbuf[_seqbufptr] = EV_CHN_VOICE;\
				_seqbuf[_seqbufptr+1] = (dev);\
				_seqbuf[_seqbufptr+2] = (event);\
				_seqbuf[_seqbufptr+3] = (chn);\
				_seqbuf[_seqbufptr+4] = (note);\
				_seqbuf[_seqbufptr+5] = (parm);\
				_seqbuf[_seqbufptr+6] = (0);\
				_seqbuf[_seqbufptr+7] = 0;\
				_SEQ_ADVBUF(8);}

#define SEQ_START_NOTE(dev, chn, note, vol) \
			_CHN_VOICE(dev, MIDI_NOTEON, chn, note, vol)

#define SEQ_STOP_NOTE(dev, chn, note, vol) \
			_CHN_VOICE(dev, MIDI_NOTEOFF, chn, note, vol)

#define SEQ_KEY_PRESSURE(dev, chn, note, pressure) \
			_CHN_VOICE(dev, MIDI_KEY_PRESSURE, chn, note, pressure)

/*
 * Midi channel messages
 */

#define _CHN_COMMON(dev, event, chn, p1, p2, w14) \
				{_SEQ_NEEDBUF(8);\
				_seqbuf[_seqbufptr] = EV_CHN_COMMON;\
				_seqbuf[_seqbufptr+1] = (dev);\
				_seqbuf[_seqbufptr+2] = (event);\
				_seqbuf[_seqbufptr+3] = (chn);\
				_seqbuf[_seqbufptr+4] = (p1);\
				_seqbuf[_seqbufptr+5] = (p2);\
				*(short *)&_seqbuf[_seqbufptr+6] = (w14);\
				_SEQ_ADVBUF(8);}
/*
 * SEQ_SYSEX permits sending of sysex messages. (It may look that it permits
 * sending any MIDI bytes but it's absolutely not possible. Trying to do
 * so _will_ cause problems with MPU401 intelligent mode).
 *
 * Sysex messages are sent in blocks of 1 to 6 bytes. Longer messages must be 
 * sent by calling SEQ_SYSEX() several times (there must be no other events
 * between them). First sysex fragment must have 0xf0 in the first byte
 * and the last byte (buf[len-1] of the last fragment must be 0xf7. No byte
 * between these sysex start and end markers cannot be larger than 0x7f. Also
 * lengths of each fragments (except the last one) must be 6.
 *
 * Breaking the above rules may work with some MIDI ports but is likely to
 * cause fatal problems with some other devices (such as MPU401).
 */
#define SEQ_SYSEX(dev, buf, len) \
				{int ii, ll=(len); \
				 unsigned char *bufp=buf;\
				 if (ll>6)ll=6;\
				_SEQ_NEEDBUF(8);\
				_seqbuf[_seqbufptr] = EV_SYSEX;\
				_seqbuf[_seqbufptr+1] = (dev);\
				for(ii=0;ii<ll;ii++)\
				   _seqbuf[_seqbufptr+ii+2] = bufp[ii];\
				for(ii=ll;ii<6;ii++)\
				   _seqbuf[_seqbufptr+ii+2] = 0xff;\
				_SEQ_ADVBUF(8);}

#define SEQ_CHN_PRESSURE(dev, chn, pressure) \
		_CHN_COMMON(dev, MIDI_CHN_PRESSURE, chn, pressure, 0, 0)

#define SEQ_SET_PATCH SEQ_PGM_CHANGE
#ifdef OSSLIB
#   define SEQ_PGM_CHANGE(dev, chn, patch) \
		{OSS_patch_caching(dev, chn, patch, seqfd, _seqbuf, _seqbuflen); \
		 _CHN_COMMON(dev, MIDI_PGM_CHANGE, chn, patch, 0, 0);}
#else
#   define SEQ_PGM_CHANGE(dev, chn, patch) \
		_CHN_COMMON(dev, MIDI_PGM_CHANGE, chn, patch, 0, 0)
#endif

#define SEQ_CONTROL(dev, chn, controller, value) \
		_CHN_COMMON(dev, MIDI_CTL_CHANGE, chn, controller, 0, value)

#define SEQ_BENDER(dev, chn, value) \
		_CHN_COMMON(dev, MIDI_PITCH_BEND, chn, 0, 0, value)

#define SEQ_V2_X_CONTROL(dev, voice, controller, value)	\
				{_SEQ_NEEDBUF(8);\
				_seqbuf[_seqbufptr] = SEQ_EXTENDED;\
				_seqbuf[_seqbufptr+1] = SEQ_CONTROLLER;\
				_seqbuf[_seqbufptr+2] = (dev);\
				_seqbuf[_seqbufptr+3] = (voice);\
				_seqbuf[_seqbufptr+4] = (controller);\
				_seqbuf[_seqbufptr+5] = ((value)&0xff);\
				_seqbuf[_seqbufptr+6] = ((value>>8)&0xff);\
				_seqbuf[_seqbufptr+7] = 0;\
				_SEQ_ADVBUF(8);}
/*
 * The following 5 macros are incorrectly implemented and obsolete.
 * Use SEQ_BENDER and SEQ_CONTROL (with proper controller) instead.
 */
#define SEQ_PITCHBEND(dev, voice, value) \
	SEQ_V2_X_CONTROL(dev, voice, CTRL_PITCH_BENDER, value)
#define SEQ_BENDER_RANGE(dev, voice, value) \
	SEQ_V2_X_CONTROL(dev, voice, CTRL_PITCH_BENDER_RANGE, value)
#define SEQ_EXPRESSION(dev, voice, value) \
	SEQ_CONTROL(dev, voice, CTL_EXPRESSION, value*128)
#define SEQ_MAIN_VOLUME(dev, voice, value) \
	SEQ_CONTROL(dev, voice, CTL_MAIN_VOLUME, (value*16383)/100)
#define SEQ_PANNING(dev, voice, pos) \
	SEQ_CONTROL(dev, voice, CTL_PAN, (pos+128) / 2)

/*
 * Timing and synchronization macros
 */

#define _TIMER_EVENT(ev, parm)	{_SEQ_NEEDBUF(8);\
			 	_seqbuf[_seqbufptr+0] = EV_TIMING; \
			 	_seqbuf[_seqbufptr+1] = (ev); \
				_seqbuf[_seqbufptr+2] = 0;\
				_seqbuf[_seqbufptr+3] = 0;\
			 	*(unsigned int *)&_seqbuf[_seqbufptr+4] = (parm); \
				_SEQ_ADVBUF(8);}

#define SEQ_START_TIMER()		_TIMER_EVENT(TMR_START, 0)
#define SEQ_STOP_TIMER()		_TIMER_EVENT(TMR_STOP, 0)
#define SEQ_CONTINUE_TIMER()		_TIMER_EVENT(TMR_CONTINUE, 0)
#define SEQ_WAIT_TIME(ticks)		_TIMER_EVENT(TMR_WAIT_ABS, ticks)
#define SEQ_DELTA_TIME(ticks)		_TIMER_EVENT(TMR_WAIT_REL, ticks)
#define SEQ_ECHO_BACK(key)		_TIMER_EVENT(TMR_ECHO, key)
#define SEQ_SET_TEMPO(value)		_TIMER_EVENT(TMR_TEMPO, value)
#define SEQ_SONGPOS(pos)		_TIMER_EVENT(TMR_SPP, pos)
#define SEQ_TIME_SIGNATURE(sig)		_TIMER_EVENT(TMR_TIMESIG, sig)

/*
 * Local control events
 */

#define _LOCAL_EVENT(ev, parm)		{_SEQ_NEEDBUF(8);\
				 	_seqbuf[_seqbufptr+0] = EV_SEQ_LOCAL; \
				 	_seqbuf[_seqbufptr+1] = (ev); \
					_seqbuf[_seqbufptr+2] = 0;\
					_seqbuf[_seqbufptr+3] = 0;\
				 	*(unsigned int *)&_seqbuf[_seqbufptr+4] = (parm); \
					_SEQ_ADVBUF(8);}

#define SEQ_PLAYAUDIO(devmask)		_LOCAL_EVENT(LOCL_STARTAUDIO, devmask)
#define SEQ_PLAYAUDIO2(devmask)		_LOCAL_EVENT(LOCL_STARTAUDIO2, devmask)
#define SEQ_PLAYAUDIO3(devmask)		_LOCAL_EVENT(LOCL_STARTAUDIO3, devmask)
#define SEQ_PLAYAUDIO4(devmask)		_LOCAL_EVENT(LOCL_STARTAUDIO4, devmask)
/*
 * Events for the level 1 interface only 
 */

#define SEQ_MIDIOUT(device, byte)	{_SEQ_NEEDBUF(4);\
					_seqbuf[_seqbufptr] = SEQ_MIDIPUTC;\
					_seqbuf[_seqbufptr+1] = (byte);\
					_seqbuf[_seqbufptr+2] = (device);\
					_seqbuf[_seqbufptr+3] = 0;\
					_SEQ_ADVBUF(4);}

/*
 * Patch loading.
 */
#ifdef OSSLIB
#   define SEQ_WRPATCH(patchx, len) \
		OSS_write_patch(seqfd, (char*)(patchx), len)
#   define SEQ_WRPATCH2(patchx, len) \
		OSS_write_patch2(seqfd, (char*)(patchx), len)
#else
#   define SEQ_WRPATCH(patchx, len) \
		{if (_seqbufptr) SEQ_DUMPBUF();\
		 if (write(seqfd, (char*)(patchx), len)==-1) \
		    perror("Write patch: /dev/sequencer");}
#   define SEQ_WRPATCH2(patchx, len) \
		(SEQ_DUMPBUF(), write(seqfd, (char*)(patchx), len))
#endif

#endif
#endif /* ifndef DISABLE_SEQUENCER */

/*
 ****************************************************************************
 * ioctl commands for the /dev/midi## 
 ****************************************************************************/
#define SNDCTL_MIDI_PRETIME	__SIOWR('m', 0, int)

#if 0
/*
 * The SNDCTL_MIDI_MPUMODE and SNDCTL_MIDI_MPUCMD calls
 * are completely obsolete. The hardware device (MPU-401 "intelligent mode"
 * and compatibles) has disappeared from the market 10 years ago so there 
 * is no need for this stuff. The MPU-401 "UART" mode devices don't support
 * this stuff.
 */
typedef struct
{
  unsigned char cmd;
  char nr_args, nr_returns;
  unsigned char data[30];
} mpu_command_rec;

#define SNDCTL_MIDI_MPUMODE	__SIOWR('m', 1, int)
#define SNDCTL_MIDI_MPUCMD	__SIOWR('m', 2, mpu_command_rec)
#endif

/*
 * SNDCTL_MIDI_MTCINPUT turns on a mode where OSS automatically inserts
 * MTC quarter frame messages (F1 xx) to the input.
 * The argument is the MTC mode:
 *
 * 	-1 = Turn MTC messages OFF (default)
 *	24 = 24 FPS 
 *	25 = 25 FPS 
 *	29 = 30 FPS drop frame
 *	30 = 30 FPS 
 *
 * Note that 25 FPS mode is probably the only mode that is supported. Other
 * modes may be supported in the future versions of OSS, 25 FPS is handy 
 * because it generates 25*4=100 quarter frame messages per second which
 * matches the usual 100 HZ system timer rate).
 *
 * The quarter frame timer will be reset to 0:00:00:00.0 at the moment this
 * ioctl is made.
 */
#define SNDCTL_MIDI_MTCINPUT	__SIOWR('m', 3, int)

/*
 * MTC/SMPTE time code record (for future use)
 */
typedef struct
{
  unsigned char hours, minutes, seconds, frames, qframes;
  char direction;
#define MTC_DIR_STOPPED		 0
#define MTC_DIR_FORWARD		 1
#define MTC_DIR_BACKWARD	-1
  unsigned char time_code_type;
  unsigned int flags;
} oss_mtc_data_t;

#define SNDCTL_MIDI_SETMODE	__SIOWR('m', 6, int)
#	define MIDI_MODE_TRADITIONAL	0
#	define MIDI_MODE_TIMED		1	/* Input times are in MIDI ticks */
#	define MIDI_MODE_TIMED_ABS  	2	/* Input times are absolute (usecs) */

/*
 * Packet header for MIDI_MODE_TIMED and MIDI_MODE_TIMED_ABS
 */
typedef unsigned long long oss_midi_time_t;	/* Variable type for MIDI time (clock ticks) */

typedef struct
{
  int magic;			/* Initialize to MIDI_HDR_MAGIC */
#define MIDI_HDR_MAGIC	-1
  unsigned short event_type;
#define MIDI_EV_WRITE			0	/* Write or read (with payload) */
#define MIDI_EV_TEMPO			1
#define MIDI_EV_ECHO			2
#define MIDI_EV_START			3
#define MIDI_EV_STOP			4
#define MIDI_EV_CONTINUE		5
#define MIDI_EV_XPRESSWRITE		6
#define MIDI_EV_TIMEBASE		7
#define MIDI_EV_DEVCTL			8	/* Device control read/write */
  unsigned short options;
#define MIDI_OPT_NONE			0x0000
#define MIDI_OPT_TIMED			0x0001
#define MIDI_OPT_CONTINUATION		0x0002
#define MIDI_OPT_USECTIME		0x0004	/* Time is absolute (in usecs) */
#define MIDI_OPT_BUSY			0x0008	/* Reserved for internal use */
  oss_midi_time_t time;
  int parm;
  int filler[3];		/* Fur future expansion - init to zeros */
} midi_packet_header_t;
/* 
 * MIDI_PAYLOAD_SIZE is the maximum size of one MIDI input chunk. It must be
 * less (or equal) than 1024 which is the read size recommended in the 
 * documentation. TODO: Explain this better.
 */
#define MIDI_PAYLOAD_SIZE		1000

typedef struct
{
  midi_packet_header_t hdr;
  unsigned char payload[MIDI_PAYLOAD_SIZE];
} midi_packet_t;

#define SNDCTL_MIDI_TIMEBASE		__SIOWR('m', 7, int)
#define SNDCTL_MIDI_TEMPO		__SIOWR('m', 8, int)
/*
 * User land MIDI servers (synths) can use SNDCTL_MIDI_SET_LATENCY
 * to request MIDI events to be sent to them in advance. The parameter
 * (in microseconds) tells how much before the events are submitted.
 *
 * This feature is only valid for loopback devices and possibly some other
 * types of virtual devices.
 */
#define SNDCTL_MIDI_SET_LATENCY		__SIOW ('m', 9, int)
/*
 ****************************************************************************
 * IOCTL commands for /dev/dsp
 ****************************************************************************/

#define SNDCTL_DSP_HALT			__SIO  ('P', 0)
#define SNDCTL_DSP_RESET		SNDCTL_DSP_HALT	/* Old name */
#define SNDCTL_DSP_SYNC			__SIO  ('P', 1)
#define SNDCTL_DSP_SPEED		__SIOWR('P', 2, int)

/* SNDCTL_DSP_STEREO is obsolete - use SNDCTL_DSP_CHANNELS instead */
#define SNDCTL_DSP_STEREO		__SIOWR('P', 3, int)
/* SNDCTL_DSP_STEREO is obsolete - use SNDCTL_DSP_CHANNELS instead */

#define SNDCTL_DSP_GETBLKSIZE		__SIOWR('P', 4, int)
#define SNDCTL_DSP_SAMPLESIZE		SNDCTL_DSP_SETFMT
#define SNDCTL_DSP_CHANNELS		__SIOWR('P', 6, int)
#define SNDCTL_DSP_POST			__SIO  ('P', 8)
#define SNDCTL_DSP_SUBDIVIDE		__SIOWR('P', 9, int)
#define SNDCTL_DSP_SETFRAGMENT		__SIOWR('P',10, int)

/* Audio data formats (Note! U8=8 and S16_LE=16 for compatibility) */
#define SNDCTL_DSP_GETFMTS		__SIOR ('P',11, int)	/* Returns a mask */
#define SNDCTL_DSP_SETFMT		__SIOWR('P',5, int)	/* Selects ONE fmt */
#	define AFMT_QUERY	0x00000000	/* Return current fmt */
#	define AFMT_MU_LAW	0x00000001
#	define AFMT_A_LAW	0x00000002
#	define AFMT_IMA_ADPCM	0x00000004
#	define AFMT_U8		0x00000008
#	define AFMT_S16_LE	0x00000010	/* Little endian signed 16 */
#	define AFMT_S16_BE	0x00000020	/* Big endian signed 16 */
#	define AFMT_S8		0x00000040
#	define AFMT_U16_LE	0x00000080	/* Little endian U16 */
#	define AFMT_U16_BE	0x00000100	/* Big endian U16 */
#	define AFMT_MPEG	0x00000200	/* MPEG (2) audio */

/* AC3 _compressed_ bitstreams (See Programmer's Guide for details). */
#	define AFMT_AC3		0x00000400
/* Ogg Vorbis _compressed_ bit streams */
#	define AFMT_VORBIS	0x00000800

/* 32 bit formats (MSB aligned) formats */
#	define AFMT_S32_LE	0x00001000
#	define AFMT_S32_BE	0x00002000

/* Reserved for _native_ endian double precision IEEE floating point */
#	define AFMT_FLOAT	0x00004000

/* 24 bit formats (LSB aligned in 32 bit word) formats */
#	define AFMT_S24_LE	0x00008000
#	define AFMT_S24_BE	0x00010000

/*
 * S/PDIF raw format. In this format the S/PDIF frames (including all
 * control and user bits) are included in the data stream. Each sample
 * is stored in a 32 bit frame (see IEC-958 for more info). This format
 * is supported by very few devices and it's only usable for purposes
 * where full access to the control/user bits is required (real time control).
 */
#	define AFMT_SPDIF_RAW	0x00020000

/* 24 bit packed (3 byte) little endian format (USB compatibility) */
#	define AFMT_S24_PACKED	0x00040000


/*
 * Some big endian/little endian handling macros (native endian and opposite
 * endian formats). The usage of these macros is described in the OSS
 * Programmer's Manual.
 */

#if defined(_AIX) || defined(AIX) || defined(sparc) || defined(__hppa) || defined(PPC) || defined(__powerpc__) && !defined(i386) && !defined(__i386) && !defined(__i386__)

/* Big endian machines */
#  define _PATCHKEY(id) (0xfd00|id)
#  define AFMT_S16_NE AFMT_S16_BE
#  define AFMT_U16_NE AFMT_U16_BE
#  define AFMT_S32_NE AFMT_S32_BE
#  define AFMT_S24_NE AFMT_S24_BE
#  define AFMT_S16_OE AFMT_S16_LE
#  define AFMT_S32_OE AFMT_S32_LE
#  define AFMT_S24_OE AFMT_S24_LE
#else
#  define _PATCHKEY(id) ((id<<8)|0xfd)
#  define AFMT_S16_NE AFMT_S16_LE
#  define AFMT_U16_NE AFMT_U16_LE
#  define AFMT_S32_NE AFMT_S32_LE
#  define AFMT_S24_NE AFMT_S24_LE
#  define AFMT_S16_OE AFMT_S16_BE
#  define AFMT_S32_OE AFMT_S32_BE
#  define AFMT_S24_OE AFMT_S24_BE
#endif
/*
 * Buffer status queries.
 */
typedef struct audio_buf_info
{
  int fragments;		/* # of available fragments (partially usend ones not counted) */
  int fragstotal;		/* Total # of fragments allocated */
  int fragsize;			/* Size of a fragment in bytes */
  int bytes;			/* Available space in bytes (includes partially used fragments) */
  /* Note! 'bytes' could be more than fragments*fragsize */
} audio_buf_info;

#define SNDCTL_DSP_GETOSPACE		__SIOR ('P',12, audio_buf_info)
#define SNDCTL_DSP_GETISPACE		__SIOR ('P',13, audio_buf_info)
#define SNDCTL_DSP_GETCAPS		__SIOR ('P',15, int)
#	define PCM_CAP_REVISION		0x000000ff	/* Bits for revision level (0 to 255) */
#	define PCM_CAP_DUPLEX		0x00000100	/* Full duplex record/playback */
#	define PCM_CAP_REALTIME		0x00000200	/* Not in use */
#	define PCM_CAP_BATCH		0x00000400	/* Device has some kind of */
							/* internal buffers which may */
							/* cause some delays and */
							/* decrease precision of timing */
#	define PCM_CAP_COPROC		0x00000800	/* Has a coprocessor */
							/* Sometimes it's a DSP */
							/* but usually not */
#	define PCM_CAP_TRIGGER		0x00001000	/* Supports SETTRIGGER */
#	define PCM_CAP_MMAP		0x00002000	/* Supports mmap() */
#	define PCM_CAP_MULTI		0x00004000	/* Supports multiple open */
#	define PCM_CAP_BIND		0x00008000	/* Supports binding to front/rear/center/lfe */
#   	define PCM_CAP_INPUT		0x00010000	/* Supports recording */
#   	define PCM_CAP_OUTPUT		0x00020000	/* Supports playback */
#	define PCM_CAP_VIRTUAL		0x00040000	/* Virtual device */
/* 0x00040000 and 0x00080000 reserved for future use */

/* Analog/digital control capabilities */
#	define PCM_CAP_ANALOGOUT	0x00100000
#	define PCM_CAP_ANALOGIN		0x00200000
#	define PCM_CAP_DIGITALOUT	0x00400000
#	define PCM_CAP_DIGITALIN	0x00800000
#	define PCM_CAP_ADMASK		0x00f00000
/*
 * NOTE! (capabilities & PCM_CAP_ADMASK)==0 means just that the
 * digital/analog interface control features are not supported by the 
 * device/driver. However the device still supports analog, digital or
 * both inputs/outputs (depending on the device). See the OSS Programmer's
 * Guide for full details.
 */
#	define PCM_CAP_SHADOW		0x01000000	/* "Shadow" device */

/*
 * Preferred channel usage. These bits can be used to
 * give recommendations to the application. Used by few drivers.
 * For example if ((caps & DSP_CH_MASK) == DSP_CH_MONO) means that
 * the device works best in mono mode. However it doesn't necessarily mean
 * that the device cannot be used in stereo. These bits should only be used
 * by special applications such as multi track hard disk recorders to find
 * out the initial setup. However the user should be able to override this
 * selection.
 *
 * To find out which modes are actually supported the application should 
 * try to select them using SNDCTL_DSP_CHANNELS.
 */
#	define DSP_CH_MASK		0x06000000	/* Mask */
#	define DSP_CH_ANY		0x00000000	/* No preferred mode */
#	define DSP_CH_MONO		0x02000000
#	define DSP_CH_STEREO		0x04000000
#	define DSP_CH_MULTI		0x06000000	/* More than two channels */

#	define PCM_CAP_HIDDEN		0x08000000	/* Hidden device */
#	define PCM_CAP_FREERATE		0x10000000
#	define PCM_CAP_MODEM		0x20000000	/* Modem device */
#	define PCM_CAP_DEFAULT		0x40000000	/* "Default" device */

/*
 * The PCM_CAP_* capability names were known as DSP_CAP_* prior OSS 4.0
 * so it's necessary to define the older names too.
 */
#define DSP_CAP_ADMASK		PCM_CAP_ADMASK
#define DSP_CAP_ANALOGIN	PCM_CAP_ANALOGIN
#define DSP_CAP_ANALOGOUT	PCM_CAP_ANALOGOUT
#define DSP_CAP_BATCH		PCM_CAP_BATCH
#define DSP_CAP_BIND		PCM_CAP_BIND
#define DSP_CAP_COPROC		PCM_CAP_COPROC
#define DSP_CAP_DEFAULT		PCM_CAP_DEFAULT
#define DSP_CAP_DIGITALIN	PCM_CAP_DIGITALIN
#define DSP_CAP_DIGITALOUT	PCM_CAP_DIGITALOUT
#define DSP_CAP_DUPLEX		PCM_CAP_DUPLEX
#define DSP_CAP_FREERATE	PCM_CAP_FREERATE
#define DSP_CAP_HIDDEN		PCM_CAP_HIDDEN
#define DSP_CAP_INPUT		PCM_CAP_INPUT
#define DSP_CAP_MMAP		PCM_CAP_MMAP
#define DSP_CAP_MODEM		PCM_CAP_MODEM
#define DSP_CAP_MULTI		PCM_CAP_MULTI
#define DSP_CAP_OUTPUT		PCM_CAP_OUTPUT
#define DSP_CAP_REALTIME	PCM_CAP_REALTIME
#define DSP_CAP_REVISION	PCM_CAP_REVISION
#define DSP_CAP_SHADOW		PCM_CAP_SHADOW
#define DSP_CAP_TRIGGER		PCM_CAP_TRIGGER
#define DSP_CAP_VIRTUAL		PCM_CAP_VIRTUAL

#define SNDCTL_DSP_GETTRIGGER		__SIOR ('P',16, int)
#define SNDCTL_DSP_SETTRIGGER		__SIOW ('P',16, int)
#	define PCM_ENABLE_INPUT		0x00000001
#	define PCM_ENABLE_OUTPUT	0x00000002

typedef struct count_info
{
  unsigned int bytes;		/* Total # of bytes processed */
  int blocks;			/* # of fragment transitions since last time */
  int ptr;			/* Current DMA pointer value */
} count_info;

#define SNDCTL_DSP_GETIPTR		__SIOR ('P',17, count_info)
#define SNDCTL_DSP_GETOPTR		__SIOR ('P',18, count_info)

typedef struct buffmem_desc
{
  unsigned *buffer;
  int size;
} buffmem_desc;
#define SNDCTL_DSP_SETSYNCRO		__SIO  ('P', 21)
#define SNDCTL_DSP_SETDUPLEX		__SIO  ('P', 22)

#define SNDCTL_DSP_PROFILE		__SIOW ('P', 23, int)	/* OBSOLETE */
#define	  APF_NORMAL	0	/* Normal applications */
#define	  APF_NETWORK	1	/* Underruns probably caused by an "external" delay */
#define   APF_CPUINTENS 2	/* Underruns probably caused by "overheating" the CPU */

#define SNDCTL_DSP_GETODELAY		__SIOR ('P', 23, int)

typedef struct audio_errinfo
{
  int play_underruns;
  int rec_overruns;
  unsigned int play_ptradjust;
  unsigned int rec_ptradjust;
  int play_errorcount;
  int rec_errorcount;
  int play_lasterror;
  int rec_lasterror;
  int play_errorparm;
  int rec_errorparm;
  int filler[16];
} audio_errinfo;

#define SNDCTL_DSP_GETPLAYVOL		__SIOR ('P', 24, int)
#define SNDCTL_DSP_SETPLAYVOL		__SIOWR('P', 24, int)
#define SNDCTL_DSP_GETERROR		__SIOR ('P', 25, audio_errinfo)
/*
 ****************************************************************************
 * Digital interface (S/PDIF) control interface
 */

typedef struct oss_digital_control
{
  unsigned int caps;
#define DIG_CBITIN_NONE			0x00000000
#define DIG_CBITIN_LIMITED		0x00000001
#define DIG_CBITIN_DATA 		0x00000002
#define DIG_CBITIN_BYTE0		0x00000004
#define DIG_CBITIN_FULL 		0x00000008
#define DIG_CBITIN_MASK 		0x0000000f
#define DIG_CBITOUT_NONE		0x00000000
#define DIG_CBITOUT_LIMITED		0x00000010
#define DIG_CBITOUT_BYTE0		0x00000020
#define DIG_CBITOUT_FULL 		0x00000040
#define DIG_CBITOUT_DATA 		0x00000080
#define DIG_CBITOUT_MASK 		0x000000f0
#define DIG_UBITIN			0x00000100
#define DIG_UBITOUT			0x00000200
#define DIG_VBITOUT			0x00000400
#define DIG_OUTRATE			0x00000800
#define DIG_INRATE			0x00001000
#define DIG_INBITS			0x00002000
#define DIG_OUTBITS			0x00004000
#define DIG_EXACT			0x00010000
#define DIG_PRO				0x00020000
#define DIG_CONSUMER			0x00040000
#define DIG_PASSTHROUGH			0x00080000
#define DIG_OUTSEL			0x00100000

  unsigned int valid;
#define VAL_CBITIN			0x00000001
#define VAL_UBITIN			0x00000002
#define VAL_CBITOUT			0x00000004
#define VAL_UBITOUT			0x00000008
#define VAL_ISTATUS			0x00000010
#define VAL_IRATE			0x00000020
#define VAL_ORATE			0x00000040
#define VAL_INBITS			0x00000080
#define VAL_OUTBITS			0x00000100
#define VAL_REQUEST			0x00000200
#define VAL_OUTSEL			0x00000400

#define VAL_OUTMASK (VAL_CBITOUT|VAL_UBITOUT|VAL_ORATE|VAL_OUTBITS|VAL_OUTSEL)

  unsigned int request, param;
#define SPD_RQ_PASSTHROUGH				1

  unsigned char cbitin[24];
  unsigned char ubitin[24];
  unsigned char cbitout[24];
  unsigned char ubitout[24];

  unsigned int outsel;
#define OUTSEL_DIGITAL		1
#define OUTSEL_ANALOG		2
#define OUTSEL_BOTH		(OUTSEL_DIGITAL|OUTSEL_ANALOG)

  int in_data;			/* Audio/data if autodetectable by the receiver */
#define IND_UNKNOWN		0
#define IND_AUDIO		1
#define IND_DATA		2

  int in_locked;		/* Receiver locked */
#define LOCK_NOT_INDICATED	0
#define LOCK_UNLOCKED		1
#define LOCK_LOCKED		2

  int in_quality;		/* Input signal quality */
#define IN_QUAL_NOT_INDICATED	0
#define IN_QUAL_POOR		1
#define IN_QUAL_GOOD		2

  int in_vbit, out_vbit;	/* V bits */
#define VBIT_NOT_INDICATED	0
#define VBIT_OFF		1
#define VBIT_ON			2

  unsigned int in_errors;	/* Various input error conditions */
#define INERR_CRC		0x0001
#define INERR_QCODE_CRC		0x0002
#define INERR_PARITY		0x0004
#define INERR_BIPHASE		0x0008

  int srate_in, srate_out;
  int bits_in, bits_out;

  int filler[32];
} oss_digital_control;

#define SNDCTL_DSP_READCTL		__SIOWR('P', 26, oss_digital_control)
#define SNDCTL_DSP_WRITECTL		__SIOWR('P', 27, oss_digital_control)

/*
 ****************************************************************************
 * Sync groups for audio devices
 */
typedef struct oss_syncgroup
{
  int id;
  int mode;
  int filler[16];
} oss_syncgroup;

#define SNDCTL_DSP_SYNCGROUP		__SIOWR('P', 28, oss_syncgroup)
#define SNDCTL_DSP_SYNCSTART		__SIOW ('P', 29, int)

/*
 **************************************************************************
 * "cooked" mode enables software based conversions for sample rate, sample
 * format (bits) and number of channels (mono/stereo). These conversions are
 * required with some devices that support only one sample rate or just stereo
 * to let the applications to use other formats. The cooked mode is enabled by
 * default. However it's necessary to disable this mode when mmap() is used or
 * when very deterministic timing is required. SNDCTL_DSP_COOKEDMODE is an
 * optional call introduced in OSS 3.9.6f. It's _error return must be ignored_
 * since normally this call will return erno=EINVAL.
 *
 * SNDCTL_DSP_COOKEDMODE must be called immediately after open before doing
 * anything else. Otherwise the call will not have any effect.
 */
#define SNDCTL_DSP_COOKEDMODE		__SIOW ('P', 30, int)

/*
 **************************************************************************
 * SNDCTL_DSP_SILENCE and SNDCTL_DSP_SKIP are new calls in OSS 3.99.0
 * that can be used to implement pause/continue during playback (no effect
 * on recording).
 */
#define SNDCTL_DSP_SILENCE		__SIO  ('P', 31)
#define SNDCTL_DSP_SKIP			__SIO  ('P', 32)
/*
 ****************************************************************************
 * Abort transfer (reset) functions for input and output
 */
#define SNDCTL_DSP_HALT_INPUT		__SIO  ('P', 33)
#define SNDCTL_DSP_RESET_INPUT	SNDCTL_DSP_HALT_INPUT	/* Old name */
#define SNDCTL_DSP_HALT_OUTPUT		__SIO  ('P', 34)
#define SNDCTL_DSP_RESET_OUTPUT	SNDCTL_DSP_HALT_OUTPUT	/* Old name */
/*
 ****************************************************************************
 * Low water level control
 */
#define SNDCTL_DSP_LOW_WATER		__SIOW ('P', 34, int)

/*
 ****************************************************************************
 * 64 bit pointer support. Only available in environments that support
 * the 64 bit (long long) integer type.
 */
#ifndef OSS_NO_LONG_LONG
typedef struct
{
  long long samples;
  int fifo_samples;
  int filler[32];		/* For future use */
} oss_count_t;

#define SNDCTL_DSP_CURRENT_IPTR		__SIOR ('P', 35, oss_count_t)
#define SNDCTL_DSP_CURRENT_OPTR		__SIOR ('P', 36, oss_count_t)
#endif

/*
 ****************************************************************************
 * Interface for selecting recording sources and playback output routings.
 */
#define SNDCTL_DSP_GET_RECSRC_NAMES	__SIOR ('P', 37, oss_mixer_enuminfo)
#define SNDCTL_DSP_GET_RECSRC		__SIOR ('P', 38, int)
#define SNDCTL_DSP_SET_RECSRC		__SIOWR('P', 38, int)

#define SNDCTL_DSP_GET_PLAYTGT_NAMES	__SIOR ('P', 39, oss_mixer_enuminfo)
#define SNDCTL_DSP_GET_PLAYTGT		__SIOR ('P', 40, int)
#define SNDCTL_DSP_SET_PLAYTGT		__SIOWR('P', 40, int)
#define SNDCTL_DSP_GETRECVOL		__SIOR ('P', 41, int)
#define SNDCTL_DSP_SETRECVOL		__SIOWR('P', 41, int)

/*
 ***************************************************************************
 * Some calls for setting the channel assignment with multi channel devices
 * (see the manual for details).
 */
#ifndef OSS_NO_LONG_LONG
#define SNDCTL_DSP_GET_CHNORDER		__SIOR ('P', 42, unsigned long long)
#define SNDCTL_DSP_SET_CHNORDER		__SIOWR('P', 42, unsigned long long)
#	define CHID_UNDEF	0
#	define CHID_L		1
#	define CHID_R		2
#	define CHID_C		3
#	define CHID_LFE		4
#	define CHID_LS		5
#	define CHID_RS		6
#	define CHID_LR		7
#	define CHID_RR		8
#define CHNORDER_UNDEF		0x0000000000000000ULL
#define CHNORDER_NORMAL		0x0000000087654321ULL
#endif

#define MAX_PEAK_CHANNELS	128
typedef unsigned short oss_peaks_t[MAX_PEAK_CHANNELS];
#define SNDCTL_DSP_GETIPEAKS		__SIOR('P', 43, oss_peaks_t)
#define SNDCTL_DSP_GETOPEAKS		__SIOR('P', 44, oss_peaks_t)

#define SNDCTL_DSP_POLICY		__SIOW('P', 45, int)	/* See the manual */

/*
 ****************************************************************************
 * Few ioctl calls that are not official parts of OSS. They have been used
 * by few freeware implementations of OSS.
 */
#define SNDCTL_DSP_GETCHANNELMASK	__SIOWR('P', 64, int)
#define SNDCTL_DSP_BIND_CHANNEL		__SIOWR('P', 65, int)
#     define DSP_BIND_QUERY           0x00000000
#     define DSP_BIND_FRONT           0x00000001
#     define DSP_BIND_SURR            0x00000002
#     define DSP_BIND_CENTER_LFE      0x00000004
#     define DSP_BIND_HANDSET         0x00000008
#     define DSP_BIND_MIC             0x00000010
#     define DSP_BIND_MODEM1          0x00000020
#     define DSP_BIND_MODEM2          0x00000040
#     define DSP_BIND_I2S             0x00000080
#     define DSP_BIND_SPDIF           0x00000100
#     define DSP_BIND_REAR            0x00000200

#ifdef sun
/* Not part of OSS. Reserved for internal use by Solaris */
#define X_SADA_GET_PLAYTGT_MASK	__SIOR ('P', 66, int)
#define X_SADA_GET_PLAYTGT	__SIOR ('P', 67, int)
#define X_SADA_SET_PLAYTGT	__SIOWR('P', 68, int)
#endif

#ifndef NO_LEGACY_MIXER
/*
 ****************************************************************************
 * IOCTL commands for the "legacy " /dev/mixer API (obsolete)
 *
 * Mixer controls
 *
 * There can be up to 20 different analog mixer channels. The
 * SOUND_MIXER_NRDEVICES gives the currently supported maximum. 
 * The SOUND_MIXER_READ_DEVMASK returns a bitmask which tells
 * the devices supported by the particular mixer.
 *
 * {!notice This "legacy" mixer API is obsolete. It has been superseded
 * by a new one (see below).
 */

#define SOUND_MIXER_NRDEVICES	28
#define SOUND_MIXER_VOLUME	0
#define SOUND_MIXER_BASS	1
#define SOUND_MIXER_TREBLE	2
#define SOUND_MIXER_SYNTH	3
#define SOUND_MIXER_PCM		4
#define SOUND_MIXER_SPEAKER	5
#define SOUND_MIXER_LINE	6
#define SOUND_MIXER_MIC		7
#define SOUND_MIXER_CD		8
#define SOUND_MIXER_IMIX	9	/*  Recording monitor  */
#define SOUND_MIXER_ALTPCM	10
#define SOUND_MIXER_RECLEV	11	/* Recording level */
#define SOUND_MIXER_IGAIN	12	/* Input gain */
#define SOUND_MIXER_OGAIN	13	/* Output gain */
/* 
 * Some soundcards have three line level inputs (line, aux1 and aux2). 
 * Since each card manufacturer has assigned different meanings to 
 * these inputs, it's impractical to assign specific meanings 
 * (eg line, cd, synth etc.) to them.
 */
#define SOUND_MIXER_LINE1	14	/* Input source 1  (aux1) */
#define SOUND_MIXER_LINE2	15	/* Input source 2  (aux2) */
#define SOUND_MIXER_LINE3	16	/* Input source 3  (line) */
#define SOUND_MIXER_DIGITAL1	17	/* Digital I/O 1 */
#define SOUND_MIXER_DIGITAL2	18	/* Digital I/O 2 */
#define SOUND_MIXER_DIGITAL3	19	/* Digital I/O 3 */
#define SOUND_MIXER_PHONE	20	/* Phone */
#define SOUND_MIXER_MONO	21	/* Mono Output */
#define SOUND_MIXER_VIDEO	22	/* Video/TV (audio) in */
#define SOUND_MIXER_RADIO	23	/* Radio in */
#define SOUND_MIXER_DEPTH	24	/* Surround depth */
#define SOUND_MIXER_REARVOL	25	/* Rear/Surround speaker vol */
#define SOUND_MIXER_CENTERVOL	26	/* Center/LFE speaker vol */
#define SOUND_MIXER_SIDEVOL	27	/* Side-Surround (8speaker) vol */

/*
 * Warning: SOUND_MIXER_SURRVOL is an old name of SOUND_MIXER_SIDEVOL.
 *          They are both assigned to the same mixer control. Don't
 *          use both control names in the same program/driver.
 */
#define SOUND_MIXER_SURRVOL	SOUND_MIXER_SIDEVOL

/* Some on/off settings (SOUND_SPECIAL_MIN - SOUND_SPECIAL_MAX) */
/* Not counted to SOUND_MIXER_NRDEVICES, but use the same number space */
#define SOUND_ONOFF_MIN		28
#define SOUND_ONOFF_MAX		30

/* Note!	Number 31 cannot be used since the sign bit is reserved */
#define SOUND_MIXER_NONE	31

/*
 * The following unsupported macros are no longer functional.
 * Use SOUND_MIXER_PRIVATE# macros in future.
 */
#define SOUND_MIXER_ENHANCE	SOUND_MIXER_NONE
#define SOUND_MIXER_MUTE	SOUND_MIXER_NONE
#define SOUND_MIXER_LOUD	SOUND_MIXER_NONE

#define SOUND_DEVICE_LABELS \
	{"Vol  ", "Bass ", "Treble", "Synth", "Pcm  ", "Speaker ", "Line ", \
	 "Mic  ", "CD   ", "Mix  ", "Pcm2 ", "Rec  ", "IGain", "OGain", \
	 "Aux1", "Aux2", "Aux3", "Digital1", "Digital2", "Digital3", \
	 "Phone", "Mono", "Video", "Radio", "Depth", \
	 "Rear", "Center", "Side"}

#define SOUND_DEVICE_NAMES \
	{"vol", "bass", "treble", "synth", "pcm", "speaker", "line", \
	 "mic", "cd", "mix", "pcm2", "rec", "igain", "ogain", \
	 "aux1", "aux2", "aux3", "dig1", "dig2", "dig3", \
	 "phone", "mono", "video", "radio", "depth", \
	 "rear", "center", "side"}

/*	Device bitmask identifiers	*/

#define SOUND_MIXER_RECSRC	0xff	/* Arg contains a bit for each recording source */
#define SOUND_MIXER_DEVMASK	0xfe	/* Arg contains a bit for each supported device */
#define SOUND_MIXER_RECMASK	0xfd	/* Arg contains a bit for each supported recording source */
#define SOUND_MIXER_CAPS	0xfc
#	define SOUND_CAP_EXCL_INPUT	0x00000001	/* Only one recording source at a time */
#	define SOUND_CAP_NOLEGACY	0x00000004	/* For internal use only */
#	define SOUND_CAP_NORECSRC	0x00000008
#define SOUND_MIXER_STEREODEVS	0xfb	/* Mixer channels supporting stereo */

/* OSS/Free ONLY */
#define SOUND_MIXER_OUTSRC    0xfa	/* Arg contains a bit for each input source to output */
#define SOUND_MIXER_OUTMASK   0xf9	/* Arg contains a bit for each supported input source to output */
/* OSS/Free ONLY */

/*	Device mask bits	*/

#define SOUND_MASK_VOLUME	(1 << SOUND_MIXER_VOLUME)
#define SOUND_MASK_BASS		(1 << SOUND_MIXER_BASS)
#define SOUND_MASK_TREBLE	(1 << SOUND_MIXER_TREBLE)
#define SOUND_MASK_SYNTH	(1 << SOUND_MIXER_SYNTH)
#define SOUND_MASK_PCM		(1 << SOUND_MIXER_PCM)
#define SOUND_MASK_SPEAKER	(1 << SOUND_MIXER_SPEAKER)
#define SOUND_MASK_LINE		(1 << SOUND_MIXER_LINE)
#define SOUND_MASK_MIC		(1 << SOUND_MIXER_MIC)
#define SOUND_MASK_CD		(1 << SOUND_MIXER_CD)
#define SOUND_MASK_IMIX		(1 << SOUND_MIXER_IMIX)
#define SOUND_MASK_ALTPCM	(1 << SOUND_MIXER_ALTPCM)
#define SOUND_MASK_RECLEV	(1 << SOUND_MIXER_RECLEV)
#define SOUND_MASK_IGAIN	(1 << SOUND_MIXER_IGAIN)
#define SOUND_MASK_OGAIN	(1 << SOUND_MIXER_OGAIN)
#define SOUND_MASK_LINE1	(1 << SOUND_MIXER_LINE1)
#define SOUND_MASK_LINE2	(1 << SOUND_MIXER_LINE2)
#define SOUND_MASK_LINE3	(1 << SOUND_MIXER_LINE3)
#define SOUND_MASK_DIGITAL1	(1 << SOUND_MIXER_DIGITAL1)
#define SOUND_MASK_DIGITAL2	(1 << SOUND_MIXER_DIGITAL2)
#define SOUND_MASK_DIGITAL3	(1 << SOUND_MIXER_DIGITAL3)
#define SOUND_MASK_MONO		(1 << SOUND_MIXER_MONO)
#define SOUND_MASK_PHONE	(1 << SOUND_MIXER_PHONE)
#define SOUND_MASK_RADIO	(1 << SOUND_MIXER_RADIO)
#define SOUND_MASK_VIDEO	(1 << SOUND_MIXER_VIDEO)
#define SOUND_MASK_DEPTH	(1 << SOUND_MIXER_DEPTH)
#define SOUND_MASK_REARVOL	(1 << SOUND_MIXER_REARVOL)
#define SOUND_MASK_CENTERVOL	(1 << SOUND_MIXER_CENTERVOL)
#define SOUND_MASK_SIDEVOL	(1 << SOUND_MIXER_SIDEVOL)

/* Note! SOUND_MASK_SURRVOL is alias of SOUND_MASK_SIDEVOL */
#define SOUND_MASK_SURRVOL	(1 << SOUND_MIXER_SIDEVOL)

/* Obsolete macros */
#define SOUND_MASK_MUTE		(1 << SOUND_MIXER_MUTE)
#define SOUND_MASK_ENHANCE	(1 << SOUND_MIXER_ENHANCE)
#define SOUND_MASK_LOUD		(1 << SOUND_MIXER_LOUD)

#define MIXER_READ(dev)			__SIOR('M', dev, int)
#define SOUND_MIXER_READ_VOLUME		MIXER_READ(SOUND_MIXER_VOLUME)
#define SOUND_MIXER_READ_BASS		MIXER_READ(SOUND_MIXER_BASS)
#define SOUND_MIXER_READ_TREBLE		MIXER_READ(SOUND_MIXER_TREBLE)
#define SOUND_MIXER_READ_SYNTH		MIXER_READ(SOUND_MIXER_SYNTH)
#define SOUND_MIXER_READ_PCM		MIXER_READ(SOUND_MIXER_PCM)
#define SOUND_MIXER_READ_SPEAKER	MIXER_READ(SOUND_MIXER_SPEAKER)
#define SOUND_MIXER_READ_LINE		MIXER_READ(SOUND_MIXER_LINE)
#define SOUND_MIXER_READ_MIC		MIXER_READ(SOUND_MIXER_MIC)
#define SOUND_MIXER_READ_CD		MIXER_READ(SOUND_MIXER_CD)
#define SOUND_MIXER_READ_IMIX		MIXER_READ(SOUND_MIXER_IMIX)
#define SOUND_MIXER_READ_ALTPCM		MIXER_READ(SOUND_MIXER_ALTPCM)
#define SOUND_MIXER_READ_RECLEV		MIXER_READ(SOUND_MIXER_RECLEV)
#define SOUND_MIXER_READ_IGAIN		MIXER_READ(SOUND_MIXER_IGAIN)
#define SOUND_MIXER_READ_OGAIN		MIXER_READ(SOUND_MIXER_OGAIN)
#define SOUND_MIXER_READ_LINE1		MIXER_READ(SOUND_MIXER_LINE1)
#define SOUND_MIXER_READ_LINE2		MIXER_READ(SOUND_MIXER_LINE2)
#define SOUND_MIXER_READ_LINE3		MIXER_READ(SOUND_MIXER_LINE3)

/* Obsolete macros */
#define SOUND_MIXER_READ_MUTE		MIXER_READ(SOUND_MIXER_MUTE)
#define SOUND_MIXER_READ_ENHANCE	MIXER_READ(SOUND_MIXER_ENHANCE)
#define SOUND_MIXER_READ_LOUD		MIXER_READ(SOUND_MIXER_LOUD)

#define SOUND_MIXER_READ_RECSRC		MIXER_READ(SOUND_MIXER_RECSRC)
#define SOUND_MIXER_READ_DEVMASK	MIXER_READ(SOUND_MIXER_DEVMASK)
#define SOUND_MIXER_READ_RECMASK	MIXER_READ(SOUND_MIXER_RECMASK)
#define SOUND_MIXER_READ_STEREODEVS	MIXER_READ(SOUND_MIXER_STEREODEVS)
#define SOUND_MIXER_READ_CAPS		MIXER_READ(SOUND_MIXER_CAPS)

#define MIXER_WRITE(dev)		__SIOWR('M', dev, int)
#define SOUND_MIXER_WRITE_VOLUME	MIXER_WRITE(SOUND_MIXER_VOLUME)
#define SOUND_MIXER_WRITE_BASS		MIXER_WRITE(SOUND_MIXER_BASS)
#define SOUND_MIXER_WRITE_TREBLE	MIXER_WRITE(SOUND_MIXER_TREBLE)
#define SOUND_MIXER_WRITE_SYNTH		MIXER_WRITE(SOUND_MIXER_SYNTH)
#define SOUND_MIXER_WRITE_PCM		MIXER_WRITE(SOUND_MIXER_PCM)
#define SOUND_MIXER_WRITE_SPEAKER	MIXER_WRITE(SOUND_MIXER_SPEAKER)
#define SOUND_MIXER_WRITE_LINE		MIXER_WRITE(SOUND_MIXER_LINE)
#define SOUND_MIXER_WRITE_MIC		MIXER_WRITE(SOUND_MIXER_MIC)
#define SOUND_MIXER_WRITE_CD		MIXER_WRITE(SOUND_MIXER_CD)
#define SOUND_MIXER_WRITE_IMIX		MIXER_WRITE(SOUND_MIXER_IMIX)
#define SOUND_MIXER_WRITE_ALTPCM	MIXER_WRITE(SOUND_MIXER_ALTPCM)
#define SOUND_MIXER_WRITE_RECLEV	MIXER_WRITE(SOUND_MIXER_RECLEV)
#define SOUND_MIXER_WRITE_IGAIN		MIXER_WRITE(SOUND_MIXER_IGAIN)
#define SOUND_MIXER_WRITE_OGAIN		MIXER_WRITE(SOUND_MIXER_OGAIN)
#define SOUND_MIXER_WRITE_LINE1		MIXER_WRITE(SOUND_MIXER_LINE1)
#define SOUND_MIXER_WRITE_LINE2		MIXER_WRITE(SOUND_MIXER_LINE2)
#define SOUND_MIXER_WRITE_LINE3		MIXER_WRITE(SOUND_MIXER_LINE3)

/* Obsolete macros */
#define SOUND_MIXER_WRITE_MUTE		MIXER_WRITE(SOUND_MIXER_MUTE)
#define SOUND_MIXER_WRITE_ENHANCE	MIXER_WRITE(SOUND_MIXER_ENHANCE)
#define SOUND_MIXER_WRITE_LOUD		MIXER_WRITE(SOUND_MIXER_LOUD)

#define SOUND_MIXER_WRITE_RECSRC	MIXER_WRITE(SOUND_MIXER_RECSRC)

typedef struct mixer_info	/* OBSOLETE */
{
  char id[16];
  char name[32];
  int modify_counter;
  int card_number;
  int port_number;
  char handle[32];
} mixer_info;

/* SOUND_MIXER_INFO is obsolete - use SNDCTL_MIXERINFO instead */
#define SOUND_MIXER_INFO		__SIOR ('M', 101, mixer_info)

/*
 * Two ioctls for special souncard function (OSS/Free only)
 */
#define SOUND_MIXER_AGC  _SIOWR('M', 103, int)
#define SOUND_MIXER_3DSE  _SIOWR('M', 104, int)
/*
 * The SOUND_MIXER_PRIVATE# commands can be redefined by low level drivers.
 * These features can be used when accessing device specific features.
 */
#define SOUND_MIXER_PRIVATE1		__SIOWR('M', 111, int)
#define SOUND_MIXER_PRIVATE2		__SIOWR('M', 112, int)
#define SOUND_MIXER_PRIVATE3		__SIOWR('M', 113, int)
#define SOUND_MIXER_PRIVATE4		__SIOWR('M', 114, int)
#define SOUND_MIXER_PRIVATE5		__SIOWR('M', 115, int)

/* The following two controls were never implemented and they should not be used. */
#define SOUND_MIXER_READ_MAINVOL		__SIOR ('M', 116, int)
#define SOUND_MIXER_WRITE_MAINVOL		__SIOWR('M', 116, int)

/*
 * SOUND_MIXER_GETLEVELS and SOUND_MIXER_SETLEVELS calls can be used
 * for querying current mixer settings from the driver and for loading
 * default volume settings _prior_ activating the mixer (loading
 * doesn't affect current state of the mixer hardware). These calls
 * are for internal use by the driver software only.
 */

typedef struct mixer_vol_table
{
  int num;			/* Index to volume table */
  char name[32];
  int levels[32];
} mixer_vol_table;

#define SOUND_MIXER_GETLEVELS		__SIOWR('M', 116, mixer_vol_table)
#define SOUND_MIXER_SETLEVELS		__SIOWR('M', 117, mixer_vol_table)

#define OSS_GETVERSION			__SIOR ('M', 118, int)

/*
 * Calls to set/get the recording gain for the currently active
 * recording source. These calls automatically map to the right control.
 * Note that these calls are not supported by all drivers. In this case
 * the call will return -1 with errno set to EINVAL
 *
 * The _MONGAIN work in similar way but set/get the monitoring gain for
 * the currently selected recording source.
 */
#define SOUND_MIXER_READ_RECGAIN	__SIOR ('M', 119, int)
#define SOUND_MIXER_WRITE_RECGAIN	__SIOWR('M', 119, int)
#define SOUND_MIXER_READ_MONGAIN	__SIOR ('M', 120, int)
#define SOUND_MIXER_WRITE_MONGAIN	__SIOWR('M', 120, int)

/* The following call is for driver development time purposes. It's not
 * present in any released drivers.
 */
typedef unsigned char oss_reserved_t[512];
#define SOUND_MIXER_RESERVED		__SIOWR('M', 121, oss_reserved_t)
#endif /* ifndef NO_LEGACY_MIXER */

/*
 *************************************************************************
 * The "new" mixer API of OSS 4.0 and later.
 *
 * This improved mixer API makes it possible to access every possible feature
 * of every possible device. However you should read the mixer programming
 * section of the OSS API Developer's Manual. There is no chance that you
 * could use this interface correctly just by examining this header.
 */

typedef struct oss_sysinfo
{
  char product[32];		/* For example OSS/Free, OSS/Linux or OSS/Solaris */
  char version[32];		/* For example 4.0a */
  int versionnum;		/* See OSS_GETVERSION */
  char options[128];		/* Reserved */

  int numaudios;		/* # of audio/dsp devices */
  int openedaudio[8];		/* Bit mask telling which audio devices are busy */

  int numsynths;		/* # of available synth devices */
  int nummidis;			/* # of available MIDI ports */
  int numtimers;		/* # of available timer devices */
  int nummixers;		/* # of mixer devices */

  int openedmidi[8];		/* Bit mask telling which midi devices are busy */
  int numcards;			/* Number of sound cards in the system */
  int numaudioengines;		/* Number of audio engines in the system */
  char license[16];		/* For example "GPL" or "CDDL" */
  int filler[236];		/* For future expansion (set to -1) */
} oss_sysinfo;

typedef struct oss_mixext
{
  int dev;			/* Mixer device number */
  int ctrl;			/* Controller number */
  int type;			/* Entry type */
#	define MIXT_DEVROOT	 0	/* Device root entry */
#	define MIXT_GROUP	 1	/* Controller group */
#	define MIXT_ONOFF	 2	/* OFF (0) or ON (1) */
#	define MIXT_ENUM	 3	/* Enumerated (0 to maxvalue) */
#	define MIXT_MONOSLIDER	 4	/* Mono slider (0 to 255) */
#	define MIXT_STEREOSLIDER 5	/* Stereo slider (dual 0 to 255) */
#	define MIXT_MESSAGE	 6	/* (Readable) textual message */
#	define MIXT_MONOVU	 7	/* VU meter value (mono) */
#	define MIXT_STEREOVU	 8	/* VU meter value (stereo) */
#	define MIXT_MONOPEAK	 9	/* VU meter peak value (mono) */
#	define MIXT_STEREOPEAK	10	/* VU meter peak value (stereo) */
#	define MIXT_RADIOGROUP	11	/* Radio button group */
#	define MIXT_MARKER	12	/* Separator between normal and extension entries */
#	define MIXT_VALUE	13	/* Decimal value entry */
#	define MIXT_HEXVALUE	14	/* Hexadecimal value entry */
#	define MIXT_MONODB	15	/* OBSOLETE */
#	define MIXT_STEREODB	16	/* OBSOLETE */
#	define MIXT_SLIDER	17	/* Slider (mono) with full (31 bit) postitive integer range */
#	define MIXT_3D		18

/*
 * Sliders with range expanded to 15 bits per channel (0-32767)
 */
#	define MIXT_MONOSLIDER16	19
#	define MIXT_STEREOSLIDER16	20
#	define MIXT_MUTE	21	/* Mute=1, unmute=0 */

  /**************************************************************/

  /* Possible value range (minvalue to maxvalue) */
  /* Note that maxvalue may also be smaller than minvalue */
  int maxvalue;
  int minvalue;

  int flags;
#	define MIXF_READABLE	0x00000001	/* Has readable value */
#	define MIXF_WRITEABLE	0x00000002	/* Has writeable value */
#	define MIXF_POLL	0x00000004	/* May change itself */
#	define MIXF_HZ		0x00000008	/* Herz scale */
#	define MIXF_STRING	0x00000010	/* Use dynamic extensions for value */
#	define MIXF_DYNAMIC	0x00000010	/* Supports dynamic extensions */
#	define MIXF_OKFAIL	0x00000020	/* Interpret value as 1=OK, 0=FAIL */
#	define MIXF_FLAT	0x00000040	/* Flat vertical space requirements */
#	define MIXF_LEGACY	0x00000080	/* Legacy mixer control group */
#	define MIXF_CENTIBEL	0x00000100	/* Centibel (0.1 dB) step size */
#	define MIXF_DECIBEL	0x00000200	/* Step size of 1 dB */
#	define MIXF_MAINVOL	0x00000400	/* Main volume control */
#	define MIXF_PCMVOL	0x00000800	/* PCM output volume control */
#	define MIXF_RECVOL	0x00001000	/* PCM recording volume control */
#	define MIXF_MONVOL	0x00002000	/* Input->output monitor volume */
#	define MIXF_WIDE	0x00004000	/* Enum control has wide labels */
#	define MIXF_DESCR	0x00008000	/* Description (tooltip) available */
  char id[16];			/* Mnemonic ID (mainly for internal use) */
  int parent;			/* Entry# of parent (group) node (-1 if root) */

  int dummy;			/* Internal use */

  int timestamp;

  char data[64];		/* Misc data (entry type dependent) */
  unsigned char enum_present[32];	/* Mask of allowed enum values */
  int control_no;		/* SOUND_MIXER_VOLUME..SOUND_MIXER_MIDI */
  /* (-1 means not indicated) */

/*
 * The desc field is reserved for internal purposes of OSS. It should not be 
 * used by applications.
 */
  unsigned int desc;
#define MIXEXT_SCOPE_MASK			0x0000003f
#define MIXEXT_SCOPE_OTHER			0x00000000
#define MIXEXT_SCOPE_INPUT			0x00000001
#define MIXEXT_SCOPE_OUTPUT			0x00000002
#define MIXEXT_SCOPE_MONITOR			0x00000003
#define MIXEXT_SCOPE_RECSWITCH			0x00000004

  char extname[32];
  int update_counter;
  int rgbcolor;		/* 0 means default color (not black) . Otherwise 24 bit RGB color */
  int filler[6];
} oss_mixext;

/*
 * Recommended colors to be used in the rgbcolor field. These match the
 * colors used as the audio jack colors in HD audio motherboards.
 */
#define OSS_RGB_BLUE	0x7aabde		// Light blue
#define OSS_RGB_GREEN	0xb3c98c		// Lime green
#define OSS_RGB_PINK	0xe88c99
#define OSS_RGB_GRAY	0xd1ccc4
#define OSS_RGB_BLACK	0x2b2926		// Light black
#define OSS_RGB_ORANGE	0xe89e47
#define OSS_RGB_RED	0xff0000
#define OSS_RGB_YELLOW	0xffff00
#define OSS_RGB_PURPLE	0x800080
#define OSS_RGB_WHITE	0xf8f8ff

typedef struct oss_mixext_root
{
  char id[16];
  char name[48];
} oss_mixext_root;

typedef struct oss_mixer_value
{
  int dev;
  int ctrl;
  int value;
  int flags;			/* Reserved for future use. Initialize to 0 */
  int timestamp;		/* Must be set to oss_mixext.timestamp */
  int filler[8];		/* Reserved for future use. Initialize to 0 */
} oss_mixer_value;

#define OSS_ENUM_MAXVALUE	255
#define OSS_ENUM_STRINGSIZE	3000
typedef struct oss_mixer_enuminfo
{
  int dev;
  int ctrl;
  int nvalues;
  int version;			/* Read the manual */
  short strindex[OSS_ENUM_MAXVALUE];
  char strings[OSS_ENUM_STRINGSIZE];
} oss_mixer_enuminfo;

#define OPEN_READ	PCM_ENABLE_INPUT
#define OPEN_WRITE	PCM_ENABLE_OUTPUT
#define OPEN_READWRITE	(OPEN_READ|OPEN_WRITE)

typedef struct oss_audioinfo
{
  int dev;			/* Audio device number */
  char name[64];
  int busy;			/* 0, OPEN_READ, OPEN_WRITE or OPEN_READWRITE */
  int pid;
  int caps;			/* PCM_CAP_INPUT, PCM_CAP_OUTPUT */
  int iformats, oformats;
  int magic;			/* Reserved for internal use */
  char cmd[64];			/* Command using the device (if known) */
  int card_number;
  int port_number;
  int mixer_dev;
  int legacy_device;		/* Obsolete field. Replaced by devnode */
  int enabled;			/* 1=enabled, 0=device not ready at this moment */
  int flags;			/* For internal use only - no practical meaning */
  int min_rate, max_rate;	/* Sample rate limits */
  int min_channels, max_channels;	/* Number of channels supported */
  int binding;			/* DSP_BIND_FRONT, etc. 0 means undefined */
  int rate_source;
  char handle[32];
#define OSS_MAX_SAMPLE_RATES	20	/* Cannot be changed  */
  unsigned int nrates, rates[OSS_MAX_SAMPLE_RATES];	/* Please read the manual before using these */
  oss_longname_t song_name;	/* Song name (if given) */
  oss_label_t label;		/* Device label (if given) */
  int latency;			/* In usecs, -1=unknown */
  oss_devnode_t devnode;	/* Device special file name (absolute path) */
  int next_play_engine;		/* Read the documentation for more info */
  int next_rec_engine;		/* Read the documentation for more info */
  int filler[184];
} oss_audioinfo;

typedef struct oss_mixerinfo
{
  int dev;
  char id[16];
  char name[32];
  int modify_counter;
  int card_number;
  int port_number;
  char handle[32];
  int magic;			/* Reserved */
  int enabled;			/* Reserved */
  int caps;
#define MIXER_CAP_VIRTUAL	0x00000001
#define MIXER_CAP_LAYOUT_B	0x00000002	/* For internal use only */
#define MIXER_CAP_NARROW	0x00000004	/* Conserve horiz space */
  int flags;			/* Reserved */
  int nrext;
  /*
   * The priority field can be used to select the default (motherboard)
   * mixer device. The mixer with the highest priority is the
   * most preferred one. -2 or less means that this device cannot be used
   * as the default mixer.
   */
  int priority;
  oss_devnode_t devnode;	/* Device special file name (absolute path) */
  int legacy_device;
  int filler[245];		/* Reserved */
} oss_mixerinfo;

typedef struct oss_midi_info
{
  int dev;			/* Midi device number */
  char name[64];
  int busy;			/* 0, OPEN_READ, OPEN_WRITE or OPEN_READWRITE */
  int pid;
  char cmd[64];			/* Command using the device (if known) */
  int caps;
#define MIDI_CAP_MPU401		0x00000001	/**** OBSOLETE ****/
#define MIDI_CAP_INPUT		0x00000002
#define MIDI_CAP_OUTPUT		0x00000004
#define MIDI_CAP_INOUT		(MIDI_CAP_INPUT|MIDI_CAP_OUTPUT)
#define MIDI_CAP_VIRTUAL	0x00000008	/* Pseudo device */
#define MIDI_CAP_MTCINPUT	0x00000010	/* Supports SNDCTL_MIDI_MTCINPUT */
#define MIDI_CAP_CLIENT		0x00000020	/* Virtual client side device */
#define MIDI_CAP_SERVER		0x00000040	/* Virtual server side device */
#define MIDI_CAP_INTERNAL	0x00000080	/* Internal (synth) device */
#define MIDI_CAP_EXTERNAL	0x00000100	/* external (MIDI port) device */
#define MIDI_CAP_PTOP		0x00000200	/* Point to point link to one device */
#define MIDI_CAP_MTC		0x00000400	/* MTC/SMPTE (control) device */
  int magic;			/* Reserved for internal use */
  int card_number;
  int port_number;
  int enabled;			/* 1=enabled, 0=device not ready at this moment */
  int flags;			/* For internal use only - no practical meaning */
  char handle[32];
  oss_longname_t song_name;	/* Song name (if known) */
  oss_label_t label;		/* Device label (if given) */
  int latency;			/* In usecs, -1=unknown */
  oss_devnode_t devnode;	/* Device special file name (absolute path) */
  int legacy_device;		/* Legacy device mapping */
  int filler[235];
} oss_midi_info;

typedef struct oss_card_info
{
  int card;
  char shortname[16];
  char longname[128];
  int flags;
  char hw_info[400];
  int intr_count, ack_count;
  int filler[154];
} oss_card_info;

#define SNDCTL_SYSINFO		__SIOR ('X', 1, oss_sysinfo)
#define OSS_SYSINFO		SNDCTL_SYSINFO	/* Old name */

#define SNDCTL_MIX_NRMIX	__SIOR ('X', 2, int)
#define SNDCTL_MIX_NREXT	__SIOWR('X', 3, int)
#define SNDCTL_MIX_EXTINFO	__SIOWR('X', 4, oss_mixext)
#define SNDCTL_MIX_READ		__SIOWR('X', 5, oss_mixer_value)
#define SNDCTL_MIX_WRITE	__SIOWR('X', 6, oss_mixer_value)

#define SNDCTL_AUDIOINFO	__SIOWR('X', 7, oss_audioinfo)
#define SNDCTL_MIX_ENUMINFO	__SIOWR('X', 8, oss_mixer_enuminfo)
#define SNDCTL_MIDIINFO		__SIOWR('X', 9, oss_midi_info)
#define SNDCTL_MIXERINFO	__SIOWR('X',10, oss_mixerinfo)
#define SNDCTL_CARDINFO		__SIOWR('X',11, oss_card_info)
#define SNDCTL_ENGINEINFO	__SIOWR('X',12, oss_audioinfo)
#define SNDCTL_AUDIOINFO_EX	__SIOWR('X',13, oss_audioinfo)

#define SNDCTL_MIX_DESCRIPTION	__SIOWR('X',14, oss_mixer_enuminfo)

/* ioctl codes 'X', 200-255 are reserved for internal use */

/*
 * Few more "globally" available ioctl calls.
 */
#define SNDCTL_SETSONG		__SIOW ('Y', 2, oss_longname_t)
#define SNDCTL_GETSONG		__SIOR ('Y', 2, oss_longname_t)
#define SNDCTL_SETNAME		__SIOW ('Y', 3, oss_longname_t)
#define SNDCTL_SETLABEL		__SIOW ('Y', 4, oss_label_t)
#define SNDCTL_GETLABEL		__SIOR ('Y', 4, oss_label_t)
/*
 * The "new" mixer API definitions end here.
 ***************************************
 */

/*
 *********************************************************
 * Few routines that are included in -lOSSlib
 *
 * At this moment this interface is not used. OSSlib contains just
 * stubs that call the related system calls directly.
 */
#ifdef OSSLIB
extern int osslib_open (const char *path, int flags, int dummy);
extern void osslib_close (int fd);
extern int osslib_write (int fd, const void *buf, int count);
extern int osslib_read (int fd, void *buf, int count);
extern int osslib_ioctl (int fd, unsigned int request, void *arg);
#else
#  define osslib_open	open
#  define osslib_close	close
#  define osslib_write	write
#  define osslib_read	read
#  define osslib_ioctl	ioctl
#endif

#if 1
#define SNDCTL_DSP_NONBLOCK		__SIO  ('P',14)	/* Obsolete. Not supported any more */
#endif

#if 1
/*
 * Some obsolete macros that are not part of Open Sound System API.
 */
#define SOUND_PCM_READ_RATE             SOUND_PCM_READ_RATE_is_obsolete
#define SOUND_PCM_READ_BITS             SOUND_PCM_READ_BITS_is_obsolete
#define SOUND_PCM_READ_CHANNELS         SOUND_PCM_READ_CHANNELS_is_obsolete
#define SOUND_PCM_WRITE_RATE            SOUND_PCM_WRITE_RATE_is_obsolet_use_SNDCTL_DSP_SPEED_instead
#define SOUND_PCM_WRITE_CHANNELS        SOUND_PCM_WRITE_CHANNELS_is_obsolete_use_SNDCTL_DSP_CHANNELS_instead
#define SOUND_PCM_WRITE_BITS            SOUND_PCM_WRITE_BITS_is_obsolete_use_SNDCTL_DSP_SETFMT_instead
#define SOUND_PCM_POST                  SOUND_PCM_POST_is_obsolete_use_SNDCTL_DSP_POST_instead
#define SOUND_PCM_RESET                 SOUND_PCM_RESET_is_obsolete_use_SNDCTL_DSP_HALT_instead
#define SOUND_PCM_SYNC                  SOUND_PCM_SYNC_is_obsolete_use_SNDCTL_DSP_SYNC_instead
#define SOUND_PCM_SUBDIVIDE             SOUND_PCM_SUBDIVIDE_is_obsolete_use_SNDCTL_DSP_SUBDIVIDE_instead
#define SOUND_PCM_SETFRAGMENT           SOUND_PCM_SETFRAGMENT_is_obsolete_use_SNDCTL_DSP_SETFRAGMENT_instead
#define SOUND_PCM_GETFMTS               SOUND_PCM_GETFMTS_is_obsolete_use_SNDCTL_DSP_GETFMTS_instead
#define SOUND_PCM_SETFMT                SOUND_PCM_SETFMT_is_obsolete_use_SNDCTL_DSP_SETFMT_instead
#define SOUND_PCM_GETOSPACE             SOUND_PCM_GETOSPACE_is_obsolete_use_SNDCTL_DSP_GETOSPACE_instead
#define SOUND_PCM_GETISPACE             SOUND_PCM_GETISPACE_is_obsolete_use_SNDCTL_DSP_GETISPACE_instead
#define SOUND_PCM_NONBLOCK              SOUND_PCM_NONBLOCK_is_obsolete_use_SNDCTL_DSP_NONBLOCK_instead
#define SOUND_PCM_GETCAPS               SOUND_PCM_GETCAPS_is_obsolete_use_SNDCTL_DSP_GETCAPS_instead
#define SOUND_PCM_GETTRIGGER            SOUND_PCM_GETTRIGGER_is_obsolete_use_SNDCTL_DSP_GETTRIGGER_instead
#define SOUND_PCM_SETTRIGGER            SOUND_PCM_SETTRIGGER_is_obsolete_use_SNDCTL_DSP_SETTRIGGER_instead
#define SOUND_PCM_SETSYNCRO             SOUND_PCM_SETSYNCRO_is_obsolete_use_SNDCTL_DSP_SETSYNCRO_instead
#define SOUND_PCM_GETIPTR               SOUND_PCM_GETIPTR_is_obsolete_use_SNDCTL_DSP_GETIPTR_instead
#define SOUND_PCM_GETOPTR               SOUND_PCM_GETOPTR_is_obsolete_use_SNDCTL_DSP_GETOPTR_instead
#define SOUND_PCM_MAPINBUF              SOUND_PCM_MAPINBUF_is_obsolete_use_SNDCTL_DSP_MAPINBUF_instead
#define SOUND_PCM_MAPOUTBUF             SOUND_PCM_MAPOUTBUF_is_obsolete_use_SNDCTL_DSP_MAPOUTBUF_instead
#endif

#endif
