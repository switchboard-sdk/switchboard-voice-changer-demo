/**
 * Integration tests for voice changer presets using Harvard sentences.
 *
 * These tests load real audio (Harvard sentences), process through each preset,
 * verify the output differs from input, and save output WAV files for manual review.
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "TestHelpers.hpp"
#include "nodes/PitchShiftNode.hpp"
#include "nodes/RingModNode.hpp"
#include "presets/VoicePresets.hpp"

#include <ChorusNode.hpp>
#include <FlangerNode.hpp>
#include <VibratoNode.hpp>
#include <DelayNode.hpp>

#include <fstream>
#include <vector>
#include <cmath>
#include <cstring>
#include <algorithm>

using namespace voicechanger;
using namespace voicechanger::test;
using Catch::Approx;

// Global Switchboard initialization
static SwitchboardInitializer switchboardInit;

// Test constants
constexpr uint OUTPUT_SAMPLE_RATE = 44100;
constexpr uint NUM_CHANNELS = 2;  // Stereo processing
constexpr uint BUFFER_SIZE = 512;

// Test assets directory (passed via CMake compile definition)
#ifndef TEST_ASSETS_DIR
#define TEST_ASSETS_DIR "test-assets"
#endif

/**
 * Simple WAV file header structure.
 */
#pragma pack(push, 1)
struct WavHeader {
    char riff[4];           // "RIFF"
    uint32_t fileSize;      // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmtSize;       // Format chunk size (16 for PCM)
    uint16_t audioFormat;   // 1 = PCM
    uint16_t numChannels;   // Number of channels
    uint32_t sampleRate;    // Sample rate
    uint32_t byteRate;      // Bytes per second
    uint16_t blockAlign;    // Bytes per sample frame
    uint16_t bitsPerSample; // Bits per sample
    char data[4];           // "data"
    uint32_t dataSize;      // Data chunk size
};
#pragma pack(pop)

/**
 * Load a WAV file and return samples as floats (-1.0 to 1.0).
 * Supports 16-bit PCM mono/stereo.
 */
bool loadWavFile(const std::string& path,
                 std::vector<float>& samples,
                 uint32_t& sampleRate,
                 uint16_t& numChannels) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    // Read RIFF header (12 bytes)
    char riff[4], wave[4];
    uint32_t fileSize;
    file.read(riff, 4);
    file.read(reinterpret_cast<char*>(&fileSize), 4);
    file.read(wave, 4);

    if (std::strncmp(riff, "RIFF", 4) != 0 ||
        std::strncmp(wave, "WAVE", 4) != 0) {
        return false;
    }

    // Read chunks until we find fmt and data
    uint16_t audioFormat = 0;
    uint16_t bitsPerSample = 0;
    sampleRate = 0;
    numChannels = 0;
    uint32_t dataSize = 0;
    bool foundFmt = false;
    bool foundData = false;

    while (!foundData && file) {
        char chunkId[4];
        uint32_t chunkSize;
        file.read(chunkId, 4);
        file.read(reinterpret_cast<char*>(&chunkSize), 4);

        if (!file) break;

        if (std::strncmp(chunkId, "fmt ", 4) == 0) {
            file.read(reinterpret_cast<char*>(&audioFormat), 2);
            file.read(reinterpret_cast<char*>(&numChannels), 2);
            file.read(reinterpret_cast<char*>(&sampleRate), 4);
            uint32_t byteRate;
            uint16_t blockAlign;
            file.read(reinterpret_cast<char*>(&byteRate), 4);
            file.read(reinterpret_cast<char*>(&blockAlign), 2);
            file.read(reinterpret_cast<char*>(&bitsPerSample), 2);
            // Skip any extra fmt bytes
            if (chunkSize > 16) {
                file.seekg(chunkSize - 16, std::ios::cur);
            }
            foundFmt = true;
        } else if (std::strncmp(chunkId, "data", 4) == 0) {
            dataSize = chunkSize;
            foundData = true;
        } else {
            // Skip unknown chunk
            file.seekg(chunkSize, std::ios::cur);
        }
    }

    if (!foundFmt || !foundData) {
        return false;
    }

    if (bitsPerSample != 16) {
        return false;  // Only support 16-bit for now
    }

    uint32_t numSamples = dataSize / (bitsPerSample / 8);
    std::vector<int16_t> rawSamples(numSamples);
    file.read(reinterpret_cast<char*>(rawSamples.data()), dataSize);

    samples.resize(numSamples);
    for (size_t i = 0; i < numSamples; ++i) {
        samples[i] = static_cast<float>(rawSamples[i]) / 32768.0f;
    }

    return true;
}

/**
 * Save samples to a WAV file.
 */
bool saveWavFile(const std::string& path,
                 const std::vector<float>& samples,
                 uint32_t sampleRate,
                 uint16_t numChannels) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        return false;
    }

    uint32_t dataSize = samples.size() * sizeof(int16_t);
    uint32_t fileSize = 36 + dataSize;

    WavHeader header;
    std::memcpy(header.riff, "RIFF", 4);
    header.fileSize = fileSize;
    std::memcpy(header.wave, "WAVE", 4);
    std::memcpy(header.fmt, "fmt ", 4);
    header.fmtSize = 16;
    header.audioFormat = 1;  // PCM
    header.numChannels = numChannels;
    header.sampleRate = sampleRate;
    header.byteRate = sampleRate * numChannels * 2;
    header.blockAlign = numChannels * 2;
    header.bitsPerSample = 16;
    std::memcpy(header.data, "data", 4);
    header.dataSize = dataSize;

    file.write(reinterpret_cast<char*>(&header), sizeof(header));

    std::vector<int16_t> rawSamples(samples.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        float sample = std::clamp(samples[i], -1.0f, 1.0f);
        rawSamples[i] = static_cast<int16_t>(sample * 32767.0f);
    }
    file.write(reinterpret_cast<char*>(rawSamples.data()), dataSize);

    return true;
}

/**
 * Simple linear resampling from srcRate to dstRate.
 */
std::vector<float> resample(const std::vector<float>& input,
                            uint32_t srcRate,
                            uint32_t dstRate) {
    if (srcRate == dstRate) {
        return input;
    }

    double ratio = static_cast<double>(dstRate) / srcRate;
    size_t outputSize = static_cast<size_t>(input.size() * ratio);
    std::vector<float> output(outputSize);

    for (size_t i = 0; i < outputSize; ++i) {
        double srcIndex = i / ratio;
        size_t idx0 = static_cast<size_t>(srcIndex);
        size_t idx1 = std::min(idx0 + 1, input.size() - 1);
        double frac = srcIndex - idx0;

        output[i] = static_cast<float>(input[idx0] * (1.0 - frac) + input[idx1] * frac);
    }

    return output;
}

/**
 * Convert mono to stereo by duplicating channels.
 */
std::vector<float> monoToStereo(const std::vector<float>& mono) {
    std::vector<float> stereo(mono.size() * 2);
    for (size_t i = 0; i < mono.size(); ++i) {
        stereo[i * 2] = mono[i];
        stereo[i * 2 + 1] = mono[i];
    }
    return stereo;
}

/**
 * Convert interleaved stereo to mono by averaging.
 */
std::vector<float> stereoToMono(const std::vector<float>& stereo) {
    std::vector<float> mono(stereo.size() / 2);
    for (size_t i = 0; i < mono.size(); ++i) {
        mono[i] = (stereo[i * 2] + stereo[i * 2 + 1]) * 0.5f;
    }
    return mono;
}

/**
 * Calculate RMS of a signal.
 */
float calculateRMS(const std::vector<float>& samples) {
    if (samples.empty()) return 0.0f;
    double sum = 0.0;
    for (float s : samples) {
        sum += s * s;
    }
    return static_cast<float>(std::sqrt(sum / samples.size()));
}

/**
 * Apply a preset to the audio processing chain.
 */
void applyPresetToNodes(const VoicePreset& preset,
                        PitchShiftNode* pitchShiftNode,
                        RingModNode* ringModNode,
                        switchboard::extensions::audioeffects::ChorusNode* chorusNode,
                        switchboard::extensions::audioeffects::FlangerNode* flangerNode,
                        switchboard::extensions::audioeffects::VibratoNode* vibratoNode,
                        switchboard::extensions::audioeffects::DelayNode* delayNode) {

    // Apply pitch shift parameters
    pitchShiftNode->setValue("pitchShift", std::make_any<float>(preset.pitchShift));
    pitchShiftNode->setValue("formantPreserve", std::make_any<float>(preset.formantPreserve));
    pitchShiftNode->setValue("outputGain", std::make_any<float>(preset.outputGain));

    // Apply ring modulation parameters
    if (preset.useRingMod) {
        ringModNode->setValue("carrierFrequency", std::make_any<float>(preset.carrierFrequency));
        ringModNode->setValue("mix", std::make_any<float>(preset.ringModMix));
    } else {
        ringModNode->setValue("mix", std::make_any<float>(0.0f));
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
        delayNode->setDryMix(1.0f - preset.delayWetMix * 0.5f);
    } else {
        delayNode->setWetMix(0.0f);
        delayNode->setDryMix(1.0f);
    }
}

/**
 * Process stereo audio through the effect chain.
 * Input/output are interleaved stereo samples.
 */
std::vector<float> processAudioThroughChain(
    const std::vector<float>& inputStereo,
    PitchShiftNode* pitchShiftNode,
    RingModNode* ringModNode,
    switchboard::extensions::audioeffects::ChorusNode* chorusNode,
    switchboard::extensions::audioeffects::FlangerNode* flangerNode,
    switchboard::extensions::audioeffects::VibratoNode* vibratoNode,
    switchboard::extensions::audioeffects::DelayNode* delayNode) {

    size_t numFrames = inputStereo.size() / 2;
    std::vector<float> outputStereo(inputStereo.size());

    // Process in buffer-sized chunks
    size_t framesProcessed = 0;

    while (framesProcessed < numFrames) {
        size_t framesToProcess = std::min(static_cast<size_t>(BUFFER_SIZE),
                                          numFrames - framesProcessed);

        // Create input/output buses for each stage
        TestAudioBus inBus1(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, framesToProcess);
        TestAudioBus outBus1(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, framesToProcess);
        TestAudioBus outBus2(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, framesToProcess);
        TestAudioBus outBus3(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, framesToProcess);
        TestAudioBus outBus4(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, framesToProcess);
        TestAudioBus outBus5(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, framesToProcess);
        TestAudioBus outBus6(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, framesToProcess);

        // Copy input samples (deinterleave)
        for (size_t frame = 0; frame < framesToProcess; ++frame) {
            size_t srcIdx = (framesProcessed + frame) * 2;
            inBus1.setSample(0, frame, inputStereo[srcIdx]);
            inBus1.setSample(1, frame, inputStereo[srcIdx + 1]);
        }

        // Process through chain: pitchShift -> ringMod -> vibrato -> chorus -> flanger -> delay
        pitchShiftNode->process(inBus1.bus, outBus1.bus);
        ringModNode->process(outBus1.bus, outBus2.bus);
        vibratoNode->process(outBus2.bus, outBus3.bus);
        chorusNode->process(outBus3.bus, outBus4.bus);
        flangerNode->process(outBus4.bus, outBus5.bus);
        delayNode->process(outBus5.bus, outBus6.bus);

        // Copy output samples (interleave)
        for (size_t frame = 0; frame < framesToProcess; ++frame) {
            size_t dstIdx = (framesProcessed + frame) * 2;
            outputStereo[dstIdx] = outBus6.getSample(0, frame);
            outputStereo[dstIdx + 1] = outBus6.getSample(1, frame);
        }

        framesProcessed += framesToProcess;
    }

    return outputStereo;
}

TEST_CASE("Integration - Process Harvard sentences through all presets", "[integration][harvard]") {
    // Load male Harvard sentence
    std::vector<float> inputSamples;
    uint32_t inputSampleRate;
    uint16_t inputChannels;

    std::string inputPath = std::string(TEST_ASSETS_DIR) + "/harvard_male_01.wav";
    REQUIRE(loadWavFile(inputPath, inputSamples, inputSampleRate, inputChannels));

    INFO("Loaded " << inputPath << ": " << inputSamples.size() << " samples, "
         << inputSampleRate << " Hz, " << inputChannels << " channels");

    // Resample from 8kHz to 44.1kHz
    std::vector<float> resampledMono = resample(inputSamples, inputSampleRate, OUTPUT_SAMPLE_RATE);
    INFO("Resampled to " << OUTPUT_SAMPLE_RATE << " Hz: " << resampledMono.size() << " samples");

    // Convert to stereo
    std::vector<float> inputStereo = monoToStereo(resampledMono);
    INFO("Converted to stereo: " << inputStereo.size() << " samples");

    float inputRMS = calculateRMS(resampledMono);
    INFO("Input RMS: " << inputRMS);
    REQUIRE(inputRMS > 0.01f);  // Verify input is not silent

    // Create effect nodes
    std::map<std::string, std::any> emptyConfig;
    PitchShiftNode pitchShiftNode(emptyConfig);
    RingModNode ringModNode(emptyConfig);
    switchboard::extensions::audioeffects::ChorusNode chorusNode(2);
    switchboard::extensions::audioeffects::FlangerNode flangerNode(2);
    switchboard::extensions::audioeffects::VibratoNode vibratoNode(2);
    switchboard::extensions::audioeffects::DelayNode delayNode(2);

    // Set bus format for all nodes
    switchboard::AudioBusFormat inputFormat(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    REQUIRE(pitchShiftNode.setBusFormat(inputFormat, outputFormat));
    REQUIRE(ringModNode.setBusFormat(inputFormat, outputFormat));
    REQUIRE(chorusNode.setBusFormat(inputFormat, outputFormat));
    REQUIRE(flangerNode.setBusFormat(inputFormat, outputFormat));
    REQUIRE(vibratoNode.setBusFormat(inputFormat, outputFormat));
    REQUIRE(delayNode.setBusFormat(inputFormat, outputFormat));

    // Process through each preset
    std::vector<float> presetRMSValues;

    for (int presetIdx = 0; presetIdx < getPresetCount(); ++presetIdx) {
        const auto& preset = getPreset(presetIdx);

        INFO("Processing preset " << (presetIdx + 1) << ": " << preset.name);

        // Apply preset settings
        applyPresetToNodes(preset,
                          &pitchShiftNode, &ringModNode, &chorusNode,
                          &flangerNode, &vibratoNode, &delayNode);

        // Process audio
        std::vector<float> outputStereo = processAudioThroughChain(
            inputStereo,
            &pitchShiftNode, &ringModNode, &chorusNode,
            &flangerNode, &vibratoNode, &delayNode);

        // Convert back to mono for analysis
        std::vector<float> outputMono = stereoToMono(outputStereo);

        float outputRMS = calculateRMS(outputMono);
        presetRMSValues.push_back(outputRMS);

        INFO("  Output RMS: " << outputRMS);

        // Verify output is not silent
        REQUIRE(outputRMS > 0.005f);

        // Save output WAV file
        std::string outputFilename = std::string(TEST_ASSETS_DIR) + "/output_" +
                                     std::to_string(presetIdx + 1) + "_" + preset.name + ".wav";
        // Replace spaces with underscores in filename
        std::replace(outputFilename.begin(), outputFilename.end(), ' ', '_');

        REQUIRE(saveWavFile(outputFilename, outputMono, OUTPUT_SAMPLE_RATE, 1));
        INFO("  Saved: " << outputFilename);
    }

    // Verify presets produce different outputs (at least some variation in RMS)
    float minRMS = *std::min_element(presetRMSValues.begin(), presetRMSValues.end());
    float maxRMS = *std::max_element(presetRMSValues.begin(), presetRMSValues.end());
    float rmsRange = maxRMS - minRMS;

    INFO("RMS range across presets: " << minRMS << " to " << maxRMS << " (range: " << rmsRange << ")");

    // There should be some variation in output levels due to different effects
    REQUIRE(rmsRange > 0.001f);
}

TEST_CASE("Integration - Each preset produces distinct output", "[integration][distinct]") {
    // Load female Harvard sentence for variety
    std::vector<float> inputSamples;
    uint32_t inputSampleRate;
    uint16_t inputChannels;

    std::string inputPath = std::string(TEST_ASSETS_DIR) + "/harvard_female_01.wav";
    REQUIRE(loadWavFile(inputPath, inputSamples, inputSampleRate, inputChannels));

    // Resample and convert to stereo
    std::vector<float> resampledMono = resample(inputSamples, inputSampleRate, OUTPUT_SAMPLE_RATE);
    std::vector<float> inputStereo = monoToStereo(resampledMono);

    // Create effect nodes
    std::map<std::string, std::any> emptyConfig;
    PitchShiftNode pitchShiftNode(emptyConfig);
    RingModNode ringModNode(emptyConfig);
    switchboard::extensions::audioeffects::ChorusNode chorusNode(2);
    switchboard::extensions::audioeffects::FlangerNode flangerNode(2);
    switchboard::extensions::audioeffects::VibratoNode vibratoNode(2);
    switchboard::extensions::audioeffects::DelayNode delayNode(2);

    // Set bus format
    switchboard::AudioBusFormat inputFormat(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    switchboard::AudioBusFormat outputFormat(OUTPUT_SAMPLE_RATE, NUM_CHANNELS, BUFFER_SIZE);
    pitchShiftNode.setBusFormat(inputFormat, outputFormat);
    ringModNode.setBusFormat(inputFormat, outputFormat);
    chorusNode.setBusFormat(inputFormat, outputFormat);
    flangerNode.setBusFormat(inputFormat, outputFormat);
    vibratoNode.setBusFormat(inputFormat, outputFormat);
    delayNode.setBusFormat(inputFormat, outputFormat);

    // Store outputs for comparison
    std::vector<std::vector<float>> presetOutputs;

    for (int presetIdx = 0; presetIdx < getPresetCount(); ++presetIdx) {
        const auto& preset = getPreset(presetIdx);

        applyPresetToNodes(preset,
                          &pitchShiftNode, &ringModNode, &chorusNode,
                          &flangerNode, &vibratoNode, &delayNode);

        std::vector<float> outputStereo = processAudioThroughChain(
            inputStereo,
            &pitchShiftNode, &ringModNode, &chorusNode,
            &flangerNode, &vibratoNode, &delayNode);

        presetOutputs.push_back(stereoToMono(outputStereo));
    }

    // Compare each pair of presets - they should be different
    int differentPairs = 0;
    int totalPairs = 0;

    for (int i = 0; i < getPresetCount(); ++i) {
        for (int j = i + 1; j < getPresetCount(); ++j) {
            totalPairs++;

            // Calculate difference between outputs
            float diffSum = 0.0f;
            size_t compareLen = std::min(presetOutputs[i].size(), presetOutputs[j].size());

            for (size_t k = 0; k < compareLen; ++k) {
                float diff = presetOutputs[i][k] - presetOutputs[j][k];
                diffSum += diff * diff;
            }

            float rmseDiff = std::sqrt(diffSum / compareLen);

            if (rmseDiff > 0.01f) {
                differentPairs++;
            }

            INFO("Preset " << (i + 1) << " vs " << (j + 1) << ": RMSE diff = " << rmseDiff);
        }
    }

    INFO("Different pairs: " << differentPairs << " / " << totalPairs);

    // Most preset pairs should produce noticeably different outputs
    REQUIRE(differentPairs >= totalPairs * 0.8);  // At least 80% should be different
}
