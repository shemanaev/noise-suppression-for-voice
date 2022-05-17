#include "common/RnNoiseCommonPlugin.h"

#include <cstring>
#include <ios>
#include <limits>
#include <algorithm>
#include <cassert>


#include <rnnoise.h>
#include <rnnoise-nu.h>

static const std::unordered_map<std::string, std::string> g_modelsMap = {
    {"default", "orig"},
    {"beguiling-drafter-2018-08-30", "bd"},
    {"conjoined-burgers-2018-08-28", "cb"},
    {"leavened-quisling-2018-08-31", "lq"},
    {"marathon-prescription-2018-08-29", "mp"},
    {"somnolent-hogwash-2018-09-01", "sh"}
};

static const std::vector<std::string> g_models = {
    "default",
    "beguiling-drafter-2018-08-30",
    "conjoined-burgers-2018-08-28",
    "leavened-quisling-2018-08-31",
    "marathon-prescription-2018-08-29", 
    "somnolent-hogwash-2018-09-01", 
};

const std::vector<std::string>& RnNoiseCommonPlugin::getAvailableModels() { return g_models; }

void RnNoiseCommonPlugin::init() {
    deinit();
    createDenoiseState();
}

void RnNoiseCommonPlugin::deinit() {
    std::lock_guard<std::mutex> guard(m_stateLock);
    m_denoiseState.reset();
}

void RnNoiseCommonPlugin::setModel(const std::string name) {
    if (name != m_model) {
        std::lock_guard<std::mutex> guard(m_stateLock);
        m_model = name;
        m_denoiseState.reset();
    }
}

void RnNoiseCommonPlugin::process(const float *in, float *out, int32_t sampleFrames, float vadThreshold, short vadRelease) {
    assert(vadThreshold >= 0.f && vadThreshold <= 1.f);

    if (sampleFrames == 0) {
        return;
    }

    if (!m_denoiseState) {
        createDenoiseState();
    }

    std::lock_guard<std::mutex> guard(m_stateLock);

    // Good case, we can copy less data around and rnnoise lib is built for it
    if (sampleFrames == k_denoiseFrameSize) {
        m_inputBuffer.resize(sampleFrames);

        for (size_t i = 0; i < sampleFrames; i++) {
            m_inputBuffer[i] = in[i] * std::numeric_limits<short>::max();
        }

        float vadProbability = rnnoise_process_frame(m_denoiseState.get(), out, &m_inputBuffer[0]);

        if (vadProbability >= vadThreshold) {
            m_remainingGracePeriod = vadRelease;
        }

        if (m_remainingGracePeriod > 0) {
            m_remainingGracePeriod--;
            for (size_t i = 0; i < sampleFrames; i++) {
                out[i] /= std::numeric_limits<short>::max();
            }
        } else {
            for (size_t i = 0; i < sampleFrames; i++) {
                out[i] = 0.f;
            }
        }

    } else {
        m_inputBuffer.resize(m_inputBuffer.size() + sampleFrames);

        // From [-1.f,1.f] range to [min short, max short] range which rnnoise lib will understand
        {
            float *inputBufferWriteStart = &(*(m_inputBuffer.end() - sampleFrames));
            for (size_t i = 0; i < sampleFrames; i++) {
                inputBufferWriteStart[i] = in[i] * std::numeric_limits<short>::max();
            }
        }

        const size_t framesToProcess = m_inputBuffer.size() / k_denoiseFrameSize;
        const size_t samplesToProcess = framesToProcess * k_denoiseFrameSize;

        m_outputBuffer.resize(m_outputBuffer.size() + samplesToProcess);

        // Process input buffer by chunks of k_denoiseFrameSize, put result into out buffer to return into range [-1.f,1.f]
        {
            float *outBufferWriteStart = &(*(m_outputBuffer.end() - samplesToProcess));

            for (size_t i = 0; i < framesToProcess; i++) {
                float *currentOutBuffer = &outBufferWriteStart[i * k_denoiseFrameSize];
                float *currentInBuffer = &m_inputBuffer[i * k_denoiseFrameSize];
                float vadProbability = rnnoise_process_frame(m_denoiseState.get(), currentOutBuffer, currentInBuffer);

                if (vadProbability >= vadThreshold) {
                    m_remainingGracePeriod = vadRelease;
                }

                if (m_remainingGracePeriod > 0) {
                    m_remainingGracePeriod--;
                    for (size_t j = 0; j < k_denoiseFrameSize; j++) {
                        currentOutBuffer[j] /= std::numeric_limits<short>::max();
                    }
                } else {
                    for (size_t j = 0; j < k_denoiseFrameSize; j++) {
                        currentOutBuffer[j] = 0.f;
                    }
                }
            }
        }

        const size_t toCopyIntoOutput = std::min(m_outputBuffer.size(), static_cast<size_t>(sampleFrames));

        if (toCopyIntoOutput > 0) {
            std::memcpy(out, &m_outputBuffer[0], toCopyIntoOutput * sizeof(float));

            m_inputBuffer.erase(m_inputBuffer.begin(), m_inputBuffer.begin() + samplesToProcess);
            m_outputBuffer.erase(m_outputBuffer.begin(), m_outputBuffer.begin() + toCopyIntoOutput);
        }

        if (toCopyIntoOutput < sampleFrames) {
            std::fill(out + toCopyIntoOutput, out + sampleFrames, 0.f);
        }
    }
}

void RnNoiseCommonPlugin::createDenoiseState() {
    std::lock_guard<std::mutex> guard(m_stateLock);
    RNNModel* model = nullptr;
    auto it = g_modelsMap.find(m_model);
    if (it != g_modelsMap.end()) {
        model = rnnoise_get_model(it->second.c_str());
    }
    m_denoiseState = std::shared_ptr<DenoiseState>(rnnoise_create(model), [](DenoiseState *st) {
        rnnoise_destroy(st);
    });
}