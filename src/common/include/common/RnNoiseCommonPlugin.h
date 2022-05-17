#pragma once

#include <mutex>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

struct DenoiseState;

class RnNoiseCommonPlugin {
public:

    void init();

    void deinit();

    void process(const float *in, float *out, int32_t sampleFrames, float vadThreshold, short vadRelease = k_vadGracePeriodSamples);

    void setModel(const std::string name);

    const std::string& getCurrentModel() { return m_model; }

    static const std::vector<std::string>& getAvailableModels();

private:

    void createDenoiseState();

private:
    static const int k_denoiseFrameSize = 480;
    static const int k_denoiseSampleRate = 48000;

    /**
     * The amount of samples that aren't silenced, regardless of rnnoise's VAD result, after one was detected.
     * This fixes cut outs in the middle of words.
     * Each sample is 10ms.
     */
    static const short k_vadGracePeriodSamples = 20;

    std::mutex m_stateLock;
    std::shared_ptr<DenoiseState> m_denoiseState;
    std::string m_model{ "default" };

    short m_remainingGracePeriod = 0;

    std::vector<float> m_inputBuffer;
    std::vector<float> m_outputBuffer;
};



