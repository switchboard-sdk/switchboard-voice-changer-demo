/**
 * Voice Changer Demo
 *
 * Real-time voice changer with 10 preset voices.
 * Uses Switchboard SDK for audio I/O and custom voice effect nodes.
 *
 * Controls:
 * - 1-9, 0: Select preset directly (1=preset 1, 0=preset 10)
 * - Up/Down arrows: Cycle through presets
 * - Q or Esc: Quit
 */

#include <switchboard/Switchboard.hpp>
#include <switchboard_v2/AudioEngine.hpp>
#include <switchboard_v2/AudioGraph.hpp>
#include <switchboard_core/ExtensionManager.hpp>

#include "extension/VoiceChangerExtension.hpp"
#include "nodes/PitchShiftNode.hpp"
#include "nodes/RingModNode.hpp"
#include "presets/VoicePresets.hpp"

#include <ChorusNode.hpp>
#include <FlangerNode.hpp>
#include <VibratoNode.hpp>
#include <DelayNode.hpp>

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <termios.h>
#include <unistd.h>

using namespace switchboard;
using namespace voicechanger;

// Global flag for clean shutdown
static std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    (void)signum;
    g_running = false;
}

/**
 * Terminal raw mode helper for keyboard input.
 */
class TerminalRawMode {
public:
    TerminalRawMode() {
        tcgetattr(STDIN_FILENO, &originalTermios_);
        struct termios raw = originalTermios_;
        raw.c_lflag &= ~(ICANON | ECHO);  // Disable canonical mode and echo
        raw.c_cc[VMIN] = 0;   // Non-blocking
        raw.c_cc[VTIME] = 1;  // 100ms timeout
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    ~TerminalRawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios_);
    }

    // Read a single key, returns -1 if no key available
    int readKey() {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            // Handle escape sequences (arrow keys)
            if (c == '\x1b') {
                char seq[2];
                if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
                if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': return 1000;  // Up arrow
                        case 'B': return 1001;  // Down arrow
                    }
                }
                return '\x1b';
            }
            return c;
        }
        return -1;
    }

private:
    struct termios originalTermios_;
};

/**
 * Apply a voice preset to all audio effect nodes.
 */
void applyPreset(const VoicePreset& preset,
                 PitchShiftNode* pitchShiftNode,
                 RingModNode* ringModNode,
                 extensions::audioeffects::ChorusNode* chorusNode,
                 extensions::audioeffects::FlangerNode* flangerNode,
                 extensions::audioeffects::VibratoNode* vibratoNode,
                 extensions::audioeffects::DelayNode* delayNode) {

    // Apply pitch shift parameters
    pitchShiftNode->setValue("pitchShift", std::make_any<float>(preset.pitchShift));
    pitchShiftNode->setValue("formantPreserve", std::make_any<float>(preset.formantPreserve));
    pitchShiftNode->setValue("outputGain", std::make_any<float>(preset.outputGain));

    // Apply ring modulation parameters
    if (preset.useRingMod) {
        ringModNode->setValue("carrierFrequency", std::make_any<float>(preset.carrierFrequency));
        ringModNode->setValue("mix", std::make_any<float>(preset.ringModMix));
    } else {
        ringModNode->setValue("mix", std::make_any<float>(0.0f));  // Bypass
    }

    // Apply chorus parameters
    chorusNode->setIsEnabled(preset.useChorus);
    if (preset.useChorus) {
        chorusNode->setSweepWidth(preset.chorusSweepWidth);
        chorusNode->setFrequency(preset.chorusFrequency);
    }

    // Apply flanger parameters
    flangerNode->setIsEnabled(preset.useFlanger);
    if (preset.useFlanger) {
        flangerNode->setSweepWidth(preset.flangerSweepWidth);
        flangerNode->setFrequency(preset.flangerFrequency);
    }

    // Apply vibrato parameters
    vibratoNode->setIsEnabled(preset.useVibrato);
    if (preset.useVibrato) {
        vibratoNode->setSweepWidth(preset.vibratoSweepWidth);
        vibratoNode->setFrequency(preset.vibratoFrequency);
    }

    // Apply delay parameters
    delayNode->setIsEnabled(preset.useDelay);
    if (preset.useDelay) {
        delayNode->setDelayMs(preset.delayMs);
        delayNode->setFeedbackLevel(preset.delayFeedback);
        delayNode->setWetMix(preset.delayWetMix);
        delayNode->setDryMix(1.0f - preset.delayWetMix * 0.5f);  // Keep some dry signal
    } else {
        delayNode->setWetMix(0.0f);
        delayNode->setDryMix(1.0f);
    }
}

/**
 * Display the current preset and status.
 */
void displayStatus(int presetIndex, const VoicePreset& preset) {
    // Clear line and print status
    std::cout << "\r\033[K";  // Clear line
    std::cout << "Preset " << (presetIndex + 1) << "/10: " << preset.name;
    std::cout << " | " << preset.description;
    std::cout << std::flush;
}

int main() {
    std::cout << "====================================" << std::endl;
    std::cout << "    Voice Changer Demo" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  1-9, 0  : Select preset (0 = preset 10)" << std::endl;
    std::cout << "  Up/Down : Cycle through presets" << std::endl;
    std::cout << "  Q/Esc   : Quit" << std::endl;
    std::cout << std::endl;

    // Set up signal handler for clean shutdown
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Initialize Switchboard SDK
    Config sdkConfig({{"appID", "voice-changer-demo"}, {"appSecret", "demo"}});
    auto initResult = Switchboard::initialize(sdkConfig);
    if (initResult.isError()) {
        std::cerr << "Failed to initialize Switchboard SDK: "
                  << initResult.error().message << std::endl;
        return 1;
    }

    // Register VoiceChanger extension
    auto extension = std::make_shared<VoiceChangerExtension>();
    ExtensionManager::getInstance().registerExtension(extension);

    // Create audio engine with ALSA API on Linux
    AudioEngine audioEngine(AudioIO::AudioAPI::ALSA);

    // List available audio devices
    auto devices = audioEngine.getAudioDevices();
    std::cout << "Available audio devices:" << std::endl;
    for (const auto& device : devices) {
        std::cout << "  - " << device.name;
        if (device.isInputDevice()) std::cout << " [Input]";
        if (device.isOutputDevice()) std::cout << " [Output]";
        std::cout << std::endl;
    }
    std::cout << std::endl;

    // Create audio graph
    AudioGraph audioGraph(2, 1024, 44100, 512);

    // Create voice effect nodes
    std::map<std::string, std::any> emptyConfig;
    auto pitchShiftNode = std::make_unique<PitchShiftNode>(emptyConfig);
    auto ringModNode = std::make_unique<RingModNode>(emptyConfig);
    auto chorusNode = std::make_unique<extensions::audioeffects::ChorusNode>(2);
    auto flangerNode = std::make_unique<extensions::audioeffects::FlangerNode>(2);
    auto vibratoNode = std::make_unique<extensions::audioeffects::VibratoNode>(2);
    auto delayNode = std::make_unique<extensions::audioeffects::DelayNode>(2);

    // Add nodes to graph
    audioGraph.addNode(*pitchShiftNode);
    audioGraph.addNode(*ringModNode);
    audioGraph.addNode(*chorusNode);
    audioGraph.addNode(*flangerNode);
    audioGraph.addNode(*vibratoNode);
    audioGraph.addNode(*delayNode);

    // Connect: input -> pitchShift -> ringMod -> vibrato -> chorus -> flanger -> delay -> output
    // This order is intentional:
    // 1. PitchShift first (core transformation)
    // 2. RingMod (adds harmonic content)
    // 3. Vibrato (pitch modulation)
    // 4. Chorus (width/thickness)
    // 5. Flanger (metallic sweep)
    // 6. Delay last (space/echo)
    auto& inputNode = audioGraph.getInputNode();
    auto& outputNode = audioGraph.getOutputNode();

    audioGraph.connect(inputNode, *pitchShiftNode);
    audioGraph.connect(*pitchShiftNode, *ringModNode);
    audioGraph.connect(*ringModNode, *vibratoNode);
    audioGraph.connect(*vibratoNode, *chorusNode);
    audioGraph.connect(*chorusNode, *flangerNode);
    audioGraph.connect(*flangerNode, *delayNode);
    audioGraph.connect(*delayNode, outputNode);

    // Apply initial preset (preset 1 = index 0)
    int currentPresetIndex = 0;
    applyPreset(getPreset(currentPresetIndex),
                pitchShiftNode.get(),
                ringModNode.get(),
                chorusNode.get(),
                flangerNode.get(),
                vibratoNode.get(),
                delayNode.get());

    // Configure stream parameters
    AudioIO::StreamParameters streamParams;
    streamParams.preferredSampleRate = 44100;
    streamParams.preferredBufferSize = 512;
    streamParams.numberOfInputChannels = 2;
    streamParams.numberOfOutputChannels = 2;

    // Start audio engine
    auto startResult = audioEngine.start(&audioGraph, streamParams);
    if (startResult.isError()) {
        std::cerr << "Failed to start audio engine: "
                  << startResult.error().message << std::endl;
        Switchboard::deinitialize();
        return 1;
    }

    std::cout << "Audio engine started. Speak into your microphone!" << std::endl;
    std::cout << std::endl;

    // Display initial preset
    displayStatus(currentPresetIndex, getPreset(currentPresetIndex));

    // Enter raw terminal mode for keyboard input
    TerminalRawMode terminalRaw;

    // Main loop
    while (g_running) {
        int key = terminalRaw.readKey();

        if (key == -1) {
            continue;  // No key pressed
        }

        bool presetChanged = false;

        switch (key) {
            case 'q':
            case 'Q':
            case '\x1b':  // Escape
                g_running = false;
                break;

            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                currentPresetIndex = key - '1';  // '1' -> 0, '2' -> 1, etc.
                presetChanged = true;
                break;

            case '0':
                currentPresetIndex = 9;  // '0' -> preset 10
                presetChanged = true;
                break;

            case 1000:  // Up arrow
                currentPresetIndex = (currentPresetIndex - 1 + 10) % 10;
                presetChanged = true;
                break;

            case 1001:  // Down arrow
                currentPresetIndex = (currentPresetIndex + 1) % 10;
                presetChanged = true;
                break;
        }

        if (presetChanged) {
            applyPreset(getPreset(currentPresetIndex),
                        pitchShiftNode.get(),
                        ringModNode.get(),
                        chorusNode.get(),
                        flangerNode.get(),
                        vibratoNode.get(),
                        delayNode.get());
            displayStatus(currentPresetIndex, getPreset(currentPresetIndex));
        }
    }

    std::cout << std::endl << std::endl;
    std::cout << "Shutting down..." << std::endl;

    // Stop audio engine
    audioEngine.stop();

    // Cleanup
    Switchboard::deinitialize();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}
