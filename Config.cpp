#include "Config.h"
#include "Logger.h"
#include <filesystem>

using namespace tinyxml2;

#pragma region Constructor/Destructor
Config::Config()
{
    // Set some sensible defaults
    this->chickenCam.frameRate          = 30;
    this->chickenCam.bitRate            = 2000 * // kbps
                                          1000;  // bps
    this->chickenCam.h264Profile        = 4;     // High
    this->chickenCam.h264PresetLevel    = 3;     // Medium
    this->chickenCam.h264IFrameInterval = 30;    // Every 30 frames
    this->chickenCam.h264InsertSpsPps   = true;
    this->chickenCam.h264MaxPerfEnabled = false; // We'll experiment with this
}
#pragma endregion

#pragma region Public methods
void Config::Load()
{
    this->loadConfigFile("config.xml");
}

ConfigChickenCam Config::GetChickenCam()
{
    return this->chickenCam;
}

std::vector<ConfigVideoSlot> Config::GetVideoSlots()
{
    return this->videoSlots;
}

std::vector<ConfigLiveVideoSource> Config::GetLiveVideoSources()
{
    return this->liveVideoSources;
}
#pragma endregion

#pragma region Private methods
void Config::loadConfigFile(std::string fileName)
{
    Logger::LogInfo(this, "Loading config from file '" + fileName + "'...");
    if (std::filesystem::exists(fileName) == false)
    {
        Logger::LogWarning(this, "File '" + fileName + "' does not exist.");
        return;
    }

    XMLDocument doc;
    doc.LoadFile(fileName.c_str());
    const XMLElement* rootElement = doc.FirstChildElement("config");
    if (rootElement == nullptr)
    {
        Logger::LogError(this, "No root element named 'config' found in '" + fileName + "'!");
        return;
    }

    // Get all the root config attributes
    rootElement->QueryUnsignedAttribute("frame-rate",            &this->chickenCam.frameRate);
    rootElement->QueryUnsignedAttribute("bit-rate",              &this->chickenCam.bitRate);
    rootElement->QueryUnsignedAttribute("h264-profile",          &this->chickenCam.h264Profile);
    rootElement->QueryUnsignedAttribute("h264-preset-level",     &this->chickenCam.h264PresetLevel);
    rootElement->QueryUnsignedAttribute("h264-i-frame-interval", &this->chickenCam.h264IFrameInterval);
    rootElement->QueryBoolAttribute("h264-insert-sps-pps",   &this->chickenCam.h264InsertSpsPps);
    rootElement->QueryBoolAttribute("h264-max-perf-enabled", &this->chickenCam.h264MaxPerfEnabled);

    // Strings are special
    const char* value = rootElement->Attribute("rtmp-target-uri");
    if (value != nullptr)
    {
        this->chickenCam.rtmpTargetUri = std::string(value);
    }

    // Get the video slots
    if (const XMLElement* el = rootElement->FirstChildElement("video-slots"))
    {
        const XMLElement* currentVideoSlotElement = el->FirstChildElement("video-slot");
        while (currentVideoSlotElement != nullptr)
        {
            ConfigVideoSlot slot;
            if (this->tryXmlQueryUnsignedAttributeLogError(currentVideoSlotElement, "id",     &slot.id)    &&
                this->tryXmlQueryUnsignedAttributeLogError(currentVideoSlotElement, "x",      &slot.x)     &&
                this->tryXmlQueryUnsignedAttributeLogError(currentVideoSlotElement, "y",      &slot.y)     &&
                this->tryXmlQueryUnsignedAttributeLogError(currentVideoSlotElement, "width",  &slot.width) &&
                this->tryXmlQueryUnsignedAttributeLogError(currentVideoSlotElement, "height", &slot.height))
            {
                this->videoSlots.push_back(slot);
            }
            else
            {
                Logger::LogError(this, "could not process video-slot element");
            }
            currentVideoSlotElement = currentVideoSlotElement->NextSiblingElement("video-slot");
        }
    }

    // Get the live video sources
    if (const XMLElement* el = rootElement->FirstChildElement("live-video-sources"))
    {
        const XMLElement* currentSourceElement = el->FirstChildElement("live-video-source");
        while (currentSourceElement != nullptr)
        {
            ConfigLiveVideoSource source;
            const char* rtspUri = currentSourceElement->Attribute("rtsp-uri");
            if ((rtspUri != nullptr) && 
                (this->tryXmlQueryUnsignedAttributeLogError(currentSourceElement, "slot-id", &source.slotId)))
            {
                source.rtspUri = std::string(rtspUri);
                this->liveVideoSources.push_back(source);
            }
            else
            {
                Logger::LogError(this, "Could not process live-video-source element");
            }
            currentSourceElement = currentSourceElement->NextSiblingElement("live-video-source");
        }
    }
}

bool Config::tryXmlQueryUnsignedAttributeLogError(
    const XMLElement* element,
    const char* name,
    unsigned int* value)
{
    if (element->QueryUnsignedAttribute(name, value) != XML_SUCCESS)
    {
        std::stringstream logMessage;
        logMessage << "Could not process attribute " << name;
        Logger::LogError(this, logMessage.str());
        return false;
    }
    return true;
}
#pragma endregion