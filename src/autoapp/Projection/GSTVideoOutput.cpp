/*
*  This file is part of openauto project.
*  Copyright (C) 2018 f1x.studio (Michal Szwaj)
*
*  openauto is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 3 of the License, or
*  (at your option) any later version.

*  openauto is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with openauto. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_GST

#include <f1x/aasdk/Common/Data.hpp>
#include <f1x/openauto/autoapp/Projection/GSTVideoOutput.hpp>
#include <f1x/openauto/Common/Log.hpp>

namespace f1x
{
namespace openauto
{
namespace autoapp
{
namespace projection
{



namespace VideoComponent
{
    static constexpr uint32_t DECODER = 0;
    static constexpr uint32_t RENDERER = 1;
    static constexpr uint32_t CLOCK = 2;
    static constexpr uint32_t SCHEDULER = 3;
}

GSTVideoOutput::GSTVideoOutput(configuration::IConfiguration::Pointer configuration, QWidget* videoContainer)
    : VideoOutput(std::move(configuration))
    , isActive_(false)
    , portSettingsChanged_(false)
    , videoContainer_(videoContainer)
{
    QQuickView *view = new QQuickView();
    videoWidget_ = QWidget::createWindowContainer(view);

    QGst::Quick::VideoSurface *surface = new QGst::Quick::VideoSurface;
    view->rootContext()->setContextProperty(QLatin1String("videoSurface"), surface);
    view->setSource(QUrl("test.qml"));
    view->show();

    m_videoSink = surface->videoSink();
    GstBus *bus;

    GError *error = NULL;
    const char* vid_launch_str = "appsrc name=mysrc is-live=true block=false max-latency=100 do-timestamp=true stream-type=stream !  "
                                 "queue ! "
                                 
                                 "h264parse ! "
        #ifdef RPI
                                 "omxh264dec ! "
        #else
                                 "avdec_h264 ! "
        #endif
                                 "capsfilter caps=video/x-raw name=mycapsfilter";
    #ifdef RPI
        OPENAUTO_LOG(info) << "[GSTVideoOutput] RPI Build, running with omxh264dec";
    #endif
    
    
    vid_pipeline = gst_parse_launch(vid_launch_str, &error);
    bus = gst_pipeline_get_bus(GST_PIPELINE(vid_pipeline));
    gst_bus_add_watch(bus, (GstBusFunc) GSTVideoOutput::bus_callback, this);
    gst_object_unref(bus);

    
    

    GstElement *sink = QGlib::RefPointer<QGst::Element>(m_videoSink);
    g_object_set (sink, "force-aspect-ratio", true, nullptr);
    g_object_set (sink, "sync", false, nullptr);
    g_object_set (sink, "async", false, nullptr);
    GstElement *capsfilter = gst_bin_get_by_name(GST_BIN(vid_pipeline), "mycapsfilter");
    gst_bin_add(GST_BIN(vid_pipeline), GST_ELEMENT(sink));
    gst_element_link(capsfilter, GST_ELEMENT(sink));

    vid_src = GST_APP_SRC(gst_bin_get_by_name(GST_BIN(vid_pipeline), "mysrc"));
    gst_app_src_set_stream_type(vid_src, GST_APP_STREAM_TYPE_STREAM);

    // videoWidget_->setVideoSink(m_videoSink);
    videoWidget_->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    videoWidget_->showFullScreen();

}
gboolean GSTVideoOutput::bus_callback(GstBus */* unused*/, GstMessage *message, gpointer *ptr) {
    gchar *debug;
    GError *err;
    gchar *name;

    switch (GST_MESSAGE_TYPE(message)) {

    case GST_MESSAGE_ERROR:
        gst_message_parse_error(message, &err, &debug);
        OPENAUTO_LOG(info) << "[GSTVideoOutput] Error "<< err->message;
        g_error_free(err);
        g_free(debug);
        break;

    case GST_MESSAGE_WARNING:
        gst_message_parse_warning(message, &err, &debug);
        OPENAUTO_LOG(info) << "[GSTVideoOutput] Warning "<<err->message<<" | Debug "<< debug;

        name = (gchar *) GST_MESSAGE_SRC_NAME(message);

        OPENAUTO_LOG(info) << "[GSTVideoOutput] Name of src "<< name ? name : "nil";
        g_error_free(err);
        g_free(debug);

        break;

    case GST_MESSAGE_EOS:
        OPENAUTO_LOG(info) << "[GSTVideoOutput] End of stream";
        break;

    case GST_MESSAGE_STATE_CHANGED:
        break;
    default:
        break;
    }

    return TRUE;
}
bool GSTVideoOutput::open()
{
     GstElement *capsfilter = gst_bin_get_by_name(GST_BIN(vid_pipeline), "mycapsfilter");
    GstPad *convert_pad = gst_element_get_static_pad(capsfilter, "sink");
    gst_pad_add_probe (convert_pad,GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM,convert_probe, this, NULL);
    gst_element_set_state(vid_pipeline, GST_STATE_PLAYING);
    return true;
}

GstPadProbeReturn GSTVideoOutput::convert_probe(GstPad *pad, GstPadProbeInfo *info, void *user_data){
    GstEvent *event = GST_PAD_PROBE_INFO_EVENT(info);
    if (GST_PAD_PROBE_INFO_TYPE(info) & GST_PAD_PROBE_TYPE_EVENT_DOWNSTREAM) {
        if (GST_EVENT_TYPE (event) == GST_EVENT_SEGMENT) {
            GstCaps *caps  = gst_pad_get_current_caps(pad);
            if(caps != NULL){
                GstVideoInfo *vinfo = gst_video_info_new ();
                gst_video_info_from_caps (vinfo, caps);
                OPENAUTO_LOG(info) << "[GSTVideoOutput] Video Width: "<<vinfo->width;
                OPENAUTO_LOG(info) << "[GSTVideoOutput] Video Height: "<<vinfo->height;

            }
            return GST_PAD_PROBE_REMOVE;
        }
    }
    return GST_PAD_PROBE_OK;
}
bool GSTVideoOutput::init()
{

    OPENAUTO_LOG(info) << "[GSTVideoOutput] init";
   
    videoWidget_->resize(800,400);

    return true;
}
bool GSTVideoOutput::setupDisplayRegion()
{
    
}

void GSTVideoOutput::write(uint64_t timestamp, const aasdk::common::DataConstBuffer& buffer)
{
    GstBuffer * buffer_ = gst_buffer_new_and_alloc(buffer.size);
    gst_buffer_fill(buffer_, 0, buffer.cdata, buffer.size);
    int ret = gst_app_src_push_buffer((GstAppSrc *) vid_src, buffer_);
    if (ret != GST_FLOW_OK) {
        OPENAUTO_LOG(info)<<"[GSTVideoOutput] push buffer returned "<< ret <<" for "<< buffer.size <<"bytes";
    }
    // OPENAUTO_LOG(info)<<"[GSTVideoOutput] write";

}

void GSTVideoOutput::stop()
{
    OPENAUTO_LOG(info) << "[GSTVideoOutput] stop.";
    gst_element_set_state(vid_pipeline, GST_STATE_PAUSED);
}


bool GSTVideoOutput::createComponents()
{
    
}

bool GSTVideoOutput::initClock()
{

}

bool GSTVideoOutput::setupTunnels()
{

}

bool GSTVideoOutput::enablePortBuffers()
{
   
}

}
}
}
}

#endif
