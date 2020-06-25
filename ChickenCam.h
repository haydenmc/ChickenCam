#pragma once
#include "CamSource.h"
#include <gst/gst.h>
#include <vector>
#include <string>
#include <memory>

struct CamSlot
{
    unsigned int X;
    unsigned int Y;
    unsigned int Width;
    unsigned int Height;
    std::string uri;
};

class ChickenCam
{
public:
    /* Constructor/Destructor */
    ChickenCam();

    /* Public methods */
    void Run();

private:
    /* Private static methods */
    static void onRtspPadAdded(GstElement* element, GstPad* pad, void* data);

    /* Private methods */
    void initGst();
    GstElement* createGstElement(char* factory, char* name);

    /* Private members */
    std::vector<CamSlot> camSlots;
    std::vector<std::shared_ptr<CamSource>> camSources;
    std::string twitchIngestUri;
    std::string twitchStreamKey;

    /* GStreamer */
    GstBus* gstBus;
    std::vector<GstElement*> gstCamSrcElements;
    GstElement* gstNvCompositor;
    GstElement* gstNvVidConv;
    GstElement* gstNvH264Enc;
    GstElement* gstH264Parse;
    GstElement* gstFlvMux;
    GstElement* gstVideoQueue;
    GstElement* gstRtmpSink;
    GstElement* gstPipeline;
};