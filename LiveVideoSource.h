#pragma once
#include "Event.h"
#include <gst/gst.h>
#include <string>

/**
 * A LiveVideoSource is essentially an rtspsrc gstreamer element with some smarts
 * on how to react when the rtspsrc src pad is made available (when the stream starts)
 * and when the stream is lost.
 * 
 * NOTE: Today LiveVideoSource assumes that this is an H264 stream, and is hard-coded
 * to use nvv4l2decoder to decode it.
 */
class LiveVideoSource
{
public:
    /* Constructor/Destructor */
    LiveVideoSource(std::string rtspUrl);

    /* Public methods */
    GstBin* GetGstBin();
    Event<>& GetStartedEvent();
    Event<>& GetStoppedEvent();
    void Init();

private:
    /* Private static methods */
    static void onRtspPadAddedStatic(GstElement* src, GstPad* pad, void* data)
    {
        reinterpret_cast<LiveVideoSource*>(data)->onRtspPadAdded(src, pad);
    }
    static void onDecodeBinPadAddedStatic(GstElement* src, GstPad* pad, void* data)
    {
        reinterpret_cast<LiveVideoSource*>(data)->onDecodeBinPadAdded(src, pad);
    }

    /* Private methods */
    void onRtspPadAdded(GstElement* src, GstPad* pad);
    void onDecodeBinPadAdded(GstElement* src, GstPad* pad);

    /* Private members */
    std::string rtspUrl;
    Event<> startedEvent;
    Event<> stoppedEvent;

    // GStreamer
    GstBin* gstBin;
    GstPad* gstBinGhostSrcPad;
    GstElement* gstRtspSrc;
};