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
    /* Private methods */
    void init();

    /* Private members */
    std::string uri;
    GstBin* gstBin;
};