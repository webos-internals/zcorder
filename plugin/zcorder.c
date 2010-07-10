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

#include <stdio.h>
#include <math.h>
#include <gst/gst.h>

#include "zcorder.h"

gdouble rms;
GstElement *pipeline, *vact, *fsink, *psrc, *fsrc, *aenc;
int is_eos = 0;
int stop_abrupt = 0;
int quit_now = 0;

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {

	bool quit_recording_loop = FALSE;

	int len = 0;
	char *jsonmessage = 0;

	int message_type = GST_MESSAGE_TYPE(msg);

	switch (message_type) {

	case GST_MESSAGE_UNKNOWN:
		break;
	case GST_MESSAGE_EOS:
		is_eos = 1;
		quit_recording_loop = TRUE;
		break;
	case GST_MESSAGE_ERROR: {
		gchar *debug;
		GError *error;
		gst_message_parse_error(msg, &error, &debug);
		g_free(debug);
		asprintf(&jsonmessage, "Error: %s", error->message);
		g_error_free(error);
		break;
	}
	case GST_MESSAGE_WARNING: {
		gchar *debug;
		GError *error;
		gst_message_parse_warning(msg, &error, &debug);
		g_free(debug);
		asprintf(&jsonmessage, "Warning: %s", error->message);
		g_error_free(error);
		break;
	}
	case GST_MESSAGE_INFO:
		break;
	case GST_MESSAGE_TAG: {
		int idx = 0;
		GstTagList *taglist;
		gst_message_parse_tag(msg, &taglist);
		asprintf(&jsonmessage, "%s", gst_structure_to_string(taglist));
		gst_tag_list_free(taglist);
		break;
	}
	case GST_MESSAGE_BUFFERING:
		break;
	case GST_MESSAGE_STATE_CHANGED: {
		GstState oldState, newState, pendingState;
		gst_message_parse_state_changed(msg, &oldState, &newState, &pendingState);
		asprintf(&jsonmessage, "Old state: %d, New state: %d, Pending state: %d", oldState, newState, pendingState);
		break;
	}
	case GST_MESSAGE_STATE_DIRTY:
		break;
	case GST_MESSAGE_STEP_DONE:
		break;
	case GST_MESSAGE_CLOCK_PROVIDE: {
		GstClock *clock;
		gboolean *ready;
		gst_message_parse_clock_provide(msg, &clock, ready);
		asprintf(&jsonmessage, "Provide clock: %s, Ready: %d", (clock ? GST_OBJECT_NAME(clock) : "NULL"), ready);
		break;
	}
	case GST_MESSAGE_CLOCK_LOST: {
		GstClock *clock;
		gst_message_parse_clock_lost(msg, &clock);
		asprintf(&jsonmessage, "Lost clock: %s", (clock ? GST_OBJECT_NAME(clock) : "NULL"));
		break;
	}
	case GST_MESSAGE_NEW_CLOCK: {
		GstClock *clock;
		gst_message_parse_new_clock(msg, &clock);
		asprintf(&jsonmessage, "New clock: %s", (clock ? GST_OBJECT_NAME(clock) : "NULL"));
		break;
	}
	case GST_MESSAGE_STRUCTURE_CHANGE:
		break;
	case GST_MESSAGE_STREAM_STATUS:
		break;
	case GST_MESSAGE_APPLICATION:
		if (message_type==16384)
			quit_recording_loop = TRUE;
		break;
	case GST_MESSAGE_ELEMENT:
		break;
	case GST_MESSAGE_SEGMENT_START:
		break;
	case GST_MESSAGE_SEGMENT_DONE:
		break;
	case GST_MESSAGE_DURATION:
		break;
	case GST_MESSAGE_LATENCY:
		break;
	case GST_MESSAGE_ASYNC_START:
		break;
	case GST_MESSAGE_ASYNC_DONE:
		break;

	}

	respond_to_gst_event(message_type, jsonmessage);

	if (quit_recording_loop)
		g_main_loop_quit(recording_loop);

	if (jsonmessage) free(jsonmessage);

	return TRUE;

}

void gst_object_deep_notify(GObject *object, GstObject *orig, GParamSpec *pspec, gchar **excluded_props) {

	GValue value = { 0, };        /* the important thing is that value.type = 0 */
	gchar *str = NULL;
	gchar *name = NULL;

	if (pspec->flags & G_PARAM_READABLE) {
		/* let's not print these out for excluded properties... */
		while (excluded_props != NULL && *excluded_props != NULL) {
			if (strcmp (pspec->name, *excluded_props) == 0)
				return;
			excluded_props++;
		}
		g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (pspec));
		g_object_get_property (G_OBJECT (orig), pspec->name, &value);

		/* FIXME: handle flags */
		if (G_IS_PARAM_SPEC_ENUM (pspec)) {
			GEnumValue *enum_value;

			enum_value =
				g_enum_get_value (G_ENUM_CLASS (g_type_class_ref (pspec->value_type)),
						g_value_get_enum (&value));

			str = g_strdup_printf ("%s (%d)", enum_value->value_nick,
					enum_value->value);
		} else {
			str = g_strdup_value_contents (&value);
		}
		name = gst_object_get_path_string (orig);
		g_print ("%s: %s = %s\n", name, pspec->name, str);
		g_free (name);
		g_free (str);
		g_value_unset (&value);
	} else {
		name = gst_object_get_path_string (orig);
		g_warning ("Parameter %s not readable in %s.", pspec->name, name);
		g_free (name);
	}

}

gboolean message_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  extern gdouble rms;
  // handle only element messages
  if (message->type == GST_MESSAGE_ELEMENT) {
    const GstStructure *s = gst_message_get_structure (message);
    const gchar *name = gst_structure_get_name (s);
    // handle only level element messages
    if (strcmp (name, "level") == 0) {
      gint channels;
      gdouble rms_dB;
      const GValue *list;
      const GValue *value;

      gint i;

      list = gst_structure_get_value (s, "rms");
      channels = gst_value_list_get_size (list);

      for (i = 0; i < channels; ++i) {
        list = gst_structure_get_value (s, "rms");
        value = gst_value_list_get_value (list, i);
        rms_dB = g_value_get_double (value);

        // normalize rms value, results in range from 0.0 to 1.0
        rms = pow (10, rms_dB / 20);
      }
    }
  }
  /* we handled the message we want, and ignored the ones we didn't want.
   * so the core can unref the message for us */
  return TRUE;
}

static gboolean active_quit () {
	if (quit_now == 1) {
		quit_now = 0;
		gst_element_set_state(psrc, GST_STATE_PAUSED);
		gst_bin_remove(GST_BIN(pipeline), psrc);
		gst_bin_add(GST_BIN(pipeline), fsrc);
		gst_element_link(fsrc, aenc);
		gst_element_set_state(psrc, GST_STATE_PLAYING);
		gst_element_send_event(pipeline, gst_event_new_eos());
		g_main_loop_quit(recording_loop);
		return FALSE;
	}
	else {
		return TRUE;
	}
}

static gboolean get_position () {
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 pos;
  int message_type = 1337;
  char *jsonmessage = 0;

  if (gst_element_query_position (pipeline, &fmt, &pos)) {
	  asprintf(&jsonmessage, "%"GST_TIME_FORMAT, GST_TIME_ARGS(pos));
	  respond_to_gst_event(message_type, jsonmessage);
  }

  if (jsonmessage) free(jsonmessage);
  if (is_eos == 1) {
	is_eos = 0;
	return FALSE;
  }
  else {
  return TRUE;
  }
}

int record_start(PIPELINE_OPTS_t *opts) {

	int ret = -1;

	GstCaps *acaps;
	GstBus *bus, *level_bus;
	gint watch_id;

	gst_init(NULL, NULL);

	recording_loop = g_main_loop_new(recording_context, FALSE);

	// Create pipeline
	pipeline = gst_pipeline_new("zcorder");

	// Setup fake source
	fsrc = gst_element_factory_make("fakesrc", "fake-source");

	// Setup pulse source
	psrc = gst_element_factory_make("pulsesrc", "pulse-source");

	if (opts->source_device == 1) {
		g_object_set(G_OBJECT(psrc), "device", "pcm_output.monitor", NULL);
		stop_abrupt = 1;
	}
	else {
		g_object_set(G_OBJECT(psrc), "device", "pcm_input", NULL);
	}

	// Setup audio encoder
	aenc = gst_element_factory_make("lame", "audio-encoder");
	g_object_set(G_OBJECT(aenc), "bitrate", opts->lame_bitrate, NULL);
	g_object_set(G_OBJECT(aenc), "quality", opts->lame_quality, NULL);

	// Setup file sink
	fsink = gst_element_factory_make("filesink", "file-sink");
	g_object_set(G_OBJECT(fsink), "location", opts->file, NULL);

	// Setup voice activation (level checker) and turn on message output
	// vact = gst_element_factory_make("level", "voice-activation");
	// g_object_set (G_OBJECT (vact), "message", TRUE, NULL);

	// Bundle up elements into a bin
	gst_bin_add_many(GST_BIN(pipeline), psrc, aenc, fsink, NULL);

	// Build audio caps
	acaps = gst_caps_new_simple(
			"audio/x-raw-int",
			"width",			G_TYPE_INT,		16,
			"depth",			G_TYPE_INT,		16,
			"endianness",		G_TYPE_INT,		1234,
			"rate",				G_TYPE_INT,		opts->stream_rate,
			"channels",			G_TYPE_INT,		1,
			"signed",			G_TYPE_BOOLEAN,	TRUE,
			NULL
	);

	bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	gst_bus_add_watch(bus, bus_call, recording_loop);

	if (opts->voice_activation == 1) {
		// run sync'ed so it doesn't trip over itself
		g_object_set (G_OBJECT (fsink), "sync", TRUE, NULL);

		// Link elements with gstlevel
		gst_element_link_filtered(psrc, vact, acaps);
		gst_element_link_many(vact, aenc, fsink, NULL);

		// Set watch for level element messages
		level_bus = gst_element_get_bus (vact);
		watch_id = gst_bus_add_watch (level_bus, message_handler, NULL);

		extern gdouble rms;
		extern int is_eos;

		int state;
		state = 1;

		gst_element_set_state(pipeline, GST_STATE_PLAYING);
		while (is_eos == 0) {
			if ((rms >= 0.2) && (state != 1)) {
				gst_element_set_state(pipeline, GST_STATE_PLAYING);
				state = 1;
			}
			else if ((rms <= 0.2) && (state != 0)) {
				gst_element_set_state(pipeline, GST_STATE_PAUSED);
				state = 0;
			}
		}
	}
	else {
		// Link elements without gstlevel
		gst_element_link_filtered(psrc, aenc, acaps);
		gst_element_link(aenc, fsink);
		gst_element_set_state(pipeline, GST_STATE_PLAYING);
		g_timeout_add_seconds (5,(GSourceFunc) active_quit, NULL);
		g_timeout_add_seconds (1, (GSourceFunc) get_position, NULL);
		g_main_loop_run(recording_loop);
	}

	//g_signal_connect(pipeline, "deep_notify", G_CALLBACK(gst_object_default_deep_notify), NULL);

	//gst_object_unref(bus);

	//gst_object_unref(level_bus);

	gst_element_set_state(pipeline, GST_STATE_NULL);

	gst_object_unref(GST_OBJECT (pipeline));

	ret = 0;

	return ret;

}

int ninja_killa_hax() {

	int ret = -1;
	GstElement *pulsesrc, *asink;
	GstElement *pipeline_killa;
	
	// Create pipeline
	pipeline_killa = gst_pipeline_new("killa");

	// Setup pulse source
	pulsesrc = gst_element_factory_make("pulsesrc", "pulse-source");
	g_object_set(G_OBJECT(psrc), "device", "pcm_input", NULL);
	g_object_set(G_OBJECT(psrc), "num-buffers", 1, NULL);

	// Setup alsa sink
	asink = gst_element_factory_make("alsasink", "alsa-sink");

	// Bundle up elements into a bin
	gst_bin_add_many(GST_BIN(pipeline_killa), pulsesrc, asink, NULL);
	
	// Link elements
	gst_element_link(pulsesrc, asink);
	gst_element_set_state(pipeline_killa, GST_STATE_PLAYING);
	sleep (3);
	gst_element_set_state(pipeline_killa, GST_STATE_NULL);

	gst_object_unref(GST_OBJECT (pipeline_killa));

	ret = 0;

	return ret;

}

bool stop_recording() {

	if(stop_abrupt == 1) {
		stop_abrupt = 0;
		is_eos = 1;
		quit_now = 1;
		return TRUE;
	}
	else {
		return gst_element_send_event(pipeline, gst_event_new_eos());
	}
}
