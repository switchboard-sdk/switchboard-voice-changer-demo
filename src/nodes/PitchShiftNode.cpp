#include "nodes/PitchShiftNode.hpp"

#include "signalsmith-stretch.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace voicechanger {

class PitchShiftNode::Impl {
public:
    signalsmith::stretch::SignalsmithStretch<float> stretch;
    std::vector<float*> inputPtrs;
    std::vector<float*> outputPtrs;
    std::vector<std::vector<float>> inputBuffers;
    std::vector<std::vector<float>> outputBuffers;

    uint sampleRate = 44100;
    uint numChannels = 2;
    bool isConfigured = false;

    float lastPitchShift = 0.0f;
    float lastFormantPreserve = 1.0f;
};

PitchShiftNode::PitchShiftNode(const std::map<std::string, std::any>& config)
    : pImpl(std::make_unique<Impl>()) {

    // Initialize from config
    if (auto it = config.find("pitchShift"); it != config.end()) {
        pitchShift_.store(std::any_cast<float>(it->second));
    }
    if (auto it = config.find("formantPreserve"); it != config.end()) {
        formantPreserve_.store(std::any_cast<float>(it->second));
    }
    if (auto it = config.find("mix"); it != config.end()) {
        mix_.store(std::any_cast<float>(it->second));
    }
    if (auto it = config.find("outputGain"); it != config.end()) {
        outputGain_.store(std::any_cast<float>(it->second));
    }
}

PitchShiftNode::~PitchShiftNode() = default;

bool PitchShiftNode::setBusFormat(switchboard::AudioBusFormat& inputBusFormat,
                                   switchboard::AudioBusFormat& outputBusFormat) {
    if (!inputBusFormat.isSet()) {
        return false;
    }

    pImpl->sampleRate = inputBusFormat.sampleRate;
    pImpl->numChannels = inputBusFormat.numberOfChannels;

    // Configure signalsmith-stretch with presetDefault for quality
    pImpl->stretch.presetDefault(
        static_cast<int>(pImpl->numChannels),
        static_cast<float>(pImpl->sampleRate)
    );

    // Allocate intermediate buffers
    pImpl->inputBuffers.resize(pImpl->numChannels);
    pImpl->outputBuffers.resize(pImpl->numChannels);
    pImpl->inputPtrs.resize(pImpl->numChannels);
    pImpl->outputPtrs.resize(pImpl->numChannels);

    // Set initial parameters
    updateStretchParameters();

    pImpl->isConfigured = true;

    // Match output format to input
    outputBusFormat = inputBusFormat;
    return true;
}

void PitchShiftNode::updateStretchParameters() {
    float pitch = pitchShift_.load();
    float formantPreserve = formantPreserve_.load();

    // Always update both pitch and formant together since formant compensation
    // depends on the pitch shift amount
    bool needsUpdate = (pitch != pImpl->lastPitchShift) ||
                       (formantPreserve != pImpl->lastFormantPreserve);

    if (needsUpdate) {
        // Set the pitch shift
        pImpl->stretch.setTransposeSemitones(pitch);

        // Calculate formant compensation factor:
        // - formantPreserve=1.0: fully compensate (keep original formants)
        // - formantPreserve=0.0: no compensation (classic chipmunk/villain effect)
        //
        // To preserve formants when pitch shifting, we need to shift formants
        // in the OPPOSITE direction by the INVERSE ratio.
        // Pitch shift factor = 2^(semitones/12)
        // Formant compensation = 1 / pitchFactor = 2^(-semitones/12)
        //
        // With partial preservation, we interpolate between 1.0 (no shift) and
        // the full compensation factor.

        float pitchFactor = std::pow(2.0f, pitch / 12.0f);
        float fullCompensation = 1.0f / pitchFactor;  // Inverse to counteract pitch shift

        // Interpolate: formantPreserve=0 -> factor=1.0 (no formant shift, chipmunk effect)
        //              formantPreserve=1 -> factor=fullCompensation (natural voice)
        float formantFactor = 1.0f + formantPreserve * (fullCompensation - 1.0f);

        pImpl->stretch.setFormantFactor(formantFactor);

        pImpl->lastPitchShift = pitch;
        pImpl->lastFormantPreserve = formantPreserve;
    }
}

bool PitchShiftNode::process(switchboard::AudioBus& inBus, switchboard::AudioBus& outBus) {
    if (!pImpl->isConfigured) {
        return false;
    }

    auto* inBuffer = inBus.getBuffer();
    auto* outBuffer = outBus.getBuffer();

    if (!inBuffer || !outBuffer) {
        return false;
    }

    uint numFrames = inBuffer->getNumberOfFrames();
    uint numChannels = inBuffer->getNumberOfChannels();

    // Update parameters if changed
    updateStretchParameters();

    // Ensure buffers are sized correctly
    for (uint ch = 0; ch < numChannels; ++ch) {
        pImpl->inputBuffers[ch].resize(numFrames);
        pImpl->outputBuffers[ch].resize(numFrames);
        pImpl->inputPtrs[ch] = pImpl->inputBuffers[ch].data();
        pImpl->outputPtrs[ch] = pImpl->outputBuffers[ch].data();
    }

    // Copy input to intermediate buffers
    for (uint ch = 0; ch < numChannels; ++ch) {
        const float* channelData = inBuffer->getReadPointer(ch);
        std::copy(channelData, channelData + numFrames, pImpl->inputBuffers[ch].begin());
    }

    // Process through signalsmith-stretch
    // The library expects const float** for input and float** for output
    pImpl->stretch.process(
        pImpl->inputPtrs.data(),
        static_cast<int>(numFrames),
        pImpl->outputPtrs.data(),
        static_cast<int>(numFrames)
    );

    // Apply mix and gain, copy to output
    float mix = mix_.load();
    float gain = outputGain_.load();
    float dryMix = 1.0f - mix;

    for (uint ch = 0; ch < numChannels; ++ch) {
        const float* inData = inBuffer->getReadPointer(ch);
        float* outData = outBuffer->getWritePointer(ch);
        float* wetData = pImpl->outputBuffers[ch].data();

        for (uint i = 0; i < numFrames; ++i) {
            float wetSample = wetData[i] * mix;
            float drySample = inData[i] * dryMix;
            outData[i] = (wetSample + drySample) * gain;
        }
    }

    return true;
}

switchboard::Result<void> PitchShiftNode::setValue(const std::string& key, const std::any& value) {
    try {
        if (key == "pitchShift") {
            float v = std::any_cast<float>(value);
            v = std::clamp(v, -24.0f, 24.0f);
            pitchShift_.store(v);
            return switchboard::makeSuccess();
        }
        if (key == "formantPreserve") {
            float v = std::any_cast<float>(value);
            v = std::clamp(v, 0.0f, 1.0f);
            formantPreserve_.store(v);
            return switchboard::makeSuccess();
        }
        if (key == "mix") {
            float v = std::any_cast<float>(value);
            v = std::clamp(v, 0.0f, 1.0f);
            mix_.store(v);
            return switchboard::makeSuccess();
        }
        if (key == "outputGain") {
            float v = std::any_cast<float>(value);
            v = std::clamp(v, 0.0f, 4.0f);
            outputGain_.store(v);
            return switchboard::makeSuccess();
        }
        return switchboard::makeError<void>("Unknown parameter: " + key);
    } catch (const std::bad_any_cast&) {
        return switchboard::makeError<void>("Invalid value type for parameter: " + key);
    }
}

switchboard::Result<std::any> PitchShiftNode::getValue(const std::string& key) {
    if (key == "pitchShift") {
        return switchboard::makeSuccess(std::make_any<float>(pitchShift_.load()));
    }
    if (key == "formantPreserve") {
        return switchboard::makeSuccess(std::make_any<float>(formantPreserve_.load()));
    }
    if (key == "mix") {
        return switchboard::makeSuccess(std::make_any<float>(mix_.load()));
    }
    if (key == "outputGain") {
        return switchboard::makeSuccess(std::make_any<float>(outputGain_.load()));
    }
    return switchboard::makeError<std::any>("Unknown parameter: " + key);
}

} // namespace voicechanger
