#include "ChickenCam.h"
#include <iostream>

#pragma region Constructor/Destructor
ChickenCam::ChickenCam()
{
    // Hard coding these for now:
    //                        X     Y     W     H
    this->camSlots.push_back({480,  0,    960,  720});
    this->camSlots.push_back({0,    0,    480,  360});
    this->camSlots.push_back({0,    360,  480,  360});
    this->initGst();
}
#pragma endregion

#pragma region Public methods
void ChickenCam::Run()
{

}
#pragma endregion

#pragma region Private methods
void ChickenCam::initGst()
{
    std::cout << "CHICKENCAM: Initializing GStreamer..." << std::endl;
    gst_init(NULL, NULL);

    std::cout << "CHICKENCAM: Creating GST elements..." << std::endl;
    this->gstNvCompositor = gst_element_factory_make("nvcompositor", "comp");

    std::cout << "CHICKENCAM: Configuring Compositor element..." << std::endl;
    for (unsigned int i = 0; i < this->camSlots.size(); ++i)
    {
        CamSlot& slot = this->camSlots.at(i);
        std::cout << 
            "CHICKENCAM: Cam Slot " << i << ": (" << 
            slot.X << ", " << slot.Y << ", " << slot.Width << ", " << slot.Height <<
            ")" << std::endl;

        // Create sink pad on compositor for this cam slot
        GstPadTemplate* compositorSinkPadTemplate = 
            gst_element_class_get_pad_template(
                GST_ELEMENT_GET_CLASS(this->gstNvCompositor), "sink_%u");
        GstPad* compositorSinkPad = 
            gst_element_request_pad(
                this->gstNvCompositor,
                compositorSinkPadTemplate,
                NULL,
                NULL);
        // Set properties on sink pad
        g_object_set(compositorSinkPad,
            "xpos",   slot.X,
            "ypos",   slot.Y,
            "width",  slot.Width,
            "height", slot.Height,
            NULL);
    }
}
#pragma endregion