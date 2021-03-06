/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * This file was (probably) generated from gstnavseek.c,
 * gstnavseek.c,v 1.7 2003/11/08 02:48:59 dschleef Exp 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstnavseek.h"
#include <string.h>
#include <math.h>

enum
{
  ARG_0,
  ARG_SEEKOFFSET
};

GstStaticPadTemplate navseek_src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GstStaticPadTemplate navseek_sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static const GstElementDetails navseek_details =
GST_ELEMENT_DETAILS ("Seek based on left-right arrows",
    "Filter/Video",
    "Seek based on navigation keys left-right",
    "Jan Schmidt <thaytan@mad.scientist.com>");

static gboolean gst_navseek_event (GstBaseTransform * trans, GstEvent * event);
static GstFlowReturn gst_navseek_transform_ip (GstBaseTransform * basetrans,
    GstBuffer * buf);
static gboolean gst_navseek_handle_src_event (GstPad * pad, GstEvent * event);
static gboolean gst_navseek_stop (GstBaseTransform * trans);
static gboolean gst_navseek_start (GstBaseTransform * trans);

static void gst_navseek_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_navseek_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

GST_BOILERPLATE (GstNavSeek, gst_navseek, GstBaseTransform,
    GST_TYPE_BASE_TRANSFORM);

static void
gst_navseek_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&navseek_sink_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&navseek_src_template));

  gst_element_class_set_details (element_class, &navseek_details);
}

static void
gst_navseek_class_init (GstNavSeekClass * klass)
{
  GstBaseTransformClass *gstbasetrans_class;
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gstbasetrans_class = GST_BASE_TRANSFORM_CLASS (klass);

  gobject_class->set_property = gst_navseek_set_property;
  gobject_class->get_property = gst_navseek_get_property;

  g_object_class_install_property (gobject_class,
      ARG_SEEKOFFSET, g_param_spec_double ("seek-offset", "Seek Offset",
          "Time in seconds to seek by", 0.0, G_MAXDOUBLE, 5.0,
          G_PARAM_READWRITE));

  gstbasetrans_class->event = GST_DEBUG_FUNCPTR (gst_navseek_event);
  gstbasetrans_class->transform_ip =
      GST_DEBUG_FUNCPTR (gst_navseek_transform_ip);
  gstbasetrans_class->start = GST_DEBUG_FUNCPTR (gst_navseek_start);
  gstbasetrans_class->stop = GST_DEBUG_FUNCPTR (gst_navseek_stop);
}

static void
gst_navseek_init (GstNavSeek * navseek, GstNavSeekClass * g_class)
{
  gst_pad_set_event_function (GST_BASE_TRANSFORM (navseek)->srcpad,
      GST_DEBUG_FUNCPTR (gst_navseek_handle_src_event));

  gst_base_transform_set_passthrough (GST_BASE_TRANSFORM (navseek), TRUE);

  navseek->seek_offset = 5.0;
  navseek->loop = FALSE;
  navseek->grab_seg_start = FALSE;
  navseek->grab_seg_end = FALSE;
  navseek->segment_start = GST_CLOCK_TIME_NONE;
  navseek->segment_end = GST_CLOCK_TIME_NONE;
}

static void
gst_navseek_seek (GstNavSeek * navseek, gint64 offset)
{
  GstFormat peer_format = GST_FORMAT_TIME;
  gboolean ret;
  GstPad *peer_pad;
  gint64 peer_value;

  /* Query for the current time then attempt to set to time + offset */
  peer_pad = gst_pad_get_peer (GST_BASE_TRANSFORM (navseek)->sinkpad);
  ret = gst_pad_query_position (peer_pad, &peer_format, &peer_value);

  if (ret && peer_format == GST_FORMAT_TIME) {
    GstEvent *event;

    peer_value += offset;
    if (peer_value < 0)
      peer_value = 0;

    event = gst_event_new_seek (1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_FLUSH,
        GST_SEEK_TYPE_SET, peer_value, GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE);

    gst_pad_send_event (peer_pad, event);
  }

  gst_object_unref (peer_pad);
}

static void
gst_navseek_segseek (GstNavSeek * navseek)
{
  GstEvent *event;
  GstPad *peer_pad;

  if ((navseek->segment_start == GST_CLOCK_TIME_NONE) ||
      (navseek->segment_end == GST_CLOCK_TIME_NONE) ||
      (!GST_PAD_IS_LINKED (GST_BASE_TRANSFORM (navseek)->sinkpad))) {
    return;
  }

  if (navseek->loop) {
    event =
        gst_event_new_seek (1.0, GST_FORMAT_TIME,
        GST_SEEK_FLAG_ACCURATE | GST_SEEK_FLAG_SEGMENT,
        GST_SEEK_TYPE_SET, navseek->segment_start, GST_SEEK_TYPE_SET,
        navseek->segment_end);
  } else {
    event =
        gst_event_new_seek (1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_ACCURATE,
        GST_SEEK_TYPE_SET, navseek->segment_start, GST_SEEK_TYPE_SET,
        navseek->segment_end);
  }

  peer_pad = gst_pad_get_peer (GST_BASE_TRANSFORM (navseek)->sinkpad);
  gst_pad_send_event (peer_pad, event);
  gst_object_unref (peer_pad);
}

static gboolean
gst_navseek_handle_src_event (GstPad * pad, GstEvent * event)
{
  GstNavSeek *navseek;
  gboolean ret = TRUE;

  navseek = GST_NAVSEEK (GST_PAD_PARENT (pad));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_NAVIGATION:
      /* Check for a keyup and convert left/right to a seek event */
    {
      const GstStructure *structure;
      const gchar *event_type;

      structure = gst_event_get_structure (event);
      g_return_val_if_fail (structure != NULL, FALSE);

      event_type = gst_structure_get_string (structure, "event");
      g_return_val_if_fail (event_type != NULL, FALSE);

      if (strcmp (event_type, "key-press") == 0) {
        const gchar *key;

        key = gst_structure_get_string (structure, "key");
        g_return_val_if_fail (key != NULL, FALSE);

        if (strcmp (key, "Left") == 0) {
          /* Seek backward by 5 secs */
          gst_navseek_seek (navseek, -1.0 * navseek->seek_offset * GST_SECOND);
        } else if (strcmp (key, "Right") == 0) {
          /* Seek forward */
          gst_navseek_seek (navseek, navseek->seek_offset * GST_SECOND);
        } else if (strcmp (key, "s") == 0) {
          /* Grab the next frame as the start frame of a segment */
          navseek->grab_seg_start = TRUE;
        } else if (strcmp (key, "e") == 0) {
          /* Grab the next frame as the end frame of a segment */
          navseek->grab_seg_end = TRUE;
        } else if (strcmp (key, "l") == 0) {
          /* Toggle the loop flag. If we have both start and end segment times send a seek */
          navseek->loop = !navseek->loop;
          gst_navseek_segseek (navseek);
        }
      } else {
        break;
      }
      gst_event_unref (event);
      event = NULL;
    }
      break;
    default:
      break;
  }

  if (event && GST_PAD_IS_LINKED (GST_BASE_TRANSFORM (navseek)->sinkpad)) {
    GstPad *peer_pad = gst_pad_get_peer (GST_BASE_TRANSFORM (navseek)->sinkpad);

    ret = gst_pad_send_event (peer_pad, event);
    gst_object_unref (peer_pad);
  }

  return ret;
}

static void
gst_navseek_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstNavSeek *navseek = GST_NAVSEEK (object);

  switch (prop_id) {
    case ARG_SEEKOFFSET:
      GST_OBJECT_LOCK (navseek);
      navseek->seek_offset = g_value_get_double (value);
      GST_OBJECT_UNLOCK (navseek);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_navseek_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstNavSeek *navseek = GST_NAVSEEK (object);

  switch (prop_id) {
    case ARG_SEEKOFFSET:
      GST_OBJECT_LOCK (navseek);
      g_value_set_double (value, navseek->seek_offset);
      GST_OBJECT_UNLOCK (navseek);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static gboolean
gst_navseek_event (GstBaseTransform * trans, GstEvent * event)
{
  GstNavSeek *navseek = GST_NAVSEEK (trans);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      GST_OBJECT_LOCK (navseek);
      if (navseek->loop)
        gst_navseek_segseek (navseek);
      GST_OBJECT_UNLOCK (navseek);
      break;
    default:
      break;
  }
  return GST_BASE_TRANSFORM_CLASS (parent_class)->event (trans, event);
}

static GstFlowReturn
gst_navseek_transform_ip (GstBaseTransform * basetrans, GstBuffer * buf)
{
  GstNavSeek *navseek = GST_NAVSEEK (basetrans);

  GST_OBJECT_LOCK (navseek);

  if (GST_BUFFER_TIMESTAMP_IS_VALID (buf)) {
    if (navseek->grab_seg_start) {
      navseek->segment_start = GST_BUFFER_TIMESTAMP (buf);
      navseek->segment_end = GST_CLOCK_TIME_NONE;
      navseek->grab_seg_start = FALSE;
    }

    if (navseek->grab_seg_end) {
      navseek->segment_end = GST_BUFFER_TIMESTAMP (buf);
      navseek->grab_seg_end = FALSE;
      gst_navseek_segseek (navseek);
    }
  }

  GST_OBJECT_UNLOCK (navseek);

  return GST_FLOW_OK;
}

static gboolean
gst_navseek_start (GstBaseTransform * trans)
{
  /* anything we should be doing here? */
  return TRUE;
}

static gboolean
gst_navseek_stop (GstBaseTransform * trans)
{
  /* anything we should be doing here? */
  return TRUE;
}
