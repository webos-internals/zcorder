/*=============================================================================
 Copyright (C) 2009 Ryan Hope <rmh3093@gmail.com>
 Copyright (C) 2010 zsoc <zsoc.webosinternals@gmail.com>

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

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <string.h>
#include <syslog.h>

#include "PDL.h"
#include "SDL.h"
#include "zcorder.h"

pthread_mutex_t recording_mutex = PTHREAD_MUTEX_INITIALIZER;

void *record_wrapper(void *ptr) {

	RECORD_REQUEST_t *req = (RECORD_REQUEST_t *)ptr;
	const char **reply;

	if (pthread_mutex_trylock(&recording_mutex)==0) {

		int ret = record_start(req->opts);

		pthread_mutex_unlock(&recording_mutex);

		if (ret==0) {
			asprintf(&reply[0], "{\"returnValue\":true, \"lastfilename\":\"%s\"}", req->opts->file);
			PDL_CallJS("eventSuccess", reply, 1);
		}
		else {
			asprintf(&reply[0], "{\"returnValue\":false}", NULL);
			PDL_CallJS("eventSuccess", reply, 1);
		}

	} else {
		asprintf(&reply[0],"{\"returnValue\":false,\"errorText\":\"Could not acquire mutex lock. Recording in progress.\"}", NULL);
		PDL_CallJS("eventSuccess", reply, 1);
	  }

	free(req->opts);
	free(req);

	int removed = 0;
	clean_dir(DEFAULT_FILE_LOCATION, removed);
	if (removed)
		printf("Removed %d 0-length files from %s", removed, DEFAULT_FILE_LOCATION);

}

PDL_bool start_record(PDL_JSParameters *params) {

	if (PDL_GetNumJSParams(params) < 1) {
		PDL_JSException(params, "Something is broken, values aren't getting from JS to plugin");
		return PDL_TRUE;
	}

	char *extension;
	extension = "mp3";

	RECORD_REQUEST_t *req = malloc(sizeof(RECORD_REQUEST_t));
	req->opts = malloc(sizeof(PIPELINE_OPTS_t));

	int source_device = PDL_GetJSParamInt(params, 0);
	int stream_rate   = PDL_GetJSParamInt(params, 1);
	int lame_bitrate  = PDL_GetJSParamInt(params, 2);
	int lame_quality  = PDL_GetJSParamInt(params, 3);
	int voice_activation = PDL_GetJSParamInt(params, 4);
	const char * filename = PDL_GetJSParamString(params, 5);

	if (!filename || !strcmp(filename, "0")) {
		char timestamp[16];
		get_timestamp_string(timestamp);
		sprintf(req->opts->file, "%s/zcorder_%s.%s", DEFAULT_FILE_LOCATION, timestamp, extension);
	}
	else {
		sprintf(req->opts->file, "%s/%s.%s", DEFAULT_FILE_LOCATION, filename, extension);
	}

	req->opts->source_device	= source_device ? source_device : 0;
	req->opts->stream_rate		= stream_rate  ? stream_rate : 16000;
	req->opts->lame_bitrate		= lame_bitrate ? lame_bitrate : 96;
	req->opts->lame_quality		= lame_quality ? lame_quality : 6;

	req->opts->voice_activation	= voice_activation ? voice_activation : 0;

	pthread_t record_thread;
	pthread_create(&record_thread, NULL, record_wrapper, req);

	return TRUE;

}

PDL_bool stop_record(PDL_JSParameters *params) {

	bool ret = stop_recording();

	if (ret){
		PDL_JSReply(params, "{\"returnValue\":true}");
	}
	else
		PDL_JSReply(params, "{\"returnValue\":false}");

	return TRUE;

}

void start_service() {

	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		syslog(LOG_INFO, "sdl init ESPLODED!!!! zsoc"); //remove me after debug plz
	}

	PDL_Init(0); // WTF does this do?
	syslog(LOG_INFO, "pdl initialized zsoc"); // remove plz
	 // register the JS callbacks
	PDL_RegisterJSHandler("start_record", start_record);
	PDL_RegisterJSHandler("stop_record", stop_record);
	syslog(LOG_INFO, "handler's registered. zsoc"); //
	// finish registration and start the callback handling thread
	PDL_JSRegistrationComplete();

	syslog(LOG_INFO, "registration complete called. zsoc"); //remove me after debug plz

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(loop);
	syslog(LOG_INFO, "gmain loop broken. zsoc"); //
}

void respond_to_gst_event(int message_type, char *jsonmessage) {

	const char *jsonResponse;

	if (message_type == 1337) // position query response
		asprintf(&jsonResponse, "{\"time\":\"%s\"}", jsonmessage);

	else
		asprintf(&jsonResponse, "{\"gst_message_type\":%d,\"message\":\"%s\"}", message_type, jsonmessage);

	PDL_CallJS("eventSuccess", jsonResponse, 1);
}
