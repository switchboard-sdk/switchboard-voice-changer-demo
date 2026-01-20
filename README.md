# Voice Changer Demo

[![Voice Changer Demo](https://img.youtube.com/vi/VIDEO_ID_HERE/maxresdefault.jpg)](https://www.youtube.com/watch?v=VIDEO_ID_HERE)

Transform your voice in real-time with this demo application built on the [Switchboard SDK](https://switchboard.audio/). From deep villain to chipmunk to robot, explore 10 dramatically different voice presets that showcase the power of real-time audio processing.

## Build Voice AI and Audio Software Faster

The [Switchboard SDK](https://switchboard.audio/) lets you design, prototype, and deploy real-time audio features across any device or operating system. Built on a high-performance C++ core with simple higher-level language bindings, it delivers the audio processing power you need without the headaches.

This demo showcases just a fraction of what's possible:

- **Real-time processing** with ultra-low latency
- **Professional audio effects** including pitch shifting, ring modulation, chorus, flanger, vibrato, and delay
- **Formant preservation** for natural-sounding pitch shifts
- **Modular node-based architecture** for flexible audio routing
- **Cross-platform deployment** from a single codebase

[Get started with Switchboard SDK](https://console.switchboard.audio/) and build your own voice-enabled applications.

## Voice Presets

| # | Preset | Description |
|---|--------|-------------|
| 1 | Deep Villain | Menacing deep voice with echo |
| 2 | Chipmunk | High-pitched squeaky voice with vibrato |
| 3 | Robot | Metallic robotic voice |
| 4 | Alien | Warbling otherworldly voice |
| 5 | Monster | Deep growling beast voice |
| 6 | Radio | Classic warm radio announcer |
| 7 | Demon | Hellish demonic voice from the depths |
| 8 | Ghost | Ethereal haunting spectral voice |
| 9 | Giant | Massive thundering giant voice |
| 0 | Cyborg | Glitchy malfunctioning android |

## Requirements

- Linux (x86_64 or aarch64)
- CMake 3.16 or higher
- C++17 compatible compiler (GCC 8+ or Clang 7+)
- ALSA development libraries
- Python 3 with invoke (optional, for task runner)

### Installing System Dependencies

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install build-essential cmake libasound2-dev
```

**Fedora:**
```bash
sudo dnf install gcc-c++ cmake alsa-lib-devel
```

## Quick Start

```bash
git clone git@github.com:switchboard-sdk/switchboard-voice-changer-demo.git
cd switchboard-voice-changer-demo
pip install invoke
inv setup --release
```

This fetches all dependencies and builds the project automatically.

## Manual Setup

If you prefer to set up manually:

### 1. Clone the Repository

```bash
git clone git@github.com:switchboard-sdk/switchboard-voice-changer-demo.git
cd switchboard-voice-changer-demo
```

### 2. Download Dependencies

```bash
mkdir -p external
cd external
```

**Switchboard SDK:**
```bash
# For x86_64:
wget https://switchboard-sdk-public.s3.amazonaws.com/builds/release/3.1.0/linux/SwitchboardSDK.zip
unzip SwitchboardSDK.zip -d switchboard-sdk

# For aarch64:
wget https://switchboard-sdk-public.s3.amazonaws.com/builds/release/3.1.0/linux-aarch64/SwitchboardSDK.zip
unzip SwitchboardSDK.zip -d switchboard-sdk
```

**Switchboard Audio Effects:**
```bash
# For x86_64:
wget https://switchboard-sdk-public.s3.amazonaws.com/builds/release/3.1.0/linux/SwitchboardAudioEffects.zip
unzip SwitchboardAudioEffects.zip -d switchboard-effects

# For aarch64:
wget https://switchboard-sdk-public.s3.amazonaws.com/builds/release/3.1.0/linux-aarch64/SwitchboardAudioEffects.zip
unzip SwitchboardAudioEffects.zip -d switchboard-effects
```

**Signalsmith Stretch (pitch shifting):**
```bash
git clone git@github.com:Signalsmith-Audio/signalsmith-stretch.git
git clone git@github.com:Signalsmith-Audio/linear.git signalsmith-linear
```

**Catch2 (for tests):**
```bash
git clone --branch v3.4.0 --depth 1 git@github.com:catchorg/Catch2.git
```

Return to the project root:
```bash
cd ..
```

### 3. Build

```bash
mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

## Building with Invoke

```bash
pip install invoke
inv build --release
```

## Running the Demo

```bash
./build/voice-changer-demo
```

The application will:
1. Initialize the audio system
2. List available audio devices
3. Start capturing from your default microphone
4. Apply voice effects in real-time
5. Play the transformed audio through your default speakers

### Controls

| Key | Action |
|-----|--------|
| 1-9 | Select preset 1-9 |
| 0 | Select preset 10 (Cyborg) |
| Up Arrow | Previous preset |
| Down Arrow | Next preset |
| Q or Esc | Quit |

## Running Tests

```bash
# All tests
./build/VoiceChangerTests

# Or with invoke
inv test

# Specific test categories
inv test --filter="[baseline]"
inv test --filter="[PitchShiftNode]"
inv test --filter="[integration]"
```

## Project Structure

```
├── src/
│   ├── main.cpp                 # Demo application entry point
│   ├── extension/               # Switchboard extension registration
│   ├── nodes/                   # Custom audio processing nodes
│   │   ├── PitchShiftNode.*     # Pitch shifting with formant preservation
│   │   └── RingModNode.*        # Ring modulation for robotic effects
│   └── presets/
│       └── VoicePresets.hpp     # 10 built-in voice presets
├── tests/                       # Catch2 test files
├── test-assets/                 # Audio files for integration tests
├── external/                    # Downloaded dependencies (gitignored)
├── CMakeLists.txt
└── tasks.py                     # Invoke task definitions
```

## How It Works

The Switchboard SDK's node-based architecture makes it easy to build complex audio processing pipelines. This demo chains multiple effect nodes together:

```
Microphone → PitchShift → RingMod → Vibrato → Chorus → Flanger → Delay → Speakers
```

**Custom Nodes:**
- **PitchShiftNode**: High-quality pitch shifting with formant preservation using [signalsmith-stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch)
- **RingModNode**: Ring modulation for metallic and robotic effects

**Built-in Switchboard Audio Effects:**
- **Vibrato**: Pitch modulation for warbling effects
- **Chorus**: Thickens and widens the sound
- **Flanger**: Metallic sweeping effects
- **Delay**: Echo and spatial depth

The Switchboard SDK handles all the low-level audio I/O, buffer management, and real-time threading, so you can focus on building great audio features.

## Learn More

This demo is just one example of what you can build with the Switchboard SDK. Explore more possibilities:

- [Switchboard SDK Documentation](https://docs.switchboard.audio/)
- [More Example Apps](https://github.com/switchboard-sdk)
- [Switchboard Extensions](https://docs.switchboard.audio/extensions/)

Ready to build your own voice-enabled application? [Sign up for free](https://console.switchboard.audio/register) and start building today.

## License

MIT License. See [LICENSE](LICENSE) for details.

## Credits

Built with the [Switchboard SDK](https://switchboard.audio/) by [Synervoz](https://synervoz.com/).

Additional libraries:
- [signalsmith-stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch) by Signalsmith Audio (MIT License)
