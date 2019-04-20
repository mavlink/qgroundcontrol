/* Libvisual - The audio visualisation framework.
 * 
 * Copyright (C) 2004, 2005, 2006 Dennis Smit <ds@nerds-incorporated.org>
 *
 * Authors: Dennis Smit <ds@nerds-incorporated.org>
 *
 * $Id: lv_audio.h,v 1.23 2006/01/22 13:23:37 synap Exp $
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

#ifndef _LV_AUDIO_H
#define _LV_AUDIO_H

#include <libvisual/lv_fourier.h>
#include <libvisual/lv_time.h>
#include <libvisual/lv_ringbuffer.h>
#include <libvisual/lv_hashmap.h>

VISUAL_BEGIN_DECLS

#define VISUAL_AUDIO(obj)				(VISUAL_CHECK_CAST ((obj), VisAudio))
#define VISUAL_AUDIO_SAMPLEPOOL(obj)			(VISUAL_CHECK_CAST ((obj), VisAudioSamplePool))
#define VISUAL_AUDIO_SAMPLEPOOL_CHANNEL(obj)		(VISUAL_CHECK_CAST ((obj), VisAudioSamplePoolChannel))
#define VISUAL_AUDIO_SAMPLE(obj)			(VISUAL_CHECK_CAST ((obj), VisAudioSample))

#define VISUAL_AUDIO_CHANNEL_LEFT	"front left 1"
#define VISUAL_AUDIO_CHANNEL_RIGHT	"front right 1"

#define VISUAL_AUDIO_CHANNEL_CATEGORY_FRONT	"front"
#define VISUAL_AUDIO_CHANNEL_CATEGORY_REAR	"rear"
#define VISUAL_AUDIO_CHANNEL_CATEGORY_RIGHT	"left"
#define VISUAL_AUDIO_CHANNEL_CATEGORY_LEFT	"right"

typedef enum {
	VISUAL_AUDIO_SAMPLE_RATE_NONE = 0,
	VISUAL_AUDIO_SAMPLE_RATE_8000,
	VISUAL_AUDIO_SAMPLE_RATE_11250,
	VISUAL_AUDIO_SAMPLE_RATE_22500,
	VISUAL_AUDIO_SAMPLE_RATE_32000,
	VISUAL_AUDIO_SAMPLE_RATE_44100,
	VISUAL_AUDIO_SAMPLE_RATE_48000,
	VISUAL_AUDIO_SAMPLE_RATE_96000,
	VISUAL_AUDIO_SAMPLE_RATE_LAST,
} VisAudioSampleRateType;

typedef enum {
	VISUAL_AUDIO_SAMPLE_FORMAT_NONE = 0,
	VISUAL_AUDIO_SAMPLE_FORMAT_U8,
	VISUAL_AUDIO_SAMPLE_FORMAT_S8,
	VISUAL_AUDIO_SAMPLE_FORMAT_U16,
	VISUAL_AUDIO_SAMPLE_FORMAT_S16,
	VISUAL_AUDIO_SAMPLE_FORMAT_U32,
	VISUAL_AUDIO_SAMPLE_FORMAT_S32,
	VISUAL_AUDIO_SAMPLE_FORMAT_FLOAT,
	VISUAL_AUDIO_SAMPLE_FORMAT_LAST,
} VisAudioSampleFormatType;

typedef enum {
	VISUAL_AUDIO_SAMPLE_CHANNEL_NONE = 0,
	VISUAL_AUDIO_SAMPLE_CHANNEL_STEREO
} VisAudioSampleChannelType;


typedef struct _VisAudio VisAudio;
typedef struct _VisAudioSamplePool VisAudioSamplePool;
typedef struct _VisAudioSamplePoolChannel VisAudioSamplePoolChannel;
typedef struct _VisAudioSample VisAudioSample;

/**
 * The VisAudio structure contains the sample and extra information
 * about the sample like a 256 bands analyzer, sound energy and
 * in the future BPM detection.
 *
 * @see visual_audio_new
 */
struct _VisAudio {
	VisObject		 object;			/**< The VisObject data. */

	VisAudioSamplePool	*samplepool;
	short			 plugpcm[2][512];		/**< PCM data that comes from the input plugin
								 * or a callback function. */
//	short			 pcm[3][512];			/**< PCM data that should be used within plugins
//								 * pcm[0][x] is the left channel, pcm[1][x] is the right
//								 * channel and pcm[2][x] is an average of both channels. */
//	short			 freq[3][256];			/**< Rateuency data as a 256 bands analyzer, with the channels
//								 * like with the pcm element. */
//	short			 freqnorm[3][256];		/**< Rateuency data like the freq member, however this time the bands
//								 * are normalized. */

//	short int		 bpmhistory[1024][6];		/**< Private member for BPM detection, not implemented right now. */
//	short int		 bpmdata[1024][6];		/**< Private member for BPM detection, not implemented right now. */
//	short int		 bpmenergy[6];			/**< Private member for BPM detection, not implemented right now. */
	int			 energy;			/**< Audio energy level. */
};

struct _VisAudioSamplePool {
	VisObject	 object;

	VisList		*channels;
};

struct _VisAudioSamplePoolChannel {
	VisObject	 object;

	VisRingBuffer	*samples;
	VisTime		 samples_timeout;

	char		*channelid;

	float		 factor;
};

struct _VisAudioSample {
	VisObject			 object;

	VisTime				 timestamp;

	VisAudioSampleRateType		 rate;
	VisAudioSampleFormatType	 format;

	VisBuffer			*buffer;
	VisBuffer			*processed;
};

/* prototypes */
VisAudio *visual_audio_new (void);
int visual_audio_init (VisAudio *audio);
int visual_audio_analyze (VisAudio *audio);

int visual_audio_get_sample (VisAudio *audio, VisBuffer *buffer, char *channelid);
int visual_audio_get_sample_mixed_simple (VisAudio *audio, VisBuffer *buffer, int channels, ...);
int visual_audio_get_sample_mixed (VisAudio *audio, VisBuffer *buffer, int divide, int channels, ...);
int visual_audio_get_sample_mixed_category (VisAudio *audio, VisBuffer *buffer, char *category, int divide);
int visual_audio_get_sample_mixed_all (VisAudio *audio, VisBuffer *buffer, int divide);

int visual_audio_get_spectrum (VisAudio *audio, VisBuffer *buffer, int samplelen, char *channelid, int normalised);
int visual_audio_get_spectrum_multiplied (VisAudio *audio, VisBuffer *buffer, int samplelen, char *channelid, int normalised, float multiplier);
int visual_audio_get_spectrum_for_sample (VisBuffer *buffer, VisBuffer *sample, int normalised);
int visual_audio_get_spectrum_for_sample_multiplied (VisBuffer *buffer, VisBuffer *sample, int normalised, float multiplier);

int visual_audio_normalise_spectrum (VisBuffer *buffer);

VisAudioSamplePool *visual_audio_samplepool_new (void);
int visual_audio_samplepool_init (VisAudioSamplePool *samplepool);
int visual_audio_samplepool_add (VisAudioSamplePool *samplepool, VisAudioSample *sample, char *channelid);
int visual_audio_samplepool_add_channel (VisAudioSamplePool *samplepool, VisAudioSamplePoolChannel *channel);
VisAudioSamplePoolChannel *visual_audio_samplepool_get_channel (VisAudioSamplePool *samplepool, char *channelid);
int visual_audio_samplepool_flush_old (VisAudioSamplePool *samplepool);

int visual_audio_samplepool_input (VisAudioSamplePool *samplepool, VisBuffer *buffer,
		VisAudioSampleRateType rate,
		VisAudioSampleFormatType format,
		VisAudioSampleChannelType channeltype);
int visual_audio_samplepool_input_channel (VisAudioSamplePool *samplepool, VisBuffer *buffer,
		VisAudioSampleRateType rate,
		VisAudioSampleFormatType format,
		char *channelid);

VisAudioSamplePoolChannel *visual_audio_samplepool_channel_new (char *channelid);
int visual_audio_samplepool_channel_init (VisAudioSamplePoolChannel *channel, char *channelid);
int visual_audio_samplepool_channel_add (VisAudioSamplePoolChannel *channel, VisAudioSample *sample);
int visual_audio_samplepool_channel_flush_old (VisAudioSamplePoolChannel *channel);

int visual_audio_sample_buffer_mix (VisBuffer *dest, VisBuffer *src, int divide, float multiplier);
int visual_audio_sample_buffer_mix_many (VisBuffer *dest, int divide, int channels, ...);

VisAudioSample *visual_audio_sample_new (VisBuffer *buffer, VisTime *timestamp,
		VisAudioSampleFormatType format,
		VisAudioSampleRateType rate);
int visual_audio_sample_init (VisAudioSample *sample, VisBuffer *buffer, VisTime *timestamp,
		VisAudioSampleFormatType format,
		VisAudioSampleRateType rate);
int visual_audio_sample_has_internal (VisAudioSample *sample);
int visual_audio_sample_transform_format (VisAudioSample *dest, VisAudioSample *src, VisAudioSampleFormatType format);
int visual_audio_sample_transform_rate (VisAudioSample *dest, VisAudioSample *src, VisAudioSampleRateType rate);
int visual_audio_sample_rate_get_length (VisAudioSampleRateType rate);
int visual_audio_sample_format_get_size (VisAudioSampleFormatType format);
int visual_audio_sample_format_is_signed (VisAudioSampleFormatType format);

VISUAL_END_DECLS

#endif /* _LV_AUDIO_H */
