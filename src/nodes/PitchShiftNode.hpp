#pragma once

#include <switchboard_core/SingleBusAudioProcessorNode.hpp>
#include <switchboard_core/NodeTypeInfo.hpp>

#include <any>
#include <atomic>
#include <map>
#include <memory>
#include <string>

namespace voicechanger {

/**
 * PitchShiftNode - Real-time pitch shifting with formant preservation.
 *
 * Uses signalsmith-stretch library for high-quality pitch shifting.
 * Supports formant preservation to prevent chipmunk effect on voice.
 *
 * Parameters:
 * - pitchShift: Pitch shift in semitones (-24 to +24)
 * - formantPreserve: Formant preservation amount (0.0 to 1.0)
 * - mix: Dry/wet mix (0.0 = dry, 1.0 = wet)
 * - outputGain: Output gain multiplier (0.0 to 4.0)
 */
class PitchShiftNode : public switchboard::SingleBusAudioProcessorNode {
public:
    static switchboard::NodeTypeInfo getNodeTypeInfo() {
        return switchboard::NodeTypeInfo{
            "VoiceChanger",
            "PitchShift",
            "PitchShift",
            "Real-time pitch shifting with formant preservation",
            {switchboard::NODE_CATEGORY_AUDIO_PROCESSING, switchboard::NODE_CATEGORY_EFFECTS}
        };
    }

    explicit PitchShiftNode(const std::map<std::string, std::any>& config);
    ~PitchShiftNode() override;

    // SingleBusAudioProcessorNode interface
    bool setBusFormat(switchboard::AudioBusFormat& inputBusFormat,
                      switchboard::AudioBusFormat& outputBusFormat) override;
    bool process(switchboard::AudioBus& inBus, switchboard::AudioBus& outBus) override;

    // Parameter access
    switchboard::Result<void> setValue(const std::string& key, const std::any& value) override;
    switchboard::Result<std::any> getValue(const std::string& key) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

    // Thread-safe parameters
    std::atomic<float> pitchShift_{0.0f};      // Semitones
    std::atomic<float> formantPreserve_{1.0f}; // 0.0 to 1.0
    std::atomic<float> mix_{1.0f};             // 0.0 to 1.0
    std::atomic<float> outputGain_{1.0f};      // 0.0 to 4.0

    void updateStretchParameters();
};

} // namespace voicechanger
