#include "RnNoiseVstPlugin.h"

#include <cstdio>
#include <cmath>
#include <sstream>

#include "common/RnNoiseCommonPlugin.h"

const char *RnNoiseVstPlugin::s_effectName = "Noise Suppression";
const char *RnNoiseVstPlugin::s_productString = "Noise Suppression";
const char *RnNoiseVstPlugin::s_vendor = "shemanaev";

RnNoiseVstPlugin::RnNoiseVstPlugin(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams)
    : AudioEffectX(audioMaster, numPrograms, numParams)
{
    setNumInputs(channels);
    setNumOutputs(channels);
    setUniqueID(366057);
    programsAreChunks(true);
    canProcessReplacing(); // supports replacing mode
    
    m_editor = new Editor(this);
    setEditor(m_editor);

    for (int i = 0; i < channels; i++)
        m_rnNoisePlugin.push_back(std::make_unique<RnNoiseCommonPlugin>());
}

RnNoiseVstPlugin::~RnNoiseVstPlugin() = default;

void RnNoiseVstPlugin::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames) {
    for (int i = 0; i < channels; i++)
        m_rnNoisePlugin[i]->process(inputs[i], outputs[i], sampleFrames, paramVadThreshold, paramVadRelease);
}

VstInt32 RnNoiseVstPlugin::startProcess() {
    for (int i = 0; i < channels; i++)
        m_rnNoisePlugin[i]->init();

    return AudioEffectX::startProcess();
}

VstInt32 RnNoiseVstPlugin::stopProcess() {
    for (int i = 0; i < channels; i++)
        m_rnNoisePlugin[i]->deinit();

    return AudioEffectX::stopProcess();
}

bool RnNoiseVstPlugin::getEffectName(char *name) {
    vst_strncpy(name, s_effectName, VstStringConstants::kVstMaxEffectNameLen);
    return true;
}

bool RnNoiseVstPlugin::getVendorString(char *text) {
    vst_strncpy(text, s_vendor, VstStringConstants::kVstMaxVendorStrLen);
    return true;
}

bool RnNoiseVstPlugin::getProductString(char *name) {
    vst_strncpy(name, s_productString, VstStringConstants::kVstMaxProductStrLen);
    return true;
}

extern AudioEffect *createEffectInstance(audioMasterCallback audioMaster) {
    return new RnNoiseVstPlugin(audioMaster, 0, RnNoiseVstPlugin::numParameters);
}

void RnNoiseVstPlugin::getParameterLabel(VstInt32 index, char* label) {
    const auto paramIdx = static_cast<Parameters>(index);
    switch (paramIdx) {
        case Parameters::vadThreshold:
            vst_strncpy(label, paramVadThresholdLabel, VstStringConstants::kVstMaxParamStrLen);
            break;
        case Parameters::vadRelease:
            vst_strncpy(label, paramVadReleaseLabel, VstStringConstants::kVstMaxParamStrLen);
            break;
    }
}

void RnNoiseVstPlugin::getParameterName(VstInt32 index, char* label) {
    const auto paramIdx = static_cast<Parameters>(index);
    switch (paramIdx) {
        case Parameters::vadThreshold:
            vst_strncpy(label, paramVadThresholdName, VstStringConstants::kVstMaxEffectNameLen);
            break;
        case Parameters::vadRelease:
            vst_strncpy(label, paramVadReleaseName, VstStringConstants::kVstMaxEffectNameLen);
            break;
    }
}

void RnNoiseVstPlugin::getParameterDisplay(VstInt32 index, char* label) {
    const auto paramIdx = static_cast<Parameters>(index);
    switch (paramIdx) {
        case Parameters::vadThreshold:
            snprintf(label, VstStringConstants::kVstMaxParamStrLen, "%.2f", paramVadThreshold);
            break;
        case Parameters::vadRelease:
            snprintf(label, VstStringConstants::kVstMaxParamStrLen, "%.3d", paramVadRelease * 10);
            break;
    }
}

float RnNoiseVstPlugin::getParameter(VstInt32 index) {
    const auto paramIdx = static_cast<Parameters>(index);
    switch (paramIdx) {
        case Parameters::vadThreshold: return paramVadThreshold;
        case Parameters::vadRelease: return paramVadRelease / 1000.f * 10;
    }
    return 1;
}

void RnNoiseVstPlugin::setParameter(VstInt32 index, float value) {
    const auto paramIdx = static_cast<Parameters>(index);
    switch (paramIdx)
    {
        case Parameters::vadThreshold:
            paramVadThreshold = value;
            break;
        case Parameters::vadRelease:
            paramVadRelease = (short)(value * 1000 / 10);
            break;
    }
}

VstInt32 RnNoiseVstPlugin::getChunk(void** data, bool isPreset) {
    std::stringstream s;
    s << std::to_string(paramVadThreshold)  << std::endl << std::to_string(paramVadRelease) << std::endl << getCurrentModel();
    m_settings = s.str();
    *data = static_cast<void*>(&m_settings[0]);
    return static_cast<VstInt32>(m_settings.length());
}

VstInt32 RnNoiseVstPlugin::setChunk(void* data, VstInt32 byteSize, bool isPreset) {
    m_settings.assign(static_cast<char*>(data), byteSize);
    std::stringstream s(m_settings);
    std::string vadThreshold, vadRelease, model;

    s >> vadThreshold >> vadRelease >> model;

    paramVadThreshold = std::stof(vadThreshold);
    paramVadRelease = std::stoi(vadRelease);
    setModel(model);

    return 0; // error code: https://www.kvraudio.com/forum/viewtopic.php?p=5639784&sid=08f756e80d6209008b7b5d29042c07fe#p5639784
}

const std::string& RnNoiseVstPlugin::getCurrentModel() {
    return m_rnNoisePlugin[0]->getCurrentModel();
}

void RnNoiseVstPlugin::setModel(std::string name) {
    for (auto& it : m_rnNoisePlugin) {
        it->setModel(name);
    }
}

const std::vector<std::string> RnNoiseVstPlugin::getAvailableModels() {
    return RnNoiseCommonPlugin::getAvailableModels();
}
