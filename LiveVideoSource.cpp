#include "LiveVideoSource.h"
#include "Logger.h"

#pragma region Constructor/Destructor
LiveVideoSource::LiveVideoSource(std::string rtspUrl) : 
    rtspUrl(rtspUrl)
{
    Logger::LogInfo(this, "Created, URI: " + rtspUrl);
}
#pragma endregion

#pragma region Public methods
GstBin* LiveVideoSource::GetGstBin()
{
    return this->gstBin;
}

Event<>& LiveVideoSource::GetStartedEvent()
{
    return this->startedEvent;
}

Event<>& LiveVideoSource::GetStoppedEvent()
{
    return this->stoppedEvent;
}

void LiveVideoSource::Init()
{
    Logger::LogInfo(this, "Initializing...");

    // Create a new bin to host our elements
    this->gstBin = GST_BIN(gst_bin_new(NULL));

    // Create the rtspsrc
    this->gstRtspSrc = gst_element_factory_make("rtspsrc", NULL);
    g_object_set(this->gstRtspSrc,
        "location", this->rtspUrl.c_str(),
        NULL);

    // Add elements to bin
    gst_bin_add_many (GST_BIN(this->gstBin),
        this->gstRtspSrc,
        NULL);

    // Listen for pads added for the rtspsrc
    g_signal_connect(this->gstRtspSrc, "pad-added", G_CALLBACK (onRtspPadAddedStatic), this);

    // Set up ghost pad source for our bin (no target until our rtsp pad arrives)
    this->gstBinGhostSrcPad = gst_ghost_pad_new_no_target("src", GST_PAD_SRC);
    if (!gst_element_add_pad(GST_ELEMENT(this->gstBin), this->gstBinGhostSrcPad))
    {
        throw std::runtime_error("Could not create bin src ghost pad");
    }
}
#pragma endregion

#pragma region Private methods
void LiveVideoSource::onRtspPadAdded(GstElement* src, GstPad* pad)
{
    Logger::LogInfo(this, "RTSP pad added!");

    // Set up elements to decode rtsp stream
    GstElement* queue = gst_element_factory_make("queue", NULL);
    GstElement* decodeBin = gst_element_factory_make("decodebin", NULL);
    // GstElement* rtpH264Depay = gst_element_factory_make("rtph264depay", NULL);
    // GstElement* h264Parse = gst_element_factory_make("h264parse", NULL);
    // GstElement* h264Decode = gst_element_factory_make("nvv4l2decoder", NULL);
    // GstElement* h264Decode = gst_element_factory_make("avdec_h264", NULL);
    // GstElement* nvVideoConvert = gst_element_factory_make("nvvidconv", NULL);
    // GstElement* capsFilter = gst_element_factory_make("capsfilter", NULL);
    // GstCaps* caps = gst_caps_new_empty_simple("video/x-raw");
    // GstCaps* caps = gst_caps_new_simple("video/x-raw",
    //     // "format", G_TYPE_STRING, "NV12",
    //     // "framerate", GST_TYPE_FRACTION, 30, 1,
    //     // "width", G_TYPE_INT, 960,
    //     // "height", G_TYPE_INT, 720,
    //     NULL);
    // g_object_set(capsFilter,
    //     "caps", caps,
    //     NULL);
    // gst_caps_unref(caps);
    //GstElement* videoConvert = gst_element_factory_make("videoconvert", NULL);

    gst_bin_add_many (GST_BIN(this->gstBin),
        queue,
        decodeBin,
        // rtpH264Depay,
        // h264Parse,
        // h264Decode,
        // nvVideoConvert,
        // videoConvert,
        // capsFilter,
        NULL);

    // Listen for pads added for the decodebin
    g_signal_connect(decodeBin, "pad-added", G_CALLBACK (onDecodeBinPadAddedStatic), this);

    if (!gst_element_link_many(
        src,
        queue,
        decodeBin,
        // rtpH264Depay,
        // h264Parse,
        // h264Decode,
        // nvVideoConvert,
        // videoConvert,
        // capsFilter,
        NULL))
    {
        throw std::runtime_error("Could not link RTSP decoding elements");
    }

    // Set up ghost src pad
    // GstPad* lastSrcPad = gst_element_get_static_pad(capsFilter, "src");
    // if (!gst_ghost_pad_set_target(GST_GHOST_PAD_CAST(this->gstBinGhostSrcPad), lastSrcPad))
    // {
    //     throw std::runtime_error("Could not update LiveVideoSource ghost source pad to point to "
    //         "new RTSP stream H264 decoder source pad");
    // }

    // Update state of elements
    gst_element_sync_state_with_parent(queue);
    gst_element_sync_state_with_parent(decodeBin);
    // gst_element_sync_state_with_parent(rtpH264Depay);
    // gst_element_sync_state_with_parent(h264Parse);
    // gst_element_sync_state_with_parent(h264Decode);
    // gst_element_sync_state_with_parent(nvVideoConvert);
    // gst_element_sync_state_with_parent(videoConvert);
    // gst_element_sync_state_with_parent(capsFilter);

    Logger::LogInfo(this, "New RTSP elements linked!");
}

void LiveVideoSource::onDecodeBinPadAdded(GstElement* src, GstPad* pad)
{
    Logger::LogInfo(this, "Decodebin pad added!");

    // Set up elements to convert raw video
    GstElement* nvVideoConvert = gst_element_factory_make("nvvidconv", NULL);
    GstElement* capsFilter = gst_element_factory_make("capsfilter", NULL);
    GstCaps* caps = gst_caps_new_empty_simple("video/x-raw");
    g_object_set(capsFilter,
        "caps", caps,
        NULL);
    gst_caps_unref(caps);

    gst_bin_add_many (GST_BIN(this->gstBin),
        nvVideoConvert,
        capsFilter,
        NULL);

    if (!gst_element_link_many(
        src,
        nvVideoConvert,
        capsFilter,
        NULL))
    {
        throw std::runtime_error("Could not link RTSP conversion elements");
    }

    // Set up ghost src pad
    GstPad* lastSrcPad = gst_element_get_static_pad(capsFilter, "src");
    if (!gst_ghost_pad_set_target(GST_GHOST_PAD_CAST(this->gstBinGhostSrcPad), lastSrcPad))
    {
        throw std::runtime_error("Could not update LiveVideoSource ghost source pad");
    }

    // Update state of elements
    gst_element_sync_state_with_parent(nvVideoConvert);
    gst_element_sync_state_with_parent(capsFilter);

    this->startedEvent.Fire();
}
#pragma endregion