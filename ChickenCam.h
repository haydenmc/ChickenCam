#pragma once
#include <gst/gst.h>
#include <vector>

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

    /* Private members */
    std::vector<CamSlot> camSlots;
    // GStreamer
    GstElement* gstPipeline;
    GstElement* gstNvCompositor;
    std::vector<GstElement*> gstCamSrcElements;
};