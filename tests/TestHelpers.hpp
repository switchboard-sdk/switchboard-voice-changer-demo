#pragma once

#include <switchboard/Switchboard.hpp>
#include <switchboard_core/AudioBuffer.hpp>
#include <switchboard_core/AudioBus.hpp>
#include <switchboard_core/AudioBusFormat.hpp>

#include <cmath>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace voicechanger::test {

using namespace switchboard;

/**
 * Helper struct for creating and managing audio buffers in tests.
 * Provides convenient access to audio data for verification.
 */
struct TestAudioBus {
    std::vector<std::vector<float>> channelData;
    std::vector<float*> channelPointers;
    AudioBuffer<float>* buffer;
    AudioBus bus;

    TestAudioBus(uint sampleRate, uint numChannels, uint numFrames)
        : channelData(numChannels, std::vector<float>(numFrames, 0.0f))
        , channelPointers(numChannels)
        , buffer(nullptr) {

        for (uint ch = 0; ch < numChannels; ++ch) {
            channelPointers[ch] = channelData[ch].data();
        }

        // AudioBuffer constructor: (numChannels, numFrames, isInterleaved, sampleRate, data**)
        buffer = new AudioBuffer<float>(
            static_cast<uint>(numChannels),
            static_cast<uint>(numFrames),
            false,  // non-interleaved
            sampleRate,
            channelPointers.data()
        );

        bus = AudioBus(buffer);
        AudioBusFormat format(sampleRate, numChannels, numFrames);
        bus.setFormat(format);
    }

    ~TestAudioBus() {
        delete buffer;
    }

    // Non-copyable
    TestAudioBus(const TestAudioBus&) = delete;
    TestAudioBus& operator=(const TestAudioBus&) = delete;

    float getSample(uint channel, uint frame) const {
        return channelData[channel][frame];
    }

    void setSample(uint channel, uint frame, float value) {
        channelData[channel][frame] = value;
    }

    void fillWithSine(float frequency, float amplitude, uint sampleRate) {
        uint numChannels = static_cast<uint>(channelData.size());
        uint numFrames = static_cast<uint>(channelData[0].size());

        for (uint frame = 0; frame < numFrames; ++frame) {
            float t = static_cast<float>(frame) / static_cast<float>(sampleRate);
            float sample = amplitude * std::sin(2.0f * static_cast<float>(M_PI) * frequency * t);
            for (uint ch = 0; ch < numChannels; ++ch) {
                channelData[ch][frame] = sample;
            }
        }
    }

    void fillWithSine(float frequency, float amplitude, uint sampleRate, uint startFrame) {
        uint numChannels = static_cast<uint>(channelData.size());
        uint numFrames = static_cast<uint>(channelData[0].size());

        for (uint frame = 0; frame < numFrames; ++frame) {
            float t = static_cast<float>(startFrame + frame) / static_cast<float>(sampleRate);
            float sample = amplitude * std::sin(2.0f * static_cast<float>(M_PI) * frequency * t);
            for (uint ch = 0; ch < numChannels; ++ch) {
                channelData[ch][frame] = sample;
            }
        }
    }

    void clear() {
        for (auto& channel : channelData) {
            std::fill(channel.begin(), channel.end(), 0.0f);
        }
    }

    float calculateRMS(uint channel) const {
        float sum = 0.0f;
        for (float sample : channelData[channel]) {
            sum += sample * sample;
        }
        return std::sqrt(sum / static_cast<float>(channelData[channel].size()));
    }

    float calculatePeak(uint channel) const {
        float peak = 0.0f;
        for (float sample : channelData[channel]) {
            peak = std::max(peak, std::abs(sample));
        }
        return peak;
    }

    bool isSilent(float threshold = 0.001f) const {
        for (const auto& channel : channelData) {
            for (float sample : channel) {
                if (std::abs(sample) > threshold) {
                    return false;
                }
            }
        }
        return true;
    }
};

/**
 * RAII wrapper for Switchboard SDK initialization.
 * Create one static instance per test file.
 */
struct SwitchboardInitializer {
    SwitchboardInitializer() {
        Config sdkConfig({{"appID", "voice-changer-test"}, {"appSecret", "test"}});
        Switchboard::initialize(sdkConfig);
    }

    ~SwitchboardInitializer() {
        Switchboard::deinitialize();
    }
};

/**
 * Convert linear gain to decibels.
 */
inline float linearToDb(float linear) {
    if (linear <= 0.0f) {
        return -std::numeric_limits<float>::infinity();
    }
    return 20.0f * std::log10(linear);
}

/**
 * Convert decibels to linear gain.
 */
inline float dbToLinear(float db) {
    return std::pow(10.0f, db / 20.0f);
}

} // namespace voicechanger::test
