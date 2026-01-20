#pragma once

#include <switchboard_core/SingleBusAudioProcessorNode.hpp>
#include <switchboard_core/NodeTypeInfo.hpp>

#include <any>
#include <atomic>
#include <map>
#include <string>

namespace voicechanger {

/**
 * RingModNode - Ring modulation effect for robotic/alien voice effects.
 *
 * Multiplies input signal by a carrier sine wave, producing sum and
 * difference frequencies (sidebands) while suppressing the original.
 *
 * Parameters:
 * - carrierFrequency: Carrier oscillator frequency in Hz (10 to 1000)
 * - mix: Dry/wet mix (0.0 = dry, 1.0 = wet)
 * - threshold: Input level below which modulation is bypassed (0.0 to 1.0)
 */
class RingModNode : public switchboard::SingleBusAudioProcessorNode {
public:
    static switchboard::NodeTypeInfo getNodeTypeInfo() {
        return switchboard::NodeTypeInfo{
            "VoiceChanger",
            "RingMod",
            "RingMod",
            "Ring modulation for robotic/alien voice effects",
            {switchboard::NODE_CATEGORY_AUDIO_PROCESSING, switchboard::NODE_CATEGORY_EFFECTS}
        };
    }

    explicit RingModNode(const std::map<std::string, std::any>& config);
    ~RingModNode() override = default;

    // SingleBusAudioProcessorNode interface
    bool setBusFormat(switchboard::AudioBusFormat& inputBusFormat,
                      switchboard::AudioBusFormat& outputBusFormat) override;
    bool process(switchboard::AudioBus& inBus, switchboard::AudioBus& outBus) override;

    // Parameter access
    switchboard::Result<void> setValue(const std::string& key, const std::any& value) override;
    switchboard::Result<std::any> getValue(const std::string& key) override;

private:
    // Thread-safe parameters
    std::atomic<float> carrierFrequency_{100.0f}; // Hz
    std::atomic<float> mix_{1.0f};                // 0.0 to 1.0
    std::atomic<float> threshold_{0.02f};         // Input gate threshold

    // Oscillator state
    double phase_ = 0.0;
    double phaseIncrement_ = 0.0;
    uint sampleRate_ = 44100;
};

} // namespace voicechanger
