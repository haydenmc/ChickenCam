#include "CamSource.h"
#include <stdexcept>
#include <iostream>

#pragma region Constructor/Destructor
CamSource::CamSource(std::string uri) : 
    uri(uri)
{
    this->init();
}
#pragma endregion

#pragma region Public methods
GstBin* CamSource::GetBin()
{
    return this->gstBin;
}
#pragma endregion

#pragma region Private methods
void CamSource::init()
{
    // Create a new bin to host our elements
    this->gstBin = GST_BIN(gst_bin_new(NULL));

    // Create our fallbackSrc fallback
    GstElement* fallbackSrc = gst_element_factory_make("videotestsrc", NULL);
    g_object_set(fallbackSrc,
        "pattern", 1,
        "is-live", 1,
        NULL);

    // Create the rtspsrc and decoders for it
    GstElement* rtspSrc = gst_element_factory_make("rtspsrc", NULL);
    g_object_set(rtspSrc,
        "location", this->uri.c_str(),
        NULL);
    this->gstRtpH264Depay = gst_element_factory_make("rtph264depay", NULL);
    GstElement* h264Parse = gst_element_factory_make("h264parse", NULL);
    GstElement* h264Decode = gst_element_factory_make("nvv4l2decoder", NULL);

    // Create our input selector
    this->gstInputSelector = gst_element_factory_make("input-selector", NULL);
    g_object_set(this->gstInputSelector,
        "sync-streams", 0,
        NULL);

    // Add elements to bin
    gst_bin_add_many (GST_BIN(this->gstBin),
        fallbackSrc,
        rtspSrc,
        this->gstRtpH264Depay,
        h264Parse,
        h264Decode,
        this->gstInputSelector,
        NULL);

    // Get input pad to hook up our first source to
    GstPadTemplate* inputSelectorSinkPadTemplate = 
        gst_element_class_get_pad_template(
            GST_ELEMENT_GET_CLASS(this->gstInputSelector), "sink_%u");
    this->gstInputSelectorFallbackSink = 
        gst_element_request_pad(
            this->gstInputSelector,
            inputSelectorSinkPadTemplate,
            NULL,
            NULL);
    
    // Hook up our queue as the first input
    GstPad* srcPad = gst_element_get_static_pad(fallbackSrc, "src");
    if (gst_pad_link(srcPad, this->gstInputSelectorFallbackSink) != GST_PAD_LINK_OK)
    {
        throw std::runtime_error("Could not link fallback source to input selector");
    }
    gst_object_unref(GST_OBJECT(srcPad));

    // Hook up decoders for the rtpsrc
    if (!gst_element_link_many(
        this->gstRtpH264Depay,
        h264Parse,
        h264Decode,
        NULL))
    {
        throw std::runtime_error("Could not link rtp decoder elements");
    }

    // Hook up decoder as the second input
    this->gstInputSelectorRtspSink = 
        gst_element_request_pad(
            this->gstInputSelector,
            inputSelectorSinkPadTemplate,
            NULL,
            NULL);
    GstPad* decoderSrcPad = gst_element_get_static_pad(h264Decode, "src");
    if (gst_pad_link(decoderSrcPad, this->gstInputSelectorRtspSink) != GST_PAD_LINK_OK)
    {
        throw std::runtime_error("Could not link rtsp source to input selector");
    }
    gst_object_unref(GST_OBJECT(decoderSrcPad));
    gst_object_unref(GST_OBJECT(inputSelectorSinkPadTemplate));

    // Listen for pads added for the rtspsrc
    g_signal_connect(rtspSrc, "pad-added", G_CALLBACK (onRtspPadAddedStatic), this);

    // Set up ghost pad source for our bin
    GstPad* inputSelectorSourcePad = gst_element_get_static_pad(this->gstInputSelector, "src");
    GstPad* ghostPad = gst_ghost_pad_new("src", inputSelectorSourcePad);
    if (ghostPad == NULL)
    {
        throw std::runtime_error("Could not create ghost pad");
    }
    if (!gst_element_add_pad(GST_ELEMENT(this->gstBin), ghostPad))
    {
        throw std::runtime_error("Could not create bin src ghost pad");
    }
    gst_object_unref(GST_OBJECT(inputSelectorSourcePad));
}

void CamSource::onRtspPadAdded(GstElement* src, GstPad* pad)
{
    // Link the new pad to the depay element
    GstPad* rtpH264DepaySink = gst_element_get_static_pad(this->gstRtpH264Depay, "sink");
    if (gst_pad_link(pad, rtpH264DepaySink) != GST_PAD_LINK_OK)
    {
        throw std::runtime_error("Could not link rtsp src pad to input selector sink pad");
    }
    gst_object_unref(rtpH264DepaySink);

     g_object_set(G_OBJECT(this->gstInputSelector),
         "active-pad", this->gstInputSelectorRtspSink,
         NULL);

    std::cout << "CAMSOURCE: RTSP pad added" << std::endl;

    gst_debug_bin_to_dot_file_with_ts(GST_BIN(this->gstBin), GST_DEBUG_GRAPH_SHOW_ALL, "rtspadded");
}
#pragma endregion