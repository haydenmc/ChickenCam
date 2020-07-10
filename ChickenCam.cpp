#include "ChickenCam.h"
#include "LiveVideoSource.h"
#include "Logger.h"
#include <iostream>

ChickenCam* ChickenCam::Instance = nullptr;

#pragma region Constructor/Destructor
ChickenCam::ChickenCam()
{
    ChickenCam::Instance = this;
    this->twitchIngestUri = std::string("rtmp://live-sea.twitch.tv/app/");
    this->twitchStreamKey = std::string("");
}
#pragma endregion

#pragma region Public methods
void ChickenCam::Init()
{
    Logger::LogInfo(this, "Initializing...");
    Logger::LogInfo(this, "gst_init");
    gst_init(NULL, NULL);

    // Hard code some video slots
    std::shared_ptr<VideoSlot> videoSlot = 
        std::make_shared<VideoSlot>(
            0,   // ID
            480, // X
            0,   // Y
            960, // W
            720  // H
        );
    this->videoSlots.push_back(videoSlot);
    videoSlot->Init();

    videoSlot = 
        std::make_shared<VideoSlot>(
            1,   // ID
            0,   // X
            0,   // Y
            480, // W
            360  // H
        );
    this->videoSlots.push_back(videoSlot);
    videoSlot->Init();

    videoSlot = 
        std::make_shared<VideoSlot>(
            2,   // ID
            0,   // X
            360, // Y
            480, // W
            360  // H
        );
    this->videoSlots.push_back(videoSlot);
    videoSlot->Init();

    // Hook up a test stream to the first slot
    std::shared_ptr<LiveVideoSource> liveVideoSource = 
        std::make_shared<LiveVideoSource>("rtsp://127.0.0.1:8554/test");
    liveVideoSource->Init();
    this->videoSlots.at(0)->AttachVideoSource(liveVideoSource);

    // Initialize ChickenCam gstreamer pipeline
    this->initGst();
}

void ChickenCam::Run()
{
    Logger::LogInfo(this, "Starting!");
    //while (true)
    //{
        // Play!
        Logger::LogInfo(this, "PLAY");
        GstStateChangeReturn ret = gst_element_set_state (this->gstPipeline, GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) {
            gst_object_unref (this->gstPipeline);
            throw std::runtime_error("Unable to set the pipeline to the playing state.");
        }
        this->gstBus = gst_element_get_bus (this->gstPipeline);

        // Listen to gst message bus
        gboolean terminate = FALSE;
        GstMessage *msg;
        do
        {
            msg = gst_bus_timed_pop_filtered(this->gstBus,
                GST_CLOCK_TIME_NONE,
                static_cast<GstMessageType>(GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS));
            
            if (msg != NULL)
            {
                GError* err;
                gchar* debug_info;

                switch (GST_MESSAGE_TYPE(msg))
                {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error (msg, &err, &debug_info);
                    std::cerr << "CHICKENCAM: Error received from element " <<
                        GST_OBJECT_NAME (msg->src) <<
                        ": " <<
                        err->message <<
                        std::endl;
                    std::cerr << "CHICKENCAM: Debugging information: " << 
                        (debug_info ? debug_info : "none") << 
                        std::endl;
                    g_clear_error (&err);
                    g_free (debug_info);
                    terminate = TRUE;
                    gst_debug_bin_to_dot_file(GST_BIN(this->gstPipeline), GST_DEBUG_GRAPH_SHOW_ALL, "error");
                    break;
                case GST_MESSAGE_EOS:
                    Logger::LogInfo(this, "End-Of-Stream reached.");
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(this->gstPipeline))
                    {
                        GstState old_state;
                        GstState new_state;
                        GstState pending_state;
                        gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                        std::string oldStateStr = std::string(gst_element_state_get_name(old_state));
                        std::string newStateStr = std::string(gst_element_state_get_name(new_state));
                        Logger::LogInfo(this, "Pipeline state changed from " + 
                            oldStateStr + " (" + std::to_string(old_state) + ")" +
                            " to " + 
                            newStateStr + " (" + std::to_string(new_state) + ")");

                        gst_debug_bin_to_dot_file(GST_BIN(this->gstPipeline), GST_DEBUG_GRAPH_SHOW_ALL, gst_element_state_get_name(new_state));
                    }
                    break;
                default:
                    Logger::LogInfo(this, "Unexpected message received.");
                    break;
                }
                gst_message_unref(msg);
            }
        }
        while (!terminate);

        gst_debug_bin_to_dot_file(GST_BIN(this->gstPipeline), GST_DEBUG_GRAPH_SHOW_ALL, "terminated");

        Logger::LogInfo(this, "Terminated.");
    //}
}

void ChickenCam::DumpPipelineDebug(const char* name)
{
    gst_debug_bin_to_dot_file(GST_BIN(this->gstPipeline), GST_DEBUG_GRAPH_SHOW_ALL, name);
}
#pragma endregion

#pragma region Private static methods
// void ChickenCam::onRtspPadAdded(GstElement* element, GstPad* pad, void* data)
// {
//     std::cout << "CHICKENCAM: Linking rtspsrc..." << std::endl;
//     // Finish linking rtspsrc
//     GstElement* rtpH264Depay = GST_ELEMENT(data);
//     GstPad* sinkpad;

//     sinkpad = gst_element_get_static_pad(rtpH264Depay, "sink");
//     GstPadLinkReturn linkResult = gst_pad_link(pad, sinkpad);
//     if (linkResult != GST_PAD_LINK_OK)
//     {
//         throw std::runtime_error(std::string("Could not link rtspsrc: ") + std::to_string(linkResult));
//     }
//     gst_object_unref(sinkpad);
// }
#pragma endregion

#pragma region Private methods
void ChickenCam::initGst()
{
    Logger::LogInfo(this, "Initializing GStreamer pipeline...");
    Logger::LogInfo(this, "Creating GST elements...");
    this->gstPipeline     = gst_pipeline_new("pipeline");
    this->gstNvCompositor = gst_element_factory_make("nvcompositor",  "compositor");
    this->gstNvVidConv    = gst_element_factory_make("nvvidconv",     "converter");
    this->gstNvH264Enc    = gst_element_factory_make("nvv4l2h264enc", "encoder");
    this->gstH264Parse    = gst_element_factory_make("h264parse",     "h264parse");
    this->gstFlvMux       = gst_element_factory_make("flvmux",        "flvmux");
    this->gstVideoQueue   = gst_element_factory_make("queue",         "videoqueue");
    this->gstRtmpSink     = gst_element_factory_make("rtmpsink",      "rtmpsink");

    Logger::LogInfo(this, "Adding elements to pipeline...");
    gst_bin_add_many (GST_BIN(this->gstPipeline),
        this->gstNvCompositor,
        this->gstNvVidConv,
        this->gstNvH264Enc,
        this->gstH264Parse,
        this->gstFlvMux,
        this->gstVideoQueue,
        this->gstRtmpSink,
        NULL);

    Logger::LogInfo(this, "Configuring Compositor element...");
    for (unsigned int i = 0; i < this->videoSlots.size(); ++i)
    {
        std::shared_ptr<VideoSlot>& slot = this->videoSlots.at(i);

        Logger::LogInfo(this, "Linking video slot ID " + std::to_string(slot->GetId()) + ": (" + 
            std::to_string(slot->GetX()) + ", " + std::to_string(slot->GetY()) + ", " + 
            std::to_string(slot->GetWidth()) + ", " + std::to_string(slot->GetHeight()) + ")");

        // Add video slot bin to our pipeline
        GstBin* videoSlotBin = slot->GetGstBin();
        gst_bin_add(GST_BIN(this->gstPipeline), GST_ELEMENT(videoSlotBin));

        // Add a caps filter with the slot size parameters
        GstElement* capsFilter = gst_element_factory_make("capsfilter", NULL);
        gst_bin_add(GST_BIN(this->gstPipeline), capsFilter);
        GstCaps* caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "NV12",
            "framerate", GST_TYPE_FRACTION, this->frameRate, 1,
            "width", G_TYPE_INT, slot->GetWidth(),
            "height", G_TYPE_INT, slot->GetHeight(),
            NULL);
        g_object_set(capsFilter,
            "caps", caps,
            NULL);
        gst_caps_unref(caps);

        // Link cam source bin to caps filter
        gst_element_link_pads(GST_ELEMENT(videoSlotBin), "src", capsFilter, "sink");

        // Add an nv video converter
        GstElement* nvVidConv = gst_element_factory_make("nvvidconv", NULL);
        gst_bin_add(GST_BIN(this->gstPipeline), nvVidConv);
        gst_element_link_pads(capsFilter, "src", nvVidConv, "sink");

        // Create sink pad on compositor for this cam slot
        GstPadTemplate* compositorSinkPadTemplate = 
            gst_element_class_get_pad_template(
                GST_ELEMENT_GET_CLASS(this->gstNvCompositor), "sink_%u");
        GstPad* compositorSinkPad = 
            gst_element_request_pad(
                this->gstNvCompositor,
                compositorSinkPadTemplate,
                NULL,
                NULL);

        // Set properties on sink pad
        g_object_set(compositorSinkPad,
            "xpos",   slot->GetX(),
            "ypos",   slot->GetY(),
            "width",  slot->GetWidth(),
            "height", slot->GetHeight(),
            NULL);

        // Link the cam source bin to the compositor
        GstPad* nvVidConvSourcePad = gst_element_get_static_pad(nvVidConv, "src");
        GstPadLinkReturn padLinkResult = gst_pad_link(nvVidConvSourcePad, compositorSinkPad);

        // unref everything
        gst_object_unref(GST_OBJECT(compositorSinkPad));
        gst_object_unref(GST_OBJECT(nvVidConvSourcePad));

        if (padLinkResult != GST_PAD_LINK_OK)
        {
            throw std::runtime_error(std::string("Could not link src pad to compositor sink pad: ") + std::to_string(padLinkResult));
        }
    }

    // Configure elements
    Logger::LogInfo(this, "Configuring Encoder element...");
    g_object_set(this->gstNvH264Enc,
        "maxperf-enable", 1,
        "preset-level", 1,
        "num-B-Frames", 0,
        "control-rate", 1,
        "profile", 2,
        "iframeinterval", 30,
        "insert-sps-pps", 1,
        "bitrate", 2000000,
        NULL);

    Logger::LogInfo(this, "Configuring RTMP element...");
    std::string rtmpLocation = this->twitchIngestUri + this->twitchStreamKey;
    g_object_set(this->gstRtmpSink,
        "location", rtmpLocation.c_str(),
        NULL);

    // Link up video
    Logger::LogInfo(this, "Linking video elements...");
    bool linkResult = gst_element_link_many(
        this->gstNvCompositor,
        this->gstNvVidConv,
        this->gstNvH264Enc,
        this->gstH264Parse,
        this->gstFlvMux,
        this->gstVideoQueue,
        this->gstRtmpSink,
        NULL);

    if (!linkResult)
    {
        gst_object_unref (this->gstPipeline);
        throw std::runtime_error("Could not link gstreamer elements!");
    }

    gst_debug_bin_to_dot_file(GST_BIN(this->gstPipeline), GST_DEBUG_GRAPH_SHOW_ALL, "debug");
}

GstElement* ChickenCam::createGstElement(char* factory, char* name)
{
    GstElement* element = gst_element_factory_make(factory, name);
    if (!element)
    {
        throw std::runtime_error("Could not create GStreamer element " + std::string(name));
    }
}
#pragma endregion