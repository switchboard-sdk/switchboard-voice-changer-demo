#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestHelpers.hpp"
#include "nodes/PitchShiftNode.hpp"

using namespace voicechanger;
using namespace voicechanger::test;
using Catch::Approx;

// Global Switchboard initialization
static SwitchboardInitializer switchboardInit;

// Test constants
constexpr uint SAMPLE_RATE = 44100;
constexpr uint NUM_CHANNELS = 2;
constexpr uint BUFFER_SIZE = 512;
constexpr int WARMUP_BUFFERS = 20;  // STFT-based nodes need warmup

TEST_CASE("PitchShiftNode - Silence in produces silence out", "[PitchShiftNode][baseline]") {
    std::map<std::string, std::any> config;
    PitchShiftNode node(config);

    // Set bus format
    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    // Process multiple buffers for STFT warmup
    for (int i = 0; i < WARMUP_BUFFERS; ++i) {
        TestAudioBus inBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
        TestAudioBus outBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);

        REQUIRE(node.process(inBus.bus, outBus.bus));

        // After warmup, verify silence
        if (i > 10) {
            for (uint ch = 0; ch < NUM_CHANNELS; ++ch) {
                for (uint frame = 0; frame < BUFFER_SIZE; ++frame) {
                    REQUIRE(std::abs(outBus.getSample(ch, frame)) < 0.01f);
                }
            }
        }
    }
}

TEST_CASE("PitchShiftNode - Stereo processing (both channels)", "[PitchShiftNode][baseline]") {
    std::map<std::string, std::any> config;
    PitchShiftNode node(config);

    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    bool ch0HasOutput = false;
    bool ch1HasOutput = false;

    // Process multiple buffers with continuous sine wave
    for (int i = 0; i < 30; ++i) {
        TestAudioBus inBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
        TestAudioBus outBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);

        // Generate 440 Hz sine on both channels
        inBus.fillWithSine(440.0f, 0.5f, SAMPLE_RATE, i * BUFFER_SIZE);

        REQUIRE(node.process(inBus.bus, outBus.bus));

        // Check for output after warmup
        if (i > WARMUP_BUFFERS) {
            for (uint frame = 0; frame < BUFFER_SIZE; ++frame) {
                if (std::abs(outBus.getSample(0, frame)) > 0.1f) {
                    ch0HasOutput = true;
                }
                if (std::abs(outBus.getSample(1, frame)) > 0.1f) {
                    ch1HasOutput = true;
                }
            }
        }
    }

    REQUIRE(ch0HasOutput);
    REQUIRE(ch1HasOutput);
}

TEST_CASE("PitchShiftNode - setValue/getValue for pitchShift", "[PitchShiftNode][baseline]") {
    std::map<std::string, std::any> config;
    PitchShiftNode node(config);

    // Get default value
    auto result = node.getValue("pitchShift");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(0.0f));

    // Set new value
    auto setResult = node.setValue("pitchShift", std::make_any<float>(-12.0f));
    REQUIRE(!setResult.isError());

    // Verify new value
    result = node.getValue("pitchShift");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(-12.0f));
}

TEST_CASE("PitchShiftNode - setValue/getValue for formantPreserve", "[PitchShiftNode][baseline]") {
    std::map<std::string, std::any> config;
    PitchShiftNode node(config);

    // Get default value (should be 1.0 for full preservation)
    auto result = node.getValue("formantPreserve");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(1.0f));

    // Set new value
    auto setResult = node.setValue("formantPreserve", std::make_any<float>(0.5f));
    REQUIRE(!setResult.isError());

    // Verify new value
    result = node.getValue("formantPreserve");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(0.5f));
}

TEST_CASE("PitchShiftNode - setValue/getValue for mix", "[PitchShiftNode][baseline]") {
    std::map<std::string, std::any> config;
    PitchShiftNode node(config);

    // Get default value (should be 1.0 for full wet)
    auto result = node.getValue("mix");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(1.0f));

    // Set new value
    auto setResult = node.setValue("mix", std::make_any<float>(0.7f));
    REQUIRE(!setResult.isError());

    // Verify new value
    result = node.getValue("mix");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(0.7f));
}

TEST_CASE("PitchShiftNode - setValue/getValue for outputGain", "[PitchShiftNode][baseline]") {
    std::map<std::string, std::any> config;
    PitchShiftNode node(config);

    // Get default value (should be 1.0 for unity gain)
    auto result = node.getValue("outputGain");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(1.0f));

    // Set new value
    auto setResult = node.setValue("outputGain", std::make_any<float>(2.0f));
    REQUIRE(!setResult.isError());

    // Verify new value
    result = node.getValue("outputGain");
    REQUIRE(!result.isError());
    REQUIRE(std::any_cast<float>(result.value()) == Approx(2.0f));
}

TEST_CASE("PitchShiftNode - Config-based initialization", "[PitchShiftNode]") {
    std::map<std::string, std::any> config = {
        {"pitchShift", -8.0f},
        {"formantPreserve", 0.8f},
        {"mix", 0.9f},
        {"outputGain", 1.5f}
    };
    PitchShiftNode node(config);

    REQUIRE(std::any_cast<float>(node.getValue("pitchShift").value()) == Approx(-8.0f));
    REQUIRE(std::any_cast<float>(node.getValue("formantPreserve").value()) == Approx(0.8f));
    REQUIRE(std::any_cast<float>(node.getValue("mix").value()) == Approx(0.9f));
    REQUIRE(std::any_cast<float>(node.getValue("outputGain").value()) == Approx(1.5f));
}

TEST_CASE("PitchShiftNode - Pitch shift actually changes frequency", "[PitchShiftNode]") {
    // Shift up by 12 semitones (octave) - frequency should double
    // Note: formantPreserve=1.0 means full preservation (natural sounding)
    // formantPreserve=0.0 would mute the signal in signalsmith-stretch
    std::map<std::string, std::any> config = {
        {"pitchShift", 12.0f},
        {"formantPreserve", 1.0f}  // Keep formants preserved
    };
    PitchShiftNode node(config);

    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    // Collect output samples after warmup
    // signalsmith-stretch needs ~2646 samples latency = ~5-6 buffers at 512 samples
    // We use more buffers to ensure stable output
    std::vector<float> outputSamples;
    constexpr int TOTAL_BUFFERS = 50;
    constexpr int WARMUP_BUFFERS_PITCH = 15;  // Skip first 15 buffers

    float maxSampleDuringProcessing = 0.0f;

    for (int i = 0; i < TOTAL_BUFFERS; ++i) {
        TestAudioBus inBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
        TestAudioBus outBus(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);

        // 220 Hz input -> should become ~440 Hz
        inBus.fillWithSine(220.0f, 0.5f, SAMPLE_RATE, i * BUFFER_SIZE);

        REQUIRE(node.process(inBus.bus, outBus.bus));

        // Track max during all processing for debugging
        for (uint frame = 0; frame < BUFFER_SIZE; ++frame) {
            maxSampleDuringProcessing = std::max(maxSampleDuringProcessing,
                std::abs(outBus.getSample(0, frame)));
        }

        if (i > WARMUP_BUFFERS_PITCH) {
            for (uint frame = 0; frame < BUFFER_SIZE; ++frame) {
                outputSamples.push_back(outBus.getSample(0, frame));
            }
        }
    }

    INFO("Max sample during entire processing: " << maxSampleDuringProcessing);

    // Verify we have non-silent output
    float maxSample = 0.0f;
    for (float s : outputSamples) {
        maxSample = std::max(maxSample, std::abs(s));
    }
    INFO("Max sample in collected output: " << maxSample);
    REQUIRE(maxSample > 0.1f);

    // Simple zero-crossing frequency estimation
    int zeroCrossings = 0;
    for (size_t i = 1; i < outputSamples.size(); ++i) {
        if ((outputSamples[i-1] < 0 && outputSamples[i] >= 0) ||
            (outputSamples[i-1] >= 0 && outputSamples[i] < 0)) {
            ++zeroCrossings;
        }
    }

    // Expected: ~440 Hz means ~440 zero crossings per second
    // We have outputSamples.size() samples at 44100 Hz
    float duration = static_cast<float>(outputSamples.size()) / SAMPLE_RATE;
    float estimatedFreq = (zeroCrossings / 2.0f) / duration;  // divide by 2 because 2 crossings per cycle

    // Allow some tolerance (pitch shifting isn't perfectly precise)
    INFO("Estimated output frequency: " << estimatedFreq << " Hz (expected ~440 Hz)");
    REQUIRE(estimatedFreq > 350.0f);
    REQUIRE(estimatedFreq < 550.0f);
}

/**
 * Helper function to estimate frequency using zero-crossing analysis.
 * Returns estimated frequency in Hz.
 */
static float estimateFrequency(const std::vector<float>& samples, float sampleRate) {
    int zeroCrossings = 0;
    for (size_t i = 1; i < samples.size(); ++i) {
        if ((samples[i-1] < 0 && samples[i] >= 0) ||
            (samples[i-1] >= 0 && samples[i] < 0)) {
            ++zeroCrossings;
        }
    }
    float duration = static_cast<float>(samples.size()) / sampleRate;
    return (zeroCrossings / 2.0f) / duration;
}

/**
 * Helper to process through pitch shifter and collect output samples.
 */
static std::vector<float> processThroughPitchShifter(
    PitchShiftNode& node,
    float inputFreq,
    float amplitude,
    uint sampleRate,
    int totalBuffers,
    int warmupBuffers,
    uint bufferSize)
{
    std::vector<float> outputSamples;

    for (int i = 0; i < totalBuffers; ++i) {
        TestAudioBus inBus(sampleRate, 2, bufferSize);
        TestAudioBus outBus(sampleRate, 2, bufferSize);

        inBus.fillWithSine(inputFreq, amplitude, sampleRate, i * bufferSize);
        node.process(inBus.bus, outBus.bus);

        if (i > warmupBuffers) {
            for (uint frame = 0; frame < bufferSize; ++frame) {
                outputSamples.push_back(outBus.getSample(0, frame));
            }
        }
    }

    return outputSamples;
}

TEST_CASE("PitchShiftNode - Positive semitones shift frequency UP (chipmunk)", "[PitchShiftNode][direction]") {
    // +12 semitones should double the frequency (octave up)
    // This is the chipmunk preset setting
    std::map<std::string, std::any> config = {
        {"pitchShift", 12.0f},
        {"formantPreserve", 1.0f}
    };
    PitchShiftNode node(config);

    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    constexpr float INPUT_FREQ = 220.0f;  // A3
    constexpr float EXPECTED_OUTPUT_FREQ = 440.0f;  // A4 (octave up)

    auto outputSamples = processThroughPitchShifter(
        node, INPUT_FREQ, 0.5f, SAMPLE_RATE, 50, 15, BUFFER_SIZE);

    // Verify non-silent output
    float maxSample = 0.0f;
    for (float s : outputSamples) {
        maxSample = std::max(maxSample, std::abs(s));
    }
    REQUIRE(maxSample > 0.1f);

    float estimatedFreq = estimateFrequency(outputSamples, SAMPLE_RATE);

    INFO("Input frequency: " << INPUT_FREQ << " Hz");
    INFO("Expected output frequency: " << EXPECTED_OUTPUT_FREQ << " Hz");
    INFO("Actual output frequency: " << estimatedFreq << " Hz");

    // Output frequency should be HIGHER than input (shifted UP)
    REQUIRE(estimatedFreq > INPUT_FREQ);

    // Should be close to double the input (within 20% tolerance for pitch shifter imprecision)
    REQUIRE(estimatedFreq > EXPECTED_OUTPUT_FREQ * 0.8f);
    REQUIRE(estimatedFreq < EXPECTED_OUTPUT_FREQ * 1.2f);
}

TEST_CASE("PitchShiftNode - Low formantPreserve does not invert pitch direction", "[PitchShiftNode][direction]") {
    // Bug reproduction: With low formantPreserve values (like 0.3 for chipmunk),
    // the pitch shift UP was being perceived as DOWN because setFormantFactor()
    // is a frequency multiplier, not a 0-1 blend value.
    //
    // A formantPreserve of 0.3 was being passed directly to setFormantFactor(0.3),
    // which shifts formants DOWN by over an octave, potentially masking/inverting
    // the pitch shift perception.
    std::map<std::string, std::any> config = {
        {"pitchShift", 12.0f},       // Octave up
        {"formantPreserve", 0.3f}    // Low value (chipmunk preset uses this)
    };
    PitchShiftNode node(config);

    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    constexpr float INPUT_FREQ = 220.0f;  // A3

    auto outputSamples = processThroughPitchShifter(
        node, INPUT_FREQ, 0.5f, SAMPLE_RATE, 50, 15, BUFFER_SIZE);

    // Verify non-silent output
    float maxSample = 0.0f;
    for (float s : outputSamples) {
        maxSample = std::max(maxSample, std::abs(s));
    }
    INFO("Max output sample: " << maxSample);
    REQUIRE(maxSample > 0.05f);  // Should have some output

    float estimatedFreq = estimateFrequency(outputSamples, SAMPLE_RATE);

    INFO("Input frequency: " << INPUT_FREQ << " Hz");
    INFO("Output frequency: " << estimatedFreq << " Hz");
    INFO("formantPreserve: 0.3");

    // CRITICAL: Even with low formantPreserve, pitch shift +12 must result in
    // output frequency HIGHER than input (should be ~440 Hz, not lower)
    REQUIRE(estimatedFreq > INPUT_FREQ);
}

TEST_CASE("PitchShiftNode - Negative semitones shift frequency DOWN (villain)", "[PitchShiftNode][direction]") {
    // -12 semitones should halve the frequency (octave down)
    // Similar to deep villain preset
    std::map<std::string, std::any> config = {
        {"pitchShift", -12.0f},
        {"formantPreserve", 1.0f}
    };
    PitchShiftNode node(config);

    switchboard::AudioBusFormat inputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(node.setBusFormat(inputFormat, outputFormat));

    constexpr float INPUT_FREQ = 440.0f;  // A4
    constexpr float EXPECTED_OUTPUT_FREQ = 220.0f;  // A3 (octave down)

    auto outputSamples = processThroughPitchShifter(
        node, INPUT_FREQ, 0.5f, SAMPLE_RATE, 50, 15, BUFFER_SIZE);

    // Verify non-silent output
    float maxSample = 0.0f;
    for (float s : outputSamples) {
        maxSample = std::max(maxSample, std::abs(s));
    }
    REQUIRE(maxSample > 0.1f);

    float estimatedFreq = estimateFrequency(outputSamples, SAMPLE_RATE);

    INFO("Input frequency: " << INPUT_FREQ << " Hz");
    INFO("Expected output frequency: " << EXPECTED_OUTPUT_FREQ << " Hz");
    INFO("Actual output frequency: " << estimatedFreq << " Hz");

    // Output frequency should be LOWER than input (shifted DOWN)
    REQUIRE(estimatedFreq < INPUT_FREQ);

    // Should be close to half the input (within 20% tolerance)
    REQUIRE(estimatedFreq > EXPECTED_OUTPUT_FREQ * 0.8f);
    REQUIRE(estimatedFreq < EXPECTED_OUTPUT_FREQ * 1.2f);
}
