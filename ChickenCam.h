#pragma once
#include <gst/gst.h>
#include <vector>
#include <string>

struct CamSlot
{
    unsigned int X;
    unsigned int Y;
    unsigned int Width;
    unsigned int Height;
};

class ChickenCam
{
public:
    /* Constructor/Destructor */
    ChickenCam();

    /* Public methods */
    void Run();

private:
    /* Private methods */
    void initGst();
    GstElement* createGstElement(char* factory, char* name);

    /* Private members */
    std::vector<CamSlot> camSlots;
    std::string mixerStreamKey;
    // GStreamer
    GstBus* gstBus;
    std::vector<GstElement*> gstCamSrcElements;
    GstElement* gstNvCompositor;
    GstElement* gstNvVidConv;
    GstElement* gstNvH264Enc;
    GstElement* gstVideoQueue;
    GstElement* gstAudioTestSrc;
    GstElement* gstOpusEnc;
    GstElement* gstFtlSink;
    GstElement* gstPipeline;
};