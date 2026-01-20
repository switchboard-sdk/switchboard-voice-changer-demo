#pragma once

#include <switchboard_core/Extension.hpp>

#include <memory>

namespace voicechanger {

/**
 * VoiceChanger Switchboard extension.
 * Provides voice changing audio effect nodes:
 * - VoiceChanger.PitchShift: Pitch shifting with formant preservation
 * - VoiceChanger.RingMod: Ring modulation for robotic/alien effects
 */
class VoiceChangerExtension : public switchboard::Extension {
public:
    /**
     * @brief Loads and registers the VoiceChanger extension with the SDK.
     * @details Call this before Switchboard::initialize() to make VoiceChanger nodes available.
     */
    static void load();

    VoiceChangerExtension();
    ~VoiceChangerExtension() override = default;

    std::string getName() override;
    std::shared_ptr<switchboard::NodeFactory> getNodeFactory() override;

    switchboard::Result<void> initialize(const std::map<std::string, std::any>& config) override;
    switchboard::Result<void> deinitialize() override;

private:
    std::shared_ptr<switchboard::NodeFactory> nodeFactory_;
};

} // namespace voicechanger

// C API for extension loading
extern "C" {
    switchboard::Extension* createExtension();
    void destroyExtension(switchboard::Extension* extension);
}
