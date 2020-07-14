#include "VideoSlot.h"
#include "Logger.h"
#include "ChickenCam.h"

#include <string>
#include <sstream>
#include <thread>
#include <chrono>

#pragma region Constructor/Destructor
VideoSlot::VideoSlot(
    unsigned int id,
    unsigned int x,
    unsigned int y,
    unsigned int width,
    unsigned int height,
    unsigned int frameRate) : 
    id(id),
    x(x),
    y(y),
    width(width),
    height(height),
    frameRate(frameRate)
{
    std::stringstream createdLogMessageStream;
    createdLogMessageStream << "Created with bounds " <<
        "(" << x << ", " << y << ", " << width << ", " << height << ") "
        "@ " << frameRate << " FPS";
    Logger::LogInfo(this, createdLogMessageStream.str());
}
#pragma endregion

#pragma region Public methods
unsigned int VideoSlot::GetId()
{
    return this->id;
}

unsigned int VideoSlot::GetX()
{
    return this->x;
}

unsigned int VideoSlot::GetY()
{
    return this->y;
}

unsigned int VideoSlot::GetWidth()
{
    return this->width;
}

unsigned int VideoSlot::GetHeight()
{
    return this->height;
}

GstBin* VideoSlot::GetGstBin()
{
    return this->gstBin;
}

void VideoSlot::Init()
{
    // Create a new bin to host our elements
    this->gstBin = GST_BIN(gst_bin_new(NULL));

    // Create fallback source
    this->gstFallbackSrc = gst_element_factory_make("videotestsrc", NULL);
    g_object_set(this->gstFallbackSrc,
        "pattern", 18,
        "is-live", 1,
        NULL);

    // Create our conversion bin for incoming streams
    this->initLiveVideoSourceConversionBin();

    // Create input selector
    this->gstInputSelector = gst_element_factory_make("input-selector", NULL);
    g_object_set(this->gstInputSelector,
        "sync-streams", 1,
        "sync-mode", 1,
        "cache-buffers", 1,
        NULL);

    // Add elements to bin
    gst_bin_add_many (GST_BIN(this->gstBin),
        this->gstFallbackSrc,
        GST_ELEMENT(this->liveVideoSourceConversionBin),
        this->gstInputSelector,
        NULL);

    // Get input-selector sink pad to hook up our fallback source to
    GstPadTemplate* inputSelectorSinkPadTemplate = 
        gst_element_class_get_pad_template(
            GST_ELEMENT_GET_CLASS(this->gstInputSelector), "sink_%u");
    this->inputSelectorFallbackSinkPad = 
        gst_element_request_pad(
            this->gstInputSelector,
            inputSelectorSinkPadTemplate,
            NULL,
            NULL);
    GstPad* fallbackSrcPad = gst_element_get_static_pad(this->gstFallbackSrc, "src");
    if (gst_pad_link(fallbackSrcPad, this->inputSelectorFallbackSinkPad) != GST_PAD_LINK_OK)
    {
        throw std::runtime_error("Could not link fallback source to input selector");
    }
    gst_object_unref(GST_OBJECT(fallbackSrcPad));

    // Hook up the conversion bin to another input selector sink
    this->inputSelectorLiveVideoSinkPad = 
        gst_element_request_pad(
            this->gstInputSelector,
            inputSelectorSinkPadTemplate,
            NULL,
            NULL);
    GstPad* liveSrcPad = gst_element_get_static_pad(GST_ELEMENT(this->liveVideoSourceConversionBin), "src");
    if (gst_pad_link(liveSrcPad, this->inputSelectorLiveVideoSinkPad) != GST_PAD_LINK_OK)
    {
        throw std::runtime_error("Could not link live video conversion bin to input selector");
    }
    gst_object_unref(GST_OBJECT(liveSrcPad));

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

void VideoSlot::AttachVideoSource(std::shared_ptr<LiveVideoSource> liveVideoSource)
{
    Logger::LogInfo(this, "Attaching live video source...");
    
    this->liveVideoSource = liveVideoSource;

    // Add the live video source bin to our bin
    gst_bin_add_many (GST_BIN(this->gstBin),
        GST_ELEMENT(this->liveVideoSource->GetGstBin()),
        NULL);

    // TODO: listen to live video source events and act accordingly to flip input-switcher
    this->liveVideoSource->GetStartedEvent().Subscribe([this]{
        this->onLiveVideoSourceStarted();
    });
    this->liveVideoSource->GetStoppedEvent().Subscribe([this]{
        this->onLiveVideoSourceStopped();
    });
}

void VideoSlot::DetachVideoSource()
{
    // TODO: flip input-switcher off of the video source
    this->liveVideoSource = nullptr;
}
#pragma endregion

#pragma region Private methods
void VideoSlot::initLiveVideoSourceConversionBin()
{
    this->liveVideoSourceConversionBin = GST_BIN(gst_bin_new(NULL));

    GstElement* videoConvert = gst_element_factory_make("videoconvert", NULL);
    GstElement* videoScale = gst_element_factory_make("videoscale", NULL);
    GstElement* videoRate = gst_element_factory_make("videorate", NULL);
    GstElement* capsFilter = gst_element_factory_make("capsfilter", NULL);
    GstCaps* caps = gst_caps_new_simple("video/x-raw",
        "format", G_TYPE_STRING, "NV12",
        "framerate", GST_TYPE_FRACTION, this->frameRate, 1,
        "width", G_TYPE_INT, this->width,
        "height", G_TYPE_INT, this->height,
        NULL);
    g_object_set(capsFilter,
        "caps", caps,
        NULL);
    gst_caps_unref(caps);

    gst_bin_add_many (GST_BIN(this->liveVideoSourceConversionBin),
        videoConvert,
        videoScale,
        videoRate,
        capsFilter,
        NULL);

    if (!gst_element_link_many(
        videoConvert,
        videoScale,
        videoRate,
        capsFilter,
        NULL))
    {
        throw std::runtime_error("Could not link live video source conversion elements");
    }

    // Set up ghost pads for the conversion bin
    GstPad* firstElementSinkPad = gst_element_get_static_pad(videoConvert, "sink");
    GstPad* ghostSinkPad = gst_ghost_pad_new("sink", firstElementSinkPad);
    if (ghostSinkPad == NULL)
    {
        throw std::runtime_error("Could not create sink ghost pad for conversion bin");
    }
    if (!gst_element_add_pad(GST_ELEMENT(this->liveVideoSourceConversionBin), ghostSinkPad))
    {
        throw std::runtime_error("Could not add conversion bin sink ghost pad");
    }
    gst_object_unref(GST_OBJECT(firstElementSinkPad));
    // gst_object_unref(GST_OBJECT(ghostSinkPad));

    GstPad* lastElementSourcePad = gst_element_get_static_pad(capsFilter, "src");
    GstPad* ghostSrcPad = gst_ghost_pad_new("src", lastElementSourcePad);
    if (ghostSrcPad == NULL)
    {
        throw std::runtime_error("Could not create src ghost pad for conversion bin");
    }
    if (!gst_element_add_pad(GST_ELEMENT(this->liveVideoSourceConversionBin), ghostSrcPad))
    {
        throw std::runtime_error("Could not add conversion bin src ghost pad");
    }
    gst_object_unref(GST_OBJECT(lastElementSourcePad));
    // gst_object_unref(GST_OBJECT(ghostSrcPad));
}

void VideoSlot::onLiveVideoSourceStarted()
{
    Logger::LogInfo(this, "Our live video source has started!");

    if (!gst_element_link_many(
        GST_ELEMENT(this->liveVideoSource->GetGstBin()),
        GST_ELEMENT(this->liveVideoSourceConversionBin),
        NULL))
    {
        throw std::runtime_error("Could not link rtsp source to conversion elements");
    }

    // Set input-selector active pad
    g_object_set(G_OBJECT(this->gstInputSelector),
        "active-pad", this->inputSelectorLiveVideoSinkPad,
        NULL);

    std::stringstream outputName;
    outputName << "rtspadded" << this->id;
    ChickenCam::Instance->DumpPipelineDebug(outputName.str().c_str());

    std::thread t([](){
        std::chrono::milliseconds timespan(10000); // or whatever
        std::this_thread::sleep_for(timespan);
        ChickenCam::Instance->DumpPipelineDebug("delayed");
    });
    t.detach();

    // std::thread t2([this](){
    //     std::chrono::milliseconds timespan(20000); // or whatever
    //     while (true)
    //     {
    //         std::this_thread::sleep_for(timespan);
    //         this->onLiveVideoSourceStopped();
    //         std::this_thread::sleep_for(timespan);
    //         this->onLiveVideoSourceStarted();
    //     }
        
    // });
    // t2.detach();
}

void VideoSlot::onLiveVideoSourceStopped()
{
    // Set input-selector active pad back to fallback
    g_object_set(G_OBJECT(this->gstInputSelector),
        "active-pad", this->inputSelectorFallbackSinkPad,
        NULL);

    // Un-link video source
    gst_element_unlink(
        GST_ELEMENT(this->liveVideoSource->GetGstBin()),
        GST_ELEMENT(this->liveVideoSourceConversionBin));
}
#pragma endregion