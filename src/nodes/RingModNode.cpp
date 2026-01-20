#include "nodes/RingModNode.hpp"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace voicechanger {

RingModNode::RingModNode(const std::map<std::string, std::any>& config) {
    // Initialize from config
    if (auto it = config.find("carrierFrequency"); it != config.end()) {
        carrierFrequency_.store(std::any_cast<float>(it->second));
    }
    if (auto it = config.find("mix"); it != config.end()) {
        mix_.store(std::any_cast<float>(it->second));
    }
    if (auto it = config.find("threshold"); it != config.end()) {
        threshold_.store(std::any_cast<float>(it->second));
    }
}

bool RingModNode::setBusFormat(switchboard::AudioBusFormat& inputBusFormat,
                                switchboard::AudioBusFormat& outputBusFormat) {
    if (!inputBusFormat.isSet()) {
        return false;
    }

    sampleRate_ = inputBusFormat.sampleRate;

    // Calculate phase increment for carrier oscillator
    float freq = carrierFrequency_.load();
    phaseIncrement_ = (2.0 * M_PI * freq) / static_cast<double>(sampleRate_);

    // Match output format to input
    outputBusFormat = inputBusFormat;
    return true;
}

bool RingModNode::process(switchboard::AudioBus& inBus, switchboard::AudioBus& outBus) {
    auto* inBuffer = inBus.getBuffer();
    auto* outBuffer = outBus.getBuffer();

    if (!inBuffer || !outBuffer) {
        return false;
    }

    uint numFrames = inBuffer->getNumberOfFrames();
    uint numChannels = inBuffer->getNumberOfChannels();

    // Update phase increment if frequency changed
    float freq = carrierFrequency_.load();
    phaseIncrement_ = (2.0 * M_PI * freq) / static_cast<double>(sampleRate_);

    float mix = mix_.load();
    float dryMix = 1.0f - mix;
    float threshold = threshold_.load();

    for (uint frame = 0; frame < numFrames; ++frame) {
        // Generate carrier sample (sine wave)
        float carrier = static_cast<float>(std::sin(phase_));

        // Advance phase
        phase_ += phaseIncrement_;
        if (phase_ >= 2.0 * M_PI) {
            phase_ -= 2.0 * M_PI;
        }

        // Process each channel
        for (uint ch = 0; ch < numChannels; ++ch) {
            const float* inData = inBuffer->getReadPointer(ch);
            float* outData = outBuffer->getWritePointer(ch);

            float inSample = inData[frame];

            // Apply threshold gating
            float modulatedSample;
            if (std::abs(inSample) < threshold) {
                // Below threshold - pass through dry or silence
                modulatedSample = 0.0f;
            } else {
                // Ring modulation: multiply input by carrier
                modulatedSample = inSample * carrier;
            }

            // Mix dry and wet
            outData[frame] = (modulatedSample * mix) + (inSample * dryMix);
        }
    }

    return true;
}

switchboard::Result<void> RingModNode::setValue(const std::string& key, const std::any& value) {
    try {
        if (key == "carrierFrequency") {
            float v = std::any_cast<float>(value);
            v = std::clamp(v, 10.0f, 1000.0f);
            carrierFrequency_.store(v);
            return switchboard::makeSuccess();
        }
        if (key == "mix") {
            float v = std::any_cast<float>(value);
            v = std::clamp(v, 0.0f, 1.0f);
            mix_.store(v);
            return switchboard::makeSuccess();
        }
        if (key == "threshold") {
            float v = std::any_cast<float>(value);
            v = std::clamp(v, 0.0f, 1.0f);
            threshold_.store(v);
            return switchboard::makeSuccess();
        }
        return switchboard::makeError<void>("Unknown parameter: " + key);
    } catch (const std::bad_any_cast&) {
        return switchboard::makeError<void>("Invalid value type for parameter: " + key);
    }
}

switchboard::Result<std::any> RingModNode::getValue(const std::string& key) {
    if (key == "carrierFrequency") {
        return switchboard::makeSuccess(std::make_any<float>(carrierFrequency_.load()));
    }
    if (key == "mix") {
        return switchboard::makeSuccess(std::make_any<float>(mix_.load()));
    }
    if (key == "threshold") {
        return switchboard::makeSuccess(std::make_any<float>(threshold_.load()));
    }
    return switchboard::makeError<std::any>("Unknown parameter: " + key);
}

} // namespace voicechanger
