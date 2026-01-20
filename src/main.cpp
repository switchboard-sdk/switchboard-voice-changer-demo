/**
 * Voice Changer Demo
 *
 * Real-time voice changer with 10 preset voices loaded from JSON.
 * Uses Switchboard SDK V3 API for audio I/O and custom voice effect nodes.
 *
 * Controls:
 * - 1-9, 0: Select preset directly (1=preset 1, 0=preset 10)
 * - Up/Down arrows: Cycle through presets
 * - Q or Esc: Quit
 */

#include <switchboard/Switchboard.hpp>

#include "extension/VoiceChangerExtension.hpp"

#include <AudioEffectsExtension.hpp>

#include <atomic>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace switchboard;

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
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 0;
        raw.c_cc[VTIME] = 1;
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    ~TerminalRawMode() {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTermios_);
    }

    int readKey() {
        char c;
        if (read(STDIN_FILENO, &c, 1) == 1) {
            if (c == '\x1b') {
                char seq[2];
                if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
                if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
                if (seq[0] == '[') {
                    switch (seq[1]) {
                        case 'A': return 1000;  // Up
                        case 'B': return 1001;  // Down
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
 * Voice preset loaded from JSON.
 */
struct VoicePreset {
    std::string name;
    std::string description;
    std::string jsonContent;
};

/**
 * Read file contents.
 */
static std::optional<std::string> readFile(const std::string& path) {
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

/**
 * Extract a string value from JSON.
 */
static std::string extractJsonString(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return "";
    size_t start = json.find("\"", pos + search.length()) + 1;
    size_t end = json.find("\"", start);
    return json.substr(start, end - start);
}

/**
 * Load all JSON presets from the presets directory.
 */
std::vector<VoicePreset> loadPresets(const std::string& presetsDir) {
    std::vector<VoicePreset> presets;

    std::vector<std::string> presetFiles = {
        "01_deep_villain.json",
        "02_chipmunk.json",
        "03_robot.json",
        "04_alien.json",
        "05_monster.json",
        "06_radio.json",
        "07_demon.json",
        "08_ghost.json",
        "09_giant.json",
        "10_cyborg.json"
    };

    for (const auto& filename : presetFiles) {
        auto content = readFile(presetsDir + "/" + filename);
        if (!content.has_value()) {
            std::cerr << "Warning: Could not open preset file: " << filename << std::endl;
            continue;
        }

        VoicePreset preset;
        preset.jsonContent = content.value();
        preset.name = extractJsonString(preset.jsonContent, "name");
        preset.description = extractJsonString(preset.jsonContent, "description");
        presets.push_back(preset);
    }

    return presets;
}

/**
 * Apply preset parameters to nodes via Switchboard API.
 */
void applyPreset(const VoicePreset& preset) {
    auto extractFloat = [&](const std::string& nodeId, const std::string& key) -> std::optional<float> {
        // Find the node's config block
        std::string nodeSearch = "\"id\": \"" + nodeId + "\"";
        size_t nodePos = preset.jsonContent.find(nodeSearch);
        if (nodePos == std::string::npos) {
            nodeSearch = "\"id\":\"" + nodeId + "\"";
            nodePos = preset.jsonContent.find(nodeSearch);
        }
        if (nodePos == std::string::npos) return std::nullopt;

        // Find the key within this node's config
        size_t configPos = preset.jsonContent.find("\"config\"", nodePos);
        if (configPos == std::string::npos) return std::nullopt;

        std::string search = "\"" + key + "\":";
        size_t pos = preset.jsonContent.find(search, configPos);
        if (pos == std::string::npos || pos > configPos + 500) return std::nullopt;

        pos += search.length();
        while (pos < preset.jsonContent.length() && (preset.jsonContent[pos] == ' ' || preset.jsonContent[pos] == '\t')) pos++;

        size_t end = pos;
        while (end < preset.jsonContent.length() && (isdigit(preset.jsonContent[end]) || preset.jsonContent[end] == '.' || preset.jsonContent[end] == '-')) end++;

        if (end > pos) {
            return std::stof(preset.jsonContent.substr(pos, end - pos));
        }
        return std::nullopt;
    };

    auto extractBool = [&](const std::string& nodeId, const std::string& key) -> std::optional<bool> {
        std::string nodeSearch = "\"id\": \"" + nodeId + "\"";
        size_t nodePos = preset.jsonContent.find(nodeSearch);
        if (nodePos == std::string::npos) {
            nodeSearch = "\"id\":\"" + nodeId + "\"";
            nodePos = preset.jsonContent.find(nodeSearch);
        }
        if (nodePos == std::string::npos) return std::nullopt;

        size_t configPos = preset.jsonContent.find("\"config\"", nodePos);
        if (configPos == std::string::npos) return std::nullopt;

        std::string search = "\"" + key + "\":";
        size_t pos = preset.jsonContent.find(search, configPos);
        if (pos == std::string::npos || pos > configPos + 500) return std::nullopt;

        pos += search.length();
        while (pos < preset.jsonContent.length() && (preset.jsonContent[pos] == ' ' || preset.jsonContent[pos] == '\t')) pos++;

        if (preset.jsonContent.substr(pos, 4) == "true") return true;
        if (preset.jsonContent.substr(pos, 5) == "false") return false;
        return std::nullopt;
    };

    auto extractInt = [&](const std::string& nodeId, const std::string& key) -> std::optional<int> {
        std::string nodeSearch = "\"id\": \"" + nodeId + "\"";
        size_t nodePos = preset.jsonContent.find(nodeSearch);
        if (nodePos == std::string::npos) {
            nodeSearch = "\"id\":\"" + nodeId + "\"";
            nodePos = preset.jsonContent.find(nodeSearch);
        }
        if (nodePos == std::string::npos) return std::nullopt;

        size_t configPos = preset.jsonContent.find("\"config\"", nodePos);
        if (configPos == std::string::npos) return std::nullopt;

        std::string search = "\"" + key + "\":";
        size_t pos = preset.jsonContent.find(search, configPos);
        if (pos == std::string::npos || pos > configPos + 500) return std::nullopt;

        pos += search.length();
        while (pos < preset.jsonContent.length() && (preset.jsonContent[pos] == ' ' || preset.jsonContent[pos] == '\t')) pos++;

        size_t end = pos;
        while (end < preset.jsonContent.length() && (isdigit(preset.jsonContent[end]) || preset.jsonContent[end] == '-')) end++;

        if (end > pos) {
            return std::stoi(preset.jsonContent.substr(pos, end - pos));
        }
        return std::nullopt;
    };

    // Apply PitchShift parameters
    if (auto v = extractFloat("pitchShift", "pitchShift"))
        Switchboard::setValue("pitchShift", "pitchShift", std::make_any<float>(*v));
    if (auto v = extractFloat("pitchShift", "formantPreserve"))
        Switchboard::setValue("pitchShift", "formantPreserve", std::make_any<float>(*v));
    if (auto v = extractFloat("pitchShift", "outputGain"))
        Switchboard::setValue("pitchShift", "outputGain", std::make_any<float>(*v));

    // Apply RingMod parameters
    if (auto v = extractFloat("ringMod", "carrierFrequency"))
        Switchboard::setValue("ringMod", "carrierFrequency", std::make_any<float>(*v));
    if (auto v = extractFloat("ringMod", "mix"))
        Switchboard::setValue("ringMod", "mix", std::make_any<float>(*v));

    // Apply Vibrato parameters
    if (auto v = extractBool("vibrato", "isEnabled"))
        Switchboard::setValue("vibrato", "isEnabled", std::make_any<bool>(*v));
    if (auto v = extractFloat("vibrato", "sweepWidth"))
        Switchboard::setValue("vibrato", "sweepWidth", std::make_any<float>(*v));
    if (auto v = extractFloat("vibrato", "frequency"))
        Switchboard::setValue("vibrato", "frequency", std::make_any<float>(*v));

    // Apply Chorus parameters
    if (auto v = extractBool("chorus", "isEnabled"))
        Switchboard::setValue("chorus", "isEnabled", std::make_any<bool>(*v));
    if (auto v = extractFloat("chorus", "sweepWidth"))
        Switchboard::setValue("chorus", "sweepWidth", std::make_any<float>(*v));
    if (auto v = extractFloat("chorus", "frequency"))
        Switchboard::setValue("chorus", "frequency", std::make_any<float>(*v));

    // Apply Flanger parameters
    if (auto v = extractBool("flanger", "isEnabled"))
        Switchboard::setValue("flanger", "isEnabled", std::make_any<bool>(*v));
    if (auto v = extractFloat("flanger", "sweepWidth"))
        Switchboard::setValue("flanger", "sweepWidth", std::make_any<float>(*v));
    if (auto v = extractFloat("flanger", "frequency"))
        Switchboard::setValue("flanger", "frequency", std::make_any<float>(*v));

    // Apply Delay parameters
    if (auto v = extractBool("delay", "isEnabled"))
        Switchboard::setValue("delay", "isEnabled", std::make_any<bool>(*v));
    if (auto v = extractInt("delay", "delayMs"))
        Switchboard::setValue("delay", "delayMs", std::make_any<uint>(*v));
    if (auto v = extractFloat("delay", "feedbackLevel"))
        Switchboard::setValue("delay", "feedbackLevel", std::make_any<float>(*v));
    if (auto v = extractFloat("delay", "wetMix"))
        Switchboard::setValue("delay", "wetMix", std::make_any<float>(*v));
    if (auto v = extractFloat("delay", "dryMix"))
        Switchboard::setValue("delay", "dryMix", std::make_any<float>(*v));
}

/**
 * Display the current preset status.
 */
void displayStatus(int presetIndex, const VoicePreset& preset) {
    std::cout << "\r\033[K";
    std::cout << "Preset " << (presetIndex + 1) << "/10: " << preset.name;
    std::cout << " | " << preset.description;
    std::cout << std::flush;
}

int main(int argc, char* argv[]) {
    std::cout << "====================================" << std::endl;
    std::cout << "    Voice Changer Demo" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  1-9, 0  : Select preset (0 = preset 10)" << std::endl;
    std::cout << "  Up/Down : Cycle through presets" << std::endl;
    std::cout << "  Q/Esc   : Quit" << std::endl;
    std::cout << std::endl;

    // Determine presets directory
    std::string presetsDir = "src/presets/json";
    if (argc > 1) {
        presetsDir = argv[1];
    }
    if (!std::filesystem::exists(presetsDir)) {
        presetsDir = "../src/presets/json";
    }
    if (!std::filesystem::exists(presetsDir)) {
        std::cerr << "Error: Could not find presets directory." << std::endl;
        return 1;
    }

    // Set up signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    // Load presets
    std::vector<VoicePreset> presets = loadPresets(presetsDir);
    if (presets.empty()) {
        std::cerr << "Error: No presets found in " << presetsDir << std::endl;
        return 1;
    }
    std::cout << "Loaded " << presets.size() << " presets" << std::endl;

    // Load extensions
    extensions::audioeffects::AudioEffectsExtension::load();
    voicechanger::VoiceChangerExtension::load();

    // Initialize Switchboard SDK
    Config sdkConfig({
        {"appID", "voice-changer-demo"},
        {"appSecret", "demo"},
        {"extensions", Config({
            {"AudioEffects", Config()},
            {"VoiceChanger", Config()}
        })}
    });

    auto initResult = Switchboard::initialize(sdkConfig);
    if (initResult.isError()) {
        std::cerr << "Failed to initialize Switchboard SDK: " << initResult.error().message << std::endl;
        return 1;
    }

    // Build engine JSON from first preset
    // Wrap the preset's graph in a RealTimeGraphRenderer
    const std::string& firstPresetJson = presets[0].jsonContent;

    // Extract the graph portion from the preset
    size_t graphPos = firstPresetJson.find("\"graph\"");
    size_t graphStart = firstPresetJson.find("{", graphPos);
    int braceCount = 1;
    size_t graphEnd = graphStart + 1;
    while (graphEnd < firstPresetJson.length() && braceCount > 0) {
        if (firstPresetJson[graphEnd] == '{') braceCount++;
        else if (firstPresetJson[graphEnd] == '}') braceCount--;
        graphEnd++;
    }
    std::string graphContent = firstPresetJson.substr(graphStart, graphEnd - graphStart);

    // Build the engine JSON
    std::string engineJson = R"({
        "type": "RealTimeGraphRenderer",
        "config": {
            "microphoneEnabled": true,
            "graph": {
                "config": {
                    "sampleRate": 44100,
                    "bufferSize": 512
                },
                "nodes": )" + [&]() {
                    size_t nodesPos = graphContent.find("\"nodes\"");
                    size_t start = graphContent.find("[", nodesPos);
                    int bracketCount = 1;
                    size_t end = start + 1;
                    while (end < graphContent.length() && bracketCount > 0) {
                        if (graphContent[end] == '[') bracketCount++;
                        else if (graphContent[end] == ']') bracketCount--;
                        end++;
                    }
                    return graphContent.substr(start, end - start);
                }() + R"(,
                "connections": )" + [&]() {
                    size_t connPos = graphContent.find("\"connections\"");
                    size_t start = graphContent.find("[", connPos);
                    int bracketCount = 1;
                    size_t end = start + 1;
                    while (end < graphContent.length() && bracketCount > 0) {
                        if (graphContent[end] == '[') bracketCount++;
                        else if (graphContent[end] == ']') bracketCount--;
                        end++;
                    }
                    return graphContent.substr(start, end - start);
                }() + R"(
            }
        }
    })";

    // Create audio engine from JSON
    auto createResult = Switchboard::createEngine(engineJson);
    if (createResult.isError()) {
        std::cerr << "Failed to create engine: " << createResult.error().message << std::endl;
        Switchboard::deinitialize();
        return 1;
    }
    const std::string engineID = createResult.value();

    // Apply initial preset parameters
    int currentPresetIndex = 0;
    applyPreset(presets[currentPresetIndex]);

    // Start audio engine
    auto startResult = Switchboard::callAction(engineID, "start", {});
    if (startResult.isError()) {
        std::cerr << "Failed to start engine: " << startResult.error().message << std::endl;
        Switchboard::destroyEngine(engineID);
        Switchboard::deinitialize();
        return 1;
    }

    std::cout << "Audio engine started. Speak into your microphone!" << std::endl;
    std::cout << std::endl;

    displayStatus(currentPresetIndex, presets[currentPresetIndex]);

    // Enter raw terminal mode
    TerminalRawMode terminalRaw;

    // Main loop
    while (g_running) {
        int key = terminalRaw.readKey();
        if (key == -1) continue;

        bool presetChanged = false;
        int numPresets = static_cast<int>(presets.size());

        switch (key) {
            case 'q':
            case 'Q':
            case '\x1b':
                g_running = false;
                break;

            case '1': case '2': case '3': case '4': case '5':
            case '6': case '7': case '8': case '9':
                if (key - '1' < numPresets) {
                    currentPresetIndex = key - '1';
                    presetChanged = true;
                }
                break;

            case '0':
                if (numPresets >= 10) {
                    currentPresetIndex = 9;
                    presetChanged = true;
                }
                break;

            case 1000:  // Up
                currentPresetIndex = (currentPresetIndex - 1 + numPresets) % numPresets;
                presetChanged = true;
                break;

            case 1001:  // Down
                currentPresetIndex = (currentPresetIndex + 1) % numPresets;
                presetChanged = true;
                break;
        }

        if (presetChanged) {
            applyPreset(presets[currentPresetIndex]);
            displayStatus(currentPresetIndex, presets[currentPresetIndex]);
        }
    }

    std::cout << std::endl << std::endl;
    std::cout << "Shutting down..." << std::endl;

    // Stop and destroy engine
    Switchboard::callAction(engineID, "stop", {});
    Switchboard::destroyEngine(engineID);
    Switchboard::deinitialize();

    std::cout << "Goodbye!" << std::endl;
    return 0;
}
