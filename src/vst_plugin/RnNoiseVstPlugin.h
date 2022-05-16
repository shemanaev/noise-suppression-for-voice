#pragma once

#include <memory>
#include <vector>
#include "vst2.x/audioeffectx.h"

class RnNoiseCommonPlugin;

class RnNoiseVstPlugin : public AudioEffectX {
public:

    RnNoiseVstPlugin(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams);

    ~RnNoiseVstPlugin() override;

    VstInt32 startProcess() override;

    VstInt32 stopProcess() override;

    void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames) override;

    bool getEffectName(char *name) override;

    bool getVendorString (char* text) override;

    bool getProductString(char *name) override;

    void getParameterLabel(VstInt32 index, char* label) override;

    void getParameterName(VstInt32 index, char* label) override;

    void getParameterDisplay(VstInt32 index, char* label) override;

    float getParameter(VstInt32 index) override;

    void setParameter(VstInt32 index, float value) override;

    static const VstInt32 numParameters = 2;

private:
    static const char* s_effectName;

    static const char* s_productString;

    static const char* s_vendor;

    enum class Parameters {
        vadThreshold = 0,
        vadRelease = 1
    };    
    
    // Parameter: VAD Threshold
    const char* paramVadThresholdLabel = "";
    const char* paramVadThresholdName = "VAD Threshold";
    float paramVadThreshold{0.f};

    // Parameter: VAD Release
    const char* paramVadReleaseLabel = "ms";
    const char* paramVadReleaseName = "VAD Release";
    short paramVadRelease{0};

    const int channels = 2;
     
    std::vector<std::unique_ptr<RnNoiseCommonPlugin>> m_rnNoisePlugin;
};