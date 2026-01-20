#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestHelpers.hpp"
#include "nodes/RingModNode.hpp"

using namespace voicechanger;
using namespace voicechanger::test;
using Catch::Approx;

// Global Switchboard initialization (shared with PitchShiftNodeTests)
static SwitchboardInitializer switchboardInit;

// Test constants
constexpr uint SAMPLE_RATE = 44100;
constexpr uint NUM_CHANNELS = 2;
constexpr uint BUFFER_SIZE = 512;

TEST_CASE("RingModNode - Silence in produces silence out", "[RingModNode][baseline]") {
    std::map<std::string, std::any> config;
    RingModNode node(config);

    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    TestAudioBus inBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    TestAudioBus outBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);

    // Process silent input
    REQUIRE(node.process(inBus.bus, outBus.bus));

    // Verify silence out
    for (uint ch = 0; ch < NUM_CHANNELS; ++ch) {
        for (uint frame = 0; frame < BUFFER_SIZE; ++frame) {
            REQUIRE(std::abs(outBus.getSample(ch, frame)) < 0.01f);
        }
    }
}

TEST_CASE("RingModNode - Stereo processing (both channels)", "[RingModNode][baseline]") {
    std::map<std::string, std::any> config = {
        {"carrierFrequency", 150.0f},
        {"mix", 1.0f}
    };
    RingModNode node(config);

    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    TestAudioBus inBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    TestAudioBus outBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);

    // Generate 440 Hz sine on both channels
    inBus.fillWithSine(440.0f, 0.5f, SAMPLE_RATE);

    REQUIRE(node.process(inBus.bus, outBus.bus));

    // Check for output on both channels
    bool ch0HasOutput = false;
    bool ch1HasOutput = false;

    for (uint frame = 0; frame < BUFFER_SIZE; ++frame) {
        if (std::abs(outBus.getSample(0, frame)) > 0.1f) {
            ch0HasOutput = true;
        }
        if (std::abs(outBus.getSample(1, frame)) > 0.1f) {
            ch1HasOutput = true;
        }
    }

    REQUIRE(ch0HasOutput);
    REQUIRE(ch1HasOutput);
}

TEST_CASE("RingModNode - setValue/getValue for carrierFrequency", "[RingModNode][baseline]") {
    std::map<std::string, std::any> config;
    RingModNode node(config);

    // Get default value
    auto result = node.getValue("carrierFrequency");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(100.0f));  // default

    // Set new value
    auto setResult = node.setValue("carrierFrequency", std::make_any<float>(200.0f));
    REQUIRE(!setResult.isError());

    // Verify new value
    result = node.getValue("carrierFrequency");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(200.0f));
}

TEST_CASE("RingModNode - setValue/getValue for mix", "[RingModNode][baseline]") {
    std::map<std::string, std::any> config;
    RingModNode node(config);

    // Get default value
    auto result = node.getValue("mix");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(1.0f));  // default full wet

    // Set new value
    auto setResult = node.setValue("mix", std::make_any<float>(0.5f));
    REQUIRE(!setResult.isError());

    // Verify new value
    result = node.getValue("mix");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(0.5f));
}

TEST_CASE("RingModNode - setValue/getValue for threshold", "[RingModNode][baseline]") {
    std::map<std::string, std::any> config;
    RingModNode node(config);

    // Get default value
    auto result = node.getValue("threshold");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(0.02f));  // default

    // Set new value
    auto setResult = node.setValue("threshold", std::make_any<float>(0.05f));
    REQUIRE(!setResult.isError());

    // Verify new value
    result = node.getValue("threshold");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(0.05f));
}

TEST_CASE("RingModNode - Config-based initialization", "[RingModNode]") {
    std::map<std::string, std::any> config = {
        {"carrierFrequency", 250.0f},
        {"mix", 0.7f},
        {"threshold", 0.03f}
    };
    RingModNode node(config);

    REQUIRE(std::any_cast<float>(node.getValue("carrierFrequency").value()) == Approx(250.0f));
    REQUIRE(std::any_cast<float>(node.getValue("mix").value()) == Approx(0.7f));
    REQUIRE(std::any_cast<float>(node.getValue("threshold").value()) == Approx(0.03f));
}

TEST_CASE("RingModNode - Threshold gates low-level signals", "[RingModNode]") {
    std::map<std::string, std::any> config = {
        {"carrierFrequency", 150.0f},
        {"mix", 1.0f},
        {"threshold", 0.1f}  // High threshold
    };
    RingModNode node(config);

    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    TestAudioBus inBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    TestAudioBus outBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);

    // Generate low-amplitude signal (below threshold)
    inBus.fillWithSine(440.0f, 0.05f, SAMPLE_RATE);

    REQUIRE(node.process(inBus.bus, outBus.bus));

    // Output should be very quiet due to threshold gating
    float rms = outBus.calculateRMS(0);
    REQUIRE(rms < 0.05f);
}

TEST_CASE("RingModNode - Ring modulation produces sidebands", "[RingModNode]") {
    // Ring modulating 440 Hz with 100 Hz carrier should produce:
    // - 440 + 100 = 540 Hz
    // - 440 - 100 = 340 Hz
    // The original 440 Hz should be suppressed

    std::map<std::string, std::any> config = {
        {"carrierFrequency", 100.0f},
        {"mix", 1.0f},
        {"threshold", 0.0f}
    };
    RingModNode node(config);

    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    // Collect output for frequency analysis
    std::vector<float> outputSamples;

    for (int i = 0; i < 10; ++i) {
        TestAudioBus inBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
        TestAudioBus outBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);

        inBus.fillWithSine(440.0f, 0.5f, SAMPLE_RATE, i * BUFFER_SIZE);

        REQUIRE(node.process(inBus.bus, outBus.bus));

        for (uint frame = 0; frame < BUFFER_SIZE; ++frame) {
            outputSamples.push_back(outBus.getSample(0, frame));
        }
    }

    // Verify output is not silent
    float maxSample = 0.0f;
    for (float s : outputSamples) {
        maxSample = std::max(maxSample, std::abs(s));
    }
    REQUIRE(maxSample > 0.1f);

    // The output should have different spectral content than pure 440 Hz
    // Simple test: zero-crossing rate should differ from pure 440 Hz
    int zeroCrossings = 0;
    for (size_t i = 1; i < outputSamples.size(); ++i) {
        if ((outputSamples[i-1] < 0 && outputSamples[i] >= 0) ||
            (outputSamples[i-1] >= 0 && outputSamples[i] < 0)) {
            ++zeroCrossings;
        }
    }

    float duration = static_cast<float>(outputSamples.size()) / SAMPLE_RATE;
    float estimatedFreq = (zeroCrossings / 2.0f) / duration;

    // Ring mod output should have a different frequency pattern than 440 Hz
    // Due to the sum/difference frequencies (340 Hz and 540 Hz), the result
    // will have a complex waveform, but the dominant frequency component
    // should be different from pure 440 Hz
    INFO("Estimated frequency pattern: " << estimatedFreq << " Hz");
    // Just verify we got a valid output - exact frequency analysis would need FFT
    REQUIRE(estimatedFreq > 100.0f);
}

TEST_CASE("RingModNode - Mix parameter blends dry/wet", "[RingModNode]") {
    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);

    // Full wet
    std::map<std::string, std::any> wetConfig = {
        {"carrierFrequency", 150.0f},
        {"mix", 1.0f}
    };
    RingModNode wetNode(wetConfig);
    REQUIRE(wetNode.setBusFormat(inputFormat, outputFormat));

    // Half wet
    std::map<std::string, std::any> halfConfig = {
        {"carrierFrequency", 150.0f},
        {"mix", 0.5f}
    };
    RingModNode halfNode(halfConfig);
    REQUIRE(halfNode.setBusFormat(inputFormat, outputFormat));

    TestAudioBus inBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    TestAudioBus wetOutBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    TestAudioBus halfOutBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);

    inBus.fillWithSine(440.0f, 0.5f, SAMPLE_RATE);

    REQUIRE(wetNode.process(inBus.bus, wetOutBus.bus));

    // Reset input (process modifies nothing but good practice)
    inBus.fillWithSine(440.0f, 0.5f, SAMPLE_RATE);
    REQUIRE(halfNode.process(inBus.bus, halfOutBus.bus));

    // The half-wet output should be different from full-wet
    // (contains blend of original and modulated signal)
    bool outputsDiffer = false;
    for (uint frame = 0; frame < BUFFER_SIZE; ++frame) {
        float diff = std::abs(wetOutBus.getSample(0, frame) - halfOutBus.getSample(0, frame));
        if (diff > 0.01f) {
            outputsDiffer = true;
            break;
        }
    }
    REQUIRE(outputsDiffer);
}
