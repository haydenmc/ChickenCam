#pragma once
#include "VideoSlot.h"
#include "LiveVideoSource.h"
#include "Config.h"
#include <gst/gst.h>
#include <vector>
#include <string>
#include <memory>

class ChickenCam
{
public:
    /* Constructor/Destructor */
    ChickenCam();

    /* Public methods */
    void Init();
    void Run();
    void DumpPipelineDebug(const char* name);

    /* Public static members */
    static ChickenCam* Instance;

private:
    /* Private methods */
    void initVideoSlots();
    void initLiveVideoSources();
    void initGst();
    bool tryHandleError(GstObject* src);

    /* Private members */
    unsigned int frameRate = 30;
    std::vector<std::shared_ptr<VideoSlot>> videoSlots;
    std::vector<std::shared_ptr<LiveVideoSource>> liveVideoSources;
    std::unique_ptr<Config> config;

    /* GStreamer */
    GstBus* gstBus;
    GstElement* gstNvCompositor;
    GstElement* gstNvVidConv;
    GstElement* gstNvH264Enc;
    GstElement* gstH264Parse;
    GstElement* gstFlvMux;
    GstElement* gstVideoQueue;
    GstElement* gstRtmpSink;
    GstElement* gstPipeline;
};