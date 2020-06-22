#include "ChickenCam.h"
#include <iostream>

#pragma region Constructor/Destructor
ChickenCam::ChickenCam()
{
    // Hard coding these for now:
    //                        X     Y     W     H
    this->camSlots.push_back({480,  0,    960,  720});
    this->camSlots.push_back({0,    0,    480,  360});
    this->camSlots.push_back({0,    360,  480,  360});
    this->mixerStreamKey = std::string("1081292-YgHqpurV88ogoN0Rasrj6jvlfRGwszhJ");
    this->initGst();
}
#pragma endregion

#pragma region Public methods
void ChickenCam::Run()
{
    while (true)
    {
        // Play!
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
                            gst_element_state_get_name(old_state) << 
                            " to " << 
                            gst_element_state_get_name(new_state) << 
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

        std::cout << "CHICKENCAM: Terminated." << std::endl;
    }
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
    this->gstVideoQueue   = gst_element_factory_make("queue",         "videoqueue");
    this->gstAudioTestSrc = gst_element_factory_make("audiotestsrc",  "audiotestsrc");
    this->gstOpusEnc      = gst_element_factory_make("opusenc",       "opusenc");
    this->gstFtlSink      = gst_element_factory_make("ftlsink",       "ftl");

    std::cout << "CHICKENCAM: Adding elements to pipeline..." << std::endl;
    gst_bin_add_many (GST_BIN(this->gstPipeline),
        this->gstNvCompositor,
        this->gstNvVidConv,
        this->gstNvH264Enc,
        this->gstVideoQueue,
        this->gstAudioTestSrc,
        this->gstOpusEnc,
        this->gstFtlSink,
        NULL);

    std::cout << "CHICKENCAM: Configuring Compositor element..." << std::endl;
    for (unsigned int i = 0; i < this->camSlots.size(); ++i)
    {
        CamSlot& slot = this->camSlots.at(i);
        std::cout << 
            "CHICKENCAM: Cam Slot " << i << ": (" << 
            slot.X << ", " << slot.Y << ", " << slot.Width << ", " << slot.Height <<
            ")" << std::endl;

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
            "xpos",   slot.X,
            "ypos",   slot.Y,
            "width",  slot.Width,
            "height", slot.Height,
            NULL);

        // For now, create a test source and hook it up to this sink pad
        GstElement* slotElement = gst_element_factory_make("videotestsrc", NULL);
        GstElement* slotCapsFilter = gst_element_factory_make("capsfilter", NULL);
        GstElement* slotNvVidConv = gst_element_factory_make("nvvidconv", NULL);

        // Configure
        g_object_set(slotElement,
            "is-live", 1,
            NULL);

        GstCaps *caps = gst_caps_new_simple ("video/x-raw",
            "format", G_TYPE_STRING, "I420",
            "framerate", GST_TYPE_FRACTION, 30, 1,
            "width", G_TYPE_INT, slot.Width,
            "height", G_TYPE_INT, slot.Height,
            NULL);
        g_object_set(slotCapsFilter,
            "caps", caps,
            NULL);
        gst_caps_unref(caps);

        gst_bin_add(GST_BIN(this->gstPipeline), slotElement);
        gst_bin_add(GST_BIN(this->gstPipeline), slotCapsFilter);
        gst_bin_add(GST_BIN(this->gstPipeline), slotNvVidConv);
        if (!gst_element_link_many(
            slotElement,
            slotCapsFilter,
            slotNvVidConv,
            NULL))
        {
            throw std::runtime_error("Could not link cam source.");
        }
        GstPad* srcPad = gst_element_get_static_pad(slotNvVidConv, "src");
        GstPadLinkReturn padLinkResult = gst_pad_link(srcPad, compositorSinkPad);
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

    std::cout << "CHICKENCAM: Configuring FTL element..." << std::endl;
    g_object_set(this->gstFtlSink,
        "stream-key", this->mixerStreamKey.c_str(),
        NULL);

    std::cout << "CHICKENCAM: Configuring audio test src element..." << std::endl;
    g_object_set(this->gstAudioTestSrc,
        "wave", 4, // silence
        NULL);

    // Link up video
    std::cout << "GST: Linking video elements..." << std::endl;
    bool linkResult = gst_element_link_many(
        this->gstNvCompositor,
        this->gstNvVidConv,
        this->gstNvH264Enc,
        this->gstVideoQueue,
        NULL);
    linkResult &= gst_element_link_pads(this->gstVideoQueue, "src", this->gstFtlSink, "videosink");

    // Link up audio
    linkResult &= gst_element_link_many(
        this->gstAudioTestSrc,
        this->gstOpusEnc,
        NULL);
    linkResult &= gst_element_link_pads(this->gstOpusEnc, "src", this->gstFtlSink, "audiosink");

    if (!linkResult)
    {
        gst_object_unref (this->gstPipeline);
        throw std::runtime_error("Could not link gstreamer elements!");
    }
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