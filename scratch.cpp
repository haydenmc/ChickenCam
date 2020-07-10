// Set up the rtsp src - we'll need to wait for it to create a pad before
        // we can link it to the rest of the pipeline
        GstElement* rtspSrc = gst_element_factory_make("rtspsrc", NULL);
        g_object_set(rtspSrc,
            "location", slot.uri.c_str(),
            NULL);

        GstElement* rtpH264Depay = gst_element_factory_make("rtph264depay", NULL);
        GstElement* h264Parse = gst_element_factory_make("h264parse", NULL);
        GstElement* h264Decode = gst_element_factory_make("nvv4l2decoder", NULL);

        g_signal_connect(rtspSrc, "pad-added", G_CALLBACK (onRtspPadAdded), rtpH264Depay);

        gst_bin_add_many (GST_BIN(this->gstPipeline),
            rtspSrc,
            rtpH264Depay,
            h264Parse,
            h264Decode,
            NULL);