#pragma once

#include <array>
#include <string>

namespace voicechanger {

/**
 * Voice preset configuration.
 * Each preset can combine PitchShift, RingMod, Chorus, Flanger, Vibrato, and Delay.
 */
struct VoicePreset {
    std::string name;
    std::string description;

    // PitchShift parameters
    float pitchShift;       // Semitones (-24 to +24)
    float formantPreserve;  // 0.0 to 1.0 (0=chipmunk effect, 1=natural voice)

    // RingMod parameters
    bool useRingMod;
    float carrierFrequency; // Hz (10 to 1000)
    float ringModMix;       // 0.0 to 1.0

    // Chorus parameters
    bool useChorus;
    float chorusSweepWidth; // Sweep width (0.001 to 0.05)
    float chorusFrequency;  // LFO frequency Hz (0.1 to 5.0)

    // Flanger parameters
    bool useFlanger;
    float flangerSweepWidth; // Sweep width (0.001 to 0.02)
    float flangerFrequency;  // LFO frequency Hz (0.1 to 2.0)

    // Vibrato parameters
    bool useVibrato;
    float vibratoSweepWidth; // Sweep width (0.001 to 0.01)
    float vibratoFrequency;  // LFO frequency Hz (2.0 to 10.0)

    // Delay parameters
    bool useDelay;
    uint delayMs;           // Delay time in ms (10 to 500)
    float delayFeedback;    // Feedback level (0.0 to 0.9)
    float delayWetMix;      // Wet mix (0.0 to 1.0)

    // Output
    float outputGain;       // 0.0 to 4.0
};

/**
 * Built-in voice presets (10 dramatically different voices).
 * Access via getPresets() or by index (0-9).
 */
inline const std::array<VoicePreset, 10>& getPresets() {
    static const std::array<VoicePreset, 10> presets = {{
        // 1. Deep Villain - Menacing deep voice with subtle delay
        {
            "Deep Villain",
            "Menacing deep voice with echo",
            -8.0f,    // pitchShift (deep)
            1.0f,     // formantPreserve (natural sounding deep)
            false,    // useRingMod
            0.0f,     // carrierFrequency
            0.0f,     // ringModMix
            false,    // useChorus
            0.0f,     // chorusSweepWidth
            0.0f,     // chorusFrequency
            false,    // useFlanger
            0.0f,     // flangerSweepWidth
            0.0f,     // flangerFrequency
            false,    // useVibrato
            0.0f,     // vibratoSweepWidth
            0.0f,     // vibratoFrequency
            true,     // useDelay (dramatic echo)
            180,      // delayMs (medium delay for drama)
            0.35f,    // delayFeedback
            0.3f,     // delayWetMix
            1.3f      // outputGain
        },

        // 2. Chipmunk - Classic high-pitched squeaky voice
        {
            "Chipmunk",
            "High-pitched squeaky voice with vibrato",
            12.0f,    // pitchShift (one octave up)
            0.0f,     // formantPreserve (none - classic chipmunk)
            false,    // useRingMod
            0.0f,     // carrierFrequency
            0.0f,     // ringModMix
            false,    // useChorus
            0.0f,     // chorusSweepWidth
            0.0f,     // chorusFrequency
            false,    // useFlanger
            0.0f,     // flangerSweepWidth
            0.0f,     // flangerFrequency
            true,     // useVibrato (adds character)
            0.003f,   // vibratoSweepWidth
            6.0f,     // vibratoFrequency (fast vibrato)
            false,    // useDelay
            0,        // delayMs
            0.0f,     // delayFeedback
            0.0f,     // delayWetMix
            1.1f      // outputGain
        },

        // 3. Robot - Metallic robotic voice with flanger
        {
            "Robot",
            "Metallic robotic voice",
            0.0f,     // pitchShift (none)
            1.0f,     // formantPreserve
            true,     // useRingMod (key to robot sound)
            180.0f,   // carrierFrequency (mid-high for metallic)
            0.65f,    // ringModMix (strong)
            false,    // useChorus
            0.0f,     // chorusSweepWidth
            0.0f,     // chorusFrequency
            true,     // useFlanger (adds metallic sweep)
            0.008f,   // flangerSweepWidth
            0.3f,     // flangerFrequency (slow sweep)
            false,    // useVibrato
            0.0f,     // vibratoSweepWidth
            0.0f,     // vibratoFrequency
            false,    // useDelay
            0,        // delayMs
            0.0f,     // delayFeedback
            0.0f,     // delayWetMix
            1.4f      // outputGain
        },

        // 4. Alien - Otherworldly warbling voice
        {
            "Alien",
            "Warbling otherworldly voice",
            5.0f,     // pitchShift (higher)
            0.5f,     // formantPreserve (partial - unusual timbre)
            true,     // useRingMod
            65.0f,    // carrierFrequency (creates warble)
            0.5f,     // ringModMix
            false,    // useChorus
            0.0f,     // chorusSweepWidth
            0.0f,     // chorusFrequency
            false,    // useFlanger
            0.0f,     // flangerSweepWidth
            0.0f,     // flangerFrequency
            true,     // useVibrato (adds alien warble)
            0.006f,   // vibratoSweepWidth (noticeable)
            4.5f,     // vibratoFrequency (medium-fast)
            true,     // useDelay (spacey echo)
            120,      // delayMs
            0.45f,    // delayFeedback (multiple echoes)
            0.35f,    // delayWetMix
            1.3f      // outputGain
        },

        // 5. Monster - Deep growling beast
        {
            "Monster",
            "Deep growling beast voice",
            -14.0f,   // pitchShift (very deep)
            0.85f,    // formantPreserve (mostly natural)
            true,     // useRingMod (adds growl)
            25.0f,    // carrierFrequency (very low - rumble)
            0.4f,     // ringModMix
            true,     // useChorus (thickens sound)
            0.02f,    // chorusSweepWidth
            0.4f,     // chorusFrequency
            false,    // useFlanger
            0.0f,     // flangerSweepWidth
            0.0f,     // flangerFrequency
            false,    // useVibrato
            0.0f,     // vibratoSweepWidth
            0.0f,     // vibratoFrequency
            false,    // useDelay
            0,        // delayMs
            0.0f,     // delayFeedback
            0.0f,     // delayWetMix
            1.6f      // outputGain (boost low frequencies)
        },

        // 6. Radio Announcer - Warm broadcast voice
        {
            "Radio",
            "Classic warm radio announcer",
            -2.0f,    // pitchShift (slightly deeper = authority)
            0.9f,     // formantPreserve (natural)
            false,    // useRingMod
            0.0f,     // carrierFrequency
            0.0f,     // ringModMix
            true,     // useChorus (slight warmth/thickness)
            0.008f,   // chorusSweepWidth (subtle)
            0.25f,    // chorusFrequency (slow)
            false,    // useFlanger
            0.0f,     // flangerSweepWidth
            0.0f,     // flangerFrequency
            false,    // useVibrato
            0.0f,     // vibratoSweepWidth
            0.0f,     // vibratoFrequency
            true,     // useDelay (room ambience)
            45,       // delayMs (short - room reflection)
            0.15f,    // delayFeedback (minimal)
            0.2f,     // delayWetMix (subtle)
            1.15f     // outputGain
        },

        // 7. Demon - Hellish layered voice
        {
            "Demon",
            "Hellish demonic voice from the depths",
            -10.0f,   // pitchShift (deep)
            0.75f,    // formantPreserve
            true,     // useRingMod (evil distortion)
            35.0f,    // carrierFrequency (ominous)
            0.35f,    // ringModMix
            true,     // useChorus (multiple voices)
            0.025f,   // chorusSweepWidth (wide)
            0.6f,     // chorusFrequency
            true,     // useFlanger (adds menace)
            0.012f,   // flangerSweepWidth
            0.15f,    // flangerFrequency (slow creepy sweep)
            false,    // useVibrato
            0.0f,     // vibratoSweepWidth
            0.0f,     // vibratoFrequency
            true,     // useDelay (hellish reverb)
            250,      // delayMs (long)
            0.5f,     // delayFeedback (lots of echoes)
            0.4f,     // delayWetMix
            1.5f      // outputGain
        },

        // 8. Ghost - Ethereal haunting whisper
        {
            "Ghost",
            "Ethereal haunting spectral voice",
            4.0f,     // pitchShift (slightly higher - otherworldly)
            0.4f,     // formantPreserve (unusual timbre)
            false,    // useRingMod
            0.0f,     // carrierFrequency
            0.0f,     // ringModMix
            true,     // useChorus (ethereal shimmer)
            0.03f,    // chorusSweepWidth (wide)
            1.2f,     // chorusFrequency (fast shimmer)
            true,     // useFlanger (ghostly sweep)
            0.015f,   // flangerSweepWidth
            0.2f,     // flangerFrequency
            true,     // useVibrato (wavering)
            0.004f,   // vibratoSweepWidth
            3.0f,     // vibratoFrequency
            true,     // useDelay (lots of reverb/echo)
            200,      // delayMs
            0.6f,     // delayFeedback (long decay)
            0.5f,     // delayWetMix (very wet)
            1.2f      // outputGain
        },

        // 9. Giant - Massive thundering voice
        {
            "Giant",
            "Massive thundering giant voice",
            -18.0f,   // pitchShift (extremely deep)
            0.95f,    // formantPreserve (keep natural character)
            false,    // useRingMod
            0.0f,     // carrierFrequency
            0.0f,     // ringModMix
            true,     // useChorus (massive/wide)
            0.015f,   // chorusSweepWidth
            0.3f,     // chorusFrequency
            false,    // useFlanger
            0.0f,     // flangerSweepWidth
            0.0f,     // flangerFrequency
            false,    // useVibrato
            0.0f,     // vibratoSweepWidth
            0.0f,     // vibratoFrequency
            true,     // useDelay (booming echo)
            300,      // delayMs (long for size)
            0.4f,     // delayFeedback
            0.35f,    // delayWetMix
            1.8f      // outputGain (compensate for deep pitch)
        },

        // 10. Cyborg - Glitchy electronic voice
        {
            "Cyborg",
            "Glitchy malfunctioning android",
            -4.0f,    // pitchShift (slightly deeper)
            0.6f,     // formantPreserve
            true,     // useRingMod (electronic buzz)
            220.0f,   // carrierFrequency (high - buzzy/digital)
            0.55f,    // ringModMix (strong electronic)
            false,    // useChorus
            0.0f,     // chorusSweepWidth
            0.0f,     // chorusFrequency
            true,     // useFlanger (digital sweep)
            0.01f,    // flangerSweepWidth
            0.8f,     // flangerFrequency (faster - glitchy)
            true,     // useVibrato (unstable pitch)
            0.005f,   // vibratoSweepWidth
            7.0f,     // vibratoFrequency (fast - glitchy)
            true,     // useDelay (digital echo)
            80,       // delayMs (short - stutter effect)
            0.55f,    // delayFeedback
            0.4f,     // delayWetMix
            1.35f     // outputGain
        }
    }};

    return presets;
}

/**
 * Get preset by index (0-9).
 * Returns preset 0 (Deep Villain) if index is out of range.
 */
inline const VoicePreset& getPreset(int index) {
    const auto& presets = getPresets();
    if (index < 0 || index >= static_cast<int>(presets.size())) {
        return presets[0];
    }
    return presets[index];
}

/**
 * Get the number of presets available.
 */
inline constexpr int getPresetCount() {
    return 10;
}

} // namespace voicechanger
