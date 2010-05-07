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

#include "zcorder.h"
#include "luna.h"

pthread_mutex_t recording_mutex = PTHREAD_MUTEX_INITIALIZER;

void *record_wrapper(void *ptr) {

	RECORD_REQUEST_t *req = (RECORD_REQUEST_t *)ptr;

	LSError lserror;
	LSErrorInit(&lserror);

	if (pthread_mutex_trylock(&recording_mutex)==0) {

		int ret = record_start(req->opts);

		pthread_mutex_unlock(&recording_mutex);

		char *jsonResponse = 0;

		if (ret==0) {
			asprintf(&jsonResponse, "{\"returnValue\":true, \"lastfilename\":\"%s\"}", req->opts->file);
			LSMessageReply(pub_bus, req->jsonmessage, jsonResponse, &lserror);
			free(jsonResponse);
		}
		else
			LSMessageReply(pub_bus, req->jsonmessage, "{\"returnValue\":false}", &lserror);
		LSMessageUnref(req->jsonmessage);

	} else
		LSMessageReply(pub_bus, req->jsonmessage, "{\"returnValue\":false,\"errorText\":\"Could not acquire mutex lock. Recording in progress.\"}", &lserror);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	free(req->opts);
	free(req);

	int removed = 0;
	clean_dir(DEFAULT_FILE_LOCATION, removed);
	if (removed)
		printf("Removed %d 0-length files from %s", removed, DEFAULT_FILE_LOCATION);

}

bool start_record(LSHandle* lshandle, LSMessage *jsonmessage, void *ctx) {

	LSMessageRef(jsonmessage);

	char *extension;
	extension = "mp3";

	RECORD_REQUEST_t *req = malloc(sizeof(RECORD_REQUEST_t));
	req->opts = malloc(sizeof(PIPELINE_OPTS_t));
	req->jsonmessage = jsonmessage;

	json_t *object = LSMessageGetPayloadJSON(jsonmessage);

	int source_device = 0;
	int stream_rate = 0;
	int lame_bitrate = 0;
	int lame_quality = 0;
	int voice_activation = 0;
	char *filename = 0;

	json_get_int(object, "source_device", &source_device);
	json_get_string(object, "filename", &filename);
	json_get_int(object, "stream_rate", &stream_rate);
	json_get_int(object, "lame_bitrate", &lame_bitrate);
	json_get_int(object, "lame_quality", &lame_quality);
	json_get_int(object, "voice_activation", &voice_activation);

	if (!filename || !strcmp(filename, "0")) {
		char timestamp[16];
		get_timestamp_string(timestamp);
		sprintf(req->opts->file, "%s/zcorder_%s.%s", DEFAULT_FILE_LOCATION, timestamp, extension);
	}
	else {
		sprintf(req->opts->file, "%s/%s.%s", DEFAULT_FILE_LOCATION, filename, extension);
	}

	req->opts->source_device	= source_device ? source_device : 0;
	req->opts->stream_rate	= stream_rate ? stream_rate : 16000;
	req->opts->lame_bitrate	= lame_bitrate ? lame_bitrate : 96;
	req->opts->lame_quality	= lame_quality ? lame_quality : 6;

	req->opts->voice_activation	= voice_activation ? voice_activation : 0;

	//json_free_value(&object);

	pthread_t record_thread;
	pthread_create(&record_thread, NULL, record_wrapper, req);

	return TRUE;

}

bool stop_record(LSHandle* lshandle, LSMessage *jsonmessage, void *ctx) {

	LSError lserror;
	LSErrorInit(&lserror);

	bool ret = stop_recording();

	if (ret){
		LSMessageReply(lshandle, jsonmessage, "{\"returnValue\":true}", &lserror);
	}
	else
		LSMessageReply(lshandle, jsonmessage, "{\"returnValue\":false}", &lserror);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}
	
	return TRUE;

}

bool killa_hax(LSHandle* lshandle, LSMessage *jsonmessage, void *ctx) {

        LSError lserror;
        LSErrorInit(&lserror);

        bool ret = ninja_killa_hax();

        if (ret==0){
                LSMessageReply(lshandle, jsonmessage, "{\"returnValue\":true}", &lserror);
        }
        else
                LSMessageReply(lshandle, jsonmessage, "{\"returnValue\":false}", &lserror);

        if (LSErrorIsSet(&lserror)) {
                LSErrorPrint(&lserror, stderr);
                LSErrorFree(&lserror);
        }
        
        return TRUE;

}


bool get_events(LSHandle* lshandle, LSMessage *jsonmessage, void *ctx) {

	LSError lserror;
	LSErrorInit(&lserror);

	bool subscribed = false;
	LSSubscriptionProcess(lshandle, jsonmessage, &subscribed, &lserror);

	if (!subscribed)
		LSMessageReply(lshandle, jsonmessage, "{\"returnValue\":false,\"errorText\":\"Requires subscription.\"}", &lserror);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

	return TRUE;

}

LSMethod methods[] = {
		{"start_record",	start_record},
		{"stop_record",		stop_record},
		{"killa_hax",		killa_hax},
		{"get_events",		get_events},
		{0,0}
};

void start_service() {

	LSError lserror;
	LSErrorInit(&lserror);

	GMainLoop *loop = g_main_loop_new(NULL, FALSE);

	LSRegisterPalmService(SERVICE_URI, &serviceHandle, &lserror);
	LSPalmServiceRegisterCategory(serviceHandle, "/", methods, NULL, NULL, NULL, &lserror);
	LSGmainAttachPalmService(serviceHandle, loop, &lserror);

	priv_bus = LSPalmServiceGetPrivateConnection(serviceHandle);
	pub_bus = LSPalmServiceGetPublicConnection(serviceHandle);

	g_main_loop_run(loop);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

}

void respond_to_gst_event(int message_type, char *jsonmessage) {

	LSError lserror;
	LSErrorInit(&lserror);

	int len = 0;
	char *jsonResponse = 0;

	if (message_type == 1337) // position query response
		len = asprintf(&jsonResponse, "{\"time\":\"%s\"}", jsonmessage);

	else
		len = asprintf(&jsonResponse, "{\"gst_message_type\":%d,\"message\":\"%s\"}", message_type, jsonmessage);

	LSSubscriptionRespond(serviceHandle, "/get_events", jsonResponse, &lserror);

	if (LSErrorIsSet(&lserror)) {
		LSErrorPrint(&lserror, stderr);
		LSErrorFree(&lserror);
	}

}
