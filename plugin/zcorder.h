/*=============================================================================
 Copyright (C) 2009 Ryan Hope <rmh3093@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 =============================================================================*/

#ifndef ZCORDER_H_
#define ZCORDER_H_

#define DEFAULT_FILE_LOCATION	"/media/internal/recordings"
#endif /* ZCORDER_H_ */

#ifndef GSTREAMER_H_
#define GSTREAMER_H_

#include <limits.h>
#include <stdbool.h>
#include <pthread.h>
#include <glib.h>

/*!
 * \brief Possible source devices for
 * 'pulsesrc' gst element
 */
typedef enum {
	INPUT_FROM_MIC,		//!< pcm_input
	INPUT_FROM_STREAM,	//!< pcm_output.monitor
} SOURCE_DEVICE_t;



typedef enum {
	VOICE_ACTIVATION_NO = 0,	//!< unlimit gstlevel
	VOICE_ACTIVATION_YES = 1,	//!< limit gstlevel
} VOICE_ACTIVATION_t;


/*!
 * \brief recording options
 */
typedef struct {

	char					file[PATH_MAX];

	SOURCE_DEVICE_t			source_device;

	unsigned int			stream_rate;
	unsigned int			lame_bitrate;
	unsigned int			lame_quality;

	char					filename;
	//VOICE_ACTIVATION_t		voice_activation;
	unsigned int			voice_activation;

} PIPELINE_OPTS_t;

GMainLoop *recording_loop;
GMainContext *recording_context;

int record_video(PIPELINE_OPTS_t *opts);
bool stop_recording();

typedef struct {
	PIPELINE_OPTS_t *opts;
} RECORD_REQUEST_t;

void respond_to_gst_event(int message_type, char *jsonmessage);

#endif /* GSTREAMER_H_ */
