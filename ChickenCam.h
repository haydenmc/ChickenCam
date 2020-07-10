#pragma once
#include "VideoSlot.h"
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
    /* Private static methods */
    //static void onRtspPadAdded(GstElement* element, GstPad* pad, void* data);

    /* Private methods */
    void initGst();
    GstElement* createGstElement(char* factory, char* name);

    /* Private members */
    unsigned int frameRate = 30;
    std::vector<std::shared_ptr<VideoSlot>> videoSlots;
    std::string twitchIngestUri;
    std::string twitchStreamKey;

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