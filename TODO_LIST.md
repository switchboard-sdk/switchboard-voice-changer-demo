# Voice Changer Demo - TODO List

## Phase 1: Project Setup
- [x] Initialize git repository
- [x] Clone signalsmith-stretch
- [x] Clone signalsmith-linear
- [x] Clone Catch2
- [x] Download Switchboard SDK
- [x] Download SwitchboardAudioEffects
- [x] Download Harvard Sentences test audio
- [x] Create CMakeLists.txt
- [x] Create tasks.py (pyinvoke)
- [x] Verify build system compiles

## Phase 2: Test Infrastructure (TDD)
- [x] Create TestHelpers.hpp
- [x] Write PitchShiftNode baseline tests (red phase)
- [x] Write RingModNode baseline tests (red phase)

## Phase 3: Core Audio Nodes
- [x] Implement PitchShiftNode (green phase)
- [x] Implement RingModNode (green phase)
- [x] All baseline tests pass (17 tests passing)

## Phase 4: Extension Registration
- [x] Create VoiceChangerNodeFactory
- [x] Create VoiceChangerExtension
- [x] Verify extension loads in Switchboard

## Phase 5: Preset System
- [x] Define VoicePresets.hpp with 10 presets

## Phase 6: Console Application
- [x] Create main.cpp with audio graph
- [x] Implement keyboard controls (1-9, 0, arrows)
- [x] Display current preset name
- [ ] Test with live microphone

## Phase 7: Integration Tests
- [ ] Load and resample Harvard Sentences
- [ ] Process through all presets
- [ ] Verify distinct output per preset

## Voice Presets (All Implemented)
1. [x] Deep Villain (-8 semitones, formant preserved)
2. [x] Chipmunk (+12 semitones, low formant)
3. [x] Robot (ring mod 150Hz)
4. [x] Alien (pitch +5, ring mod 80Hz)
5. [x] Monster (pitch -12, ring mod 30Hz)
6. [x] Radio DJ (pitch +2, formant 0.8)
7. [x] Demon (pitch -10, ring mod, chorus)
8. [x] Ghost (pitch +3, heavy chorus)
9. [x] Giant (pitch -15, formant 0.9)
10. [x] Cyberpunk (pitch -3, ring mod 200Hz)

## Build Instructions

```bash
# Build
inv build

# Run tests
inv test

# Run the demo
inv run
# Or directly: ./build/voice-changer-demo
```
