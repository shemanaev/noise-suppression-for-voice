#include "RnNoiseVstPlugin.h"

#include <cstdio>
#include <cmath>

#include "common/RnNoiseCommonPlugin.h"

const char *RnNoiseVstPlugin::s_effectName = "Noise Suppression";
const char *RnNoiseVstPlugin::s_productString = "Noise Suppression";

RnNoiseVstPlugin::RnNoiseVstPlugin(audioMasterCallback audioMaster, VstInt32 numPrograms, VstInt32 numParams)
        : AudioEffectX(audioMaster, numPrograms, numParams) {
    setNumInputs(1); // mono in
    setNumOutputs(1); // mono out
    setUniqueID(366056);
    canProcessReplacing(); // supports replacing mode

    m_rnNoisePlugin = std::make_unique<RnNoiseCommonPlugin>();
}

RnNoiseVstPlugin::~RnNoiseVstPlugin() = default;

void RnNoiseVstPlugin::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames) {
    // Mono in/out only
    float *inChannel0 = inputs[0];
    float *outChannel0 = outputs[0];

    m_rnNoisePlugin->process(inChannel0, outChannel0, sampleFrames, paramVadThreshold, paramVadRelease);
}

VstInt32 RnNoiseVstPlugin::startProcess() {
    m_rnNoisePlugin->init();

    return AudioEffectX::startProcess();
}

VstInt32 RnNoiseVstPlugin::stopProcess() {
    m_rnNoisePlugin->deinit();

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