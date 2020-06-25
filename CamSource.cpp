#include "CamSource.h"
#include <stdexcept>

#pragma region Constructor/Destructor
CamSource::CamSource(std::string uri) : 
    uri(uri)
{
    this->init();
}
#pragma endregion

#pragma region Public methods
GstBin* CamSource::GetBin()
{
    return this->gstBin;
}
#pragma endregion

#pragma region Private methods
void CamSource::init()
{
    // Create a new bin to host our elements
    this->gstBin = GST_BIN(gst_bin_new(NULL));

    // Create our fallbackSrc fallback
    GstElement* fallbackSrc = gst_element_factory_make("videotestsrc", NULL);
    g_object_set(fallbackSrc,
        //"pattern", "gamut",
        "is-live", 1,
        NULL);

    // Create our input selector
    GstElement* inputSelector = gst_element_factory_make("input-selector", NULL);

    // Add elements to bin
    gst_bin_add_many (GST_BIN(this->gstBin),
        fallbackSrc,
        inputSelector,
        NULL);

    // Get input pad to hook up our first source to
    GstPadTemplate* inputSelectorSinkPadTemplate = 
        gst_element_class_get_pad_template(
            GST_ELEMENT_GET_CLASS(inputSelector), "sink_%u");
    GstPad* inputSelectorSinkPad = 
        gst_element_request_pad(
            inputSelector,
            inputSelectorSinkPadTemplate,
            NULL,
            NULL);
    gst_object_unref(GST_OBJECT(inputSelectorSinkPadTemplate));

    // Hook up our queue as the first input
    GstPad* srcPad = gst_element_get_static_pad(fallbackSrc, "src");
    if (gst_pad_link(srcPad, inputSelectorSinkPad) != GST_PAD_LINK_OK)
    {
        throw std::runtime_error("Could not link fallback source to input selector");
    }
    gst_object_unref(GST_OBJECT(srcPad));
    gst_object_unref(GST_OBJECT(inputSelectorSinkPad));

    // Set up ghost pad source for our bin
    GstPad* inputSelectorSourcePad = gst_element_get_static_pad(inputSelector, "src");
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
#pragma endregion