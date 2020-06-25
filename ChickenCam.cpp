#include "ChickenCam.h"
#include <iostream>

#pragma region Constructor/Destructor
ChickenCam::ChickenCam()
{
    // Hard coding these for now:
    //                        X     Y     W     H
    this->camSlots.push_back({
        480, // X
        0,   // Y
        960, // W
        720, // H
        std::string("rtsp://170.93.143.139/rtplive/470011e600ef003a004ee33696235daa")
    });
    this->camSlots.push_back({
        0,   // X
        0,   // Y
        480, // W
        360, // H
        std::string("rtsp://170.93.143.139/rtplive/470011e600ef003a004ee33696235daa")
    });
    this->camSlots.push_back({
        0,   // X
        360, // Y
        480, // W
        360, // H
        std::string("rtsp://170.93.143.139/rtplive/470011e600ef003a004ee33696235daa")
    });
    this->twitchIngestUri = std::string("rtmp://live-sea.twitch.tv/app/");
    this->twitchStreamKey = std::string("");
    this->initGst();
}
#pragma endregion

#pragma region Public methods
void ChickenCam::Run()
{
    //while (true)
    //{
        // Play!
        std::cout << "CHICKENCAM: PLAY" << std::endl;
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
                    gst_debug_bin_to_dot_file_with_ts(GST_BIN(this->gstPipeline), GST_DEBUG_GRAPH_SHOW_ALL, "error");
                    break;
                case GST_MESSAGE_EOS:
                    std::cout << "CHICKENCAM: End-Of-Stream reached." << std::endl;
                    terminate = TRUE;
                    break;
                case GST_MESSAGE_STATE_CHANGED:
                    if (GST_MESSAGE_SRC(msg) == GST_OBJECT(this->gstPipeline))
                    {
                        GstState old_state;
                        GstState new_state;
                        GstState pending_state;
                        gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
                        std::cout << "CHICKENCAM: Pipeline state changed from " << 
                            gst_element_state_get_name(old_state) << " (" << old_state << ")" <<
                            " to " << 
                            gst_element_state_get_name(new_state) << " (" << new_state << ")" <<
                            std::endl;
                    }
                    break;
                default:
                    std::cout << "CHICKENCAM: Unexpected message received." << std::endl;
                    break;
                }
                gst_message_unref(msg);
            }
        }
        while (!terminate);

        gst_debug_bin_to_dot_file_with_ts(GST_BIN(this->gstPipeline), GST_DEBUG_GRAPH_SHOW_ALL, "terminated");

        std::cout << "CHICKENCAM: Terminated." << std::endl;
    //}
}
#pragma endregion

#pragma region Private static methods
void ChickenCam::onRtspPadAdded(GstElement* element, GstPad* pad, void* data)
{
    std::cout << "CHICKENCAM: Linking rtspsrc..." << std::endl;
    // Finish linking rtspsrc
    GstElement* rtpH264Depay = GST_ELEMENT(data);
    GstPad* sinkpad;

    sinkpad = gst_element_get_static_pad(rtpH264Depay, "sink");
    GstPadLinkReturn linkResult = gst_pad_link(pad, sinkpad);
    if (linkResult != GST_PAD_LINK_OK)
    {
        throw std::runtime_error(std::string("Could not link rtspsrc: ") + std::to_string(linkResult));
    }
    gst_object_unref(sinkpad);
}
#pragma endregion

#pragma region Private methods
void ChickenCam::initGst()
{
    std::cout << "CHICKENCAM: Initializing GStreamer..." << std::endl;
    gst_init(NULL, NULL);

    std::cout << "CHICKENCAM: Creating GST elements..." << std::endl;
    this->gstPipeline     = gst_pipeline_new("pipeline");
    this->gstNvCompositor = gst_element_factory_make("nvcompositor",  "compositor");
    this->gstNvVidConv    = gst_element_factory_make("nvvidconv",     "converter");
    this->gstNvH264Enc    = gst_element_factory_make("nvv4l2h264enc", "encoder");
    this->gstH264Parse    = gst_element_factory_make("h264parse",     "h264parse");
    this->gstFlvMux       = gst_element_factory_make("flvmux",        "flvmux");
    this->gstVideoQueue   = gst_element_factory_make("queue",         "videoqueue");
    this->gstRtmpSink     = gst_element_factory_make("rtmpsink",      "rtmpsink");

    std::cout << "CHICKENCAM: Adding elements to pipeline..." << std::endl;
    gst_bin_add_many (GST_BIN(this->gstPipeline),
        this->gstNvCompositor,
        this->gstNvVidConv,
        this->gstNvH264Enc,
        this->gstH264Parse,
        this->gstFlvMux,
        this->gstVideoQueue,
        this->gstRtmpSink,
        NULL);

    std::cout << "CHICKENCAM: Configuring Compositor element..." << std::endl;
    for (unsigned int i = 0; i < this->camSlots.size(); ++i)
    {
        CamSlot& slot = this->camSlots.at(i);
        std::cout << 
            "CHICKENCAM: Cam Slot " << i << ": (" << 
            slot.X << ", " << slot.Y << ", " << slot.Width << ", " << slot.Height <<
            ")" << std::endl;

        // Create a cam source
        std::shared_ptr<CamSource> camSource = std::make_shared<CamSource>(slot.uri);
        this->camSources.push_back(camSource);

        // Add cam source bin to our pipeline
        GstBin* camSourceBin = camSource->GetBin();
        gst_bin_add(GST_BIN(this->gstPipeline), GST_ELEMENT(camSourceBin));

        // Add a caps filter with the slot size parameters
        GstElement* capsFilter = gst_element_factory_make("capsfilter", NULL);
        gst_bin_add(GST_BIN(this->gstPipeline), capsFilter);
        GstCaps* caps = gst_caps_new_simple("video/x-raw",
            "format", G_TYPE_STRING, "RGBA",
            //"framerate", GST_TYPE_FRACTION, 30, 1,
            "width", G_TYPE_INT, slot.Width,
            "height", G_TYPE_INT, slot.Height,
            NULL);
        g_object_set(capsFilter,
            "caps", caps,
            NULL);
        gst_caps_unref(caps);

        // Link cam source bin to caps filter
        gst_element_link_pads(GST_ELEMENT(camSourceBin), "src", capsFilter, "sink");

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
        gst_object_unref(GST_OBJECT(compositorSinkPadTemplate));

        // Set properties on sink pad
        g_object_set(compositorSinkPad,
            "xpos",   slot.X,
            "ypos",   slot.Y,
            "width",  slot.Width,
            "height", slot.Height,
            NULL);

        // Link the cam source bin to the compositor
        GstPad* capsFilterSourcePad = gst_element_get_static_pad(capsFilter, "src");
        GstPadLinkReturn padLinkResult = gst_pad_link(capsFilterSourcePad, compositorSinkPad);

        // unref everything
        gst_object_unref(GST_OBJECT(compositorSinkPad));
        gst_object_unref(GST_OBJECT(capsFilterSourcePad));

        if (padLinkResult != GST_PAD_LINK_OK)
        {
            throw std::runtime_error(std::string("Could not link src pad to compositor sink pad: ") + std::to_string(padLinkResult));
        }
    }

    // Configure elements
    std::cout << "CHICKENCAM: Configuring Encoder element..." << std::endl;
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

    std::cout << "CHICKENCAM: Configuring RTMP element..." << std::endl;
    std::string rtmpLocation = this->twitchIngestUri + this->twitchStreamKey;
    g_object_set(this->gstRtmpSink,
        "location", rtmpLocation.c_str(),
        NULL);

    // Link up video
    std::cout << "CHICKENCAM: Linking video elements..." << std::endl;
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