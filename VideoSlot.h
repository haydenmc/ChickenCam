#pragma once
#include "LiveVideoSource.h"
#include <gst/gst.h>
#include <memory>

/**
 * A VideoSlot is essentially an input-selector gstreamer element with some
 * metadata around where it should be composited into the final frame.
 * It can be attached to a LiveVideoSource, or it can use a fallback video source
 * when the LiveVideoSource either isn't present or isn't available.
 */
class VideoSlot
{
public:
    /* Constructor/Destructor */
    VideoSlot(
        unsigned int id,
        unsigned int x,
        unsigned int y,
        unsigned int width,
        unsigned int height,
        unsigned int frameRate);

    /* Public methods */
    unsigned int GetId();
    unsigned int GetX();
    unsigned int GetY();
    unsigned int GetWidth();
    unsigned int GetHeight();
    GstBin* GetGstBin();
    void Init();
    void AttachVideoSource(std::shared_ptr<LiveVideoSource> liveVideoSource);
    void DetachVideoSource();

private:
    /* Private methods */
    void initLiveVideoSourceConversionBin();
    void onLiveVideoSourceStarted();
    void onLiveVideoSourceStopped();

    /* Private members */
    // Slot parameters
    unsigned int id;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
    unsigned int frameRate;

    // Connected sources
    std::shared_ptr<LiveVideoSource> liveVideoSource;

    // GStreamer
    GstBin* gstBin;
    GstBin* liveVideoSourceConversionBin;
    GstElement* gstFallbackSrc;
    GstElement* gstInputSelector;
    GstPad* inputSelectorFallbackSinkPad;
    GstPad* inputSelectorLiveVideoSinkPad;
};