#pragma once
#include <string>
#include <vector>
#include <tinyxml2.h>

struct ConfigChickenCam
{
    unsigned int frameRate;
    unsigned int bitRate;
    unsigned int h264Profile;       // Per nvv4l2h264enc:
                                    // 0: Baseline
                                    // 2: Main
                                    // 4: High
    unsigned int h264PresetLevel;   // Per nvv4l2h264enc:
                                    // 0: Disable
                                    // 1: UltraFast
                                    // 2: Fast
                                    // 3: Medium
                                    // 4: Slow
    unsigned int h264IFrameInterval;
    bool h264InsertSpsPps;
    bool h264MaxPerfEnabled;
    std::string rtmpTargetUri;
};

struct ConfigVideoSlot
{
    unsigned int id;
    unsigned int x;
    unsigned int y;
    unsigned int width;
    unsigned int height;
};

struct ConfigLiveVideoSource
{
    unsigned int slotId;
    std::string rtspUri;
};

class Config
{
public:
    /* Constructor/Destructor */
    Config();

    /* Public methods */
    void Load();
    ConfigChickenCam GetChickenCam();
    std::vector<ConfigVideoSlot> GetVideoSlots();
    std::vector<ConfigLiveVideoSource> GetLiveVideoSources();

private:
    /* Private methods */
    void loadConfigFile(std::string fileName);
    bool tryXmlQueryUnsignedAttributeLogError(
        const tinyxml2::XMLElement* element,
        const char* name,
        unsigned int* value);

    /* Private members */
    ConfigChickenCam chickenCam;
    std::vector<ConfigVideoSlot> videoSlots;
    std::vector<ConfigLiveVideoSource> liveVideoSources;
};