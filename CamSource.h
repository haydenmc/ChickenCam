#pragma once
#include <gst/gst.h>
#include <string>

class CamSource
{
public:
    /* Constructor/Destructor */
    CamSource(std::string uri);

    /* Public methods */
    GstBin* GetBin();

private:
    /* Private static methods */
    static void onRtspPadAddedStatic(GstElement* src, GstPad* pad, void* data)
    {
        reinterpret_cast<CamSource*>(data)->onRtspPadAdded(src, pad);
    }

    /* Private methods */
    void init();
    void onRtspPadAdded(GstElement* src, GstPad* pad);

    /* Private members */
    std::string uri;
    GstElement* gstRtpH264Depay;
    GstElement* gstInputSelector;
    GstPad* gstInputSelectorFallbackSink;
    GstPad* gstInputSelectorRtspSink;
    GstBin* gstBin;
};