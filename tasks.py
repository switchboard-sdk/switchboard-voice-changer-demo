"""
Invoke tasks for Voice Changer Demo project.

Usage:
    inv setup       - Fetch dependencies and build (fresh checkout)
    inv fetch-deps  - Download all external dependencies
    inv build       - Build the project
    inv test        - Run all tests
    inv run         - Run the demo application
    inv clean       - Clean build artifacts
    inv rebuild     - Clean and rebuild
"""

from invoke import task
import os
import platform
import shutil

BUILD_DIR = "build"
EXTERNAL_DIR = "external"
SDK_VERSION = "3.1.0"


def get_arch():
    """Detect system architecture."""
    machine = platform.machine().lower()
    if machine in ("x86_64", "amd64"):
        return "x86_64"
    elif machine in ("aarch64", "arm64"):
        return "aarch64"
    else:
        raise RuntimeError(f"Unsupported architecture: {machine}")


def get_sdk_url(arch):
    """Get Switchboard SDK download URL for architecture."""
    if arch == "x86_64":
        return f"https://switchboard-sdk-public.s3.amazonaws.com/builds/release/{SDK_VERSION}/linux/SwitchboardSDK.zip"
    else:
        return f"https://switchboard-sdk-public.s3.amazonaws.com/builds/release/{SDK_VERSION}/linux-aarch64/SwitchboardSDK.zip"


def get_effects_url(arch):
    """Get Switchboard Audio Effects download URL for architecture."""
    if arch == "x86_64":
        return f"https://switchboard-sdk-public.s3.amazonaws.com/builds/release/{SDK_VERSION}/linux/SwitchboardAudioEffects.zip"
    else:
        return f"https://switchboard-sdk-public.s3.amazonaws.com/builds/release/{SDK_VERSION}/linux-aarch64/SwitchboardAudioEffects.zip"


@task
def fetch_deps(c):
    """Download all external dependencies."""
    arch = get_arch()
    print(f"Detected architecture: {arch}")

    os.makedirs(EXTERNAL_DIR, exist_ok=True)

    # Switchboard SDK
    sdk_dir = os.path.join(EXTERNAL_DIR, "switchboard-sdk")
    if not os.path.exists(sdk_dir):
        print("Downloading Switchboard SDK...")
        sdk_url = get_sdk_url(arch)
        c.run(f"wget -q --show-progress -O /tmp/SwitchboardSDK.zip '{sdk_url}'")
        c.run(f"unzip -q /tmp/SwitchboardSDK.zip -d {sdk_dir}")
        c.run("rm /tmp/SwitchboardSDK.zip")
    else:
        print("Switchboard SDK already exists, skipping...")

    # Switchboard Audio Effects
    effects_dir = os.path.join(EXTERNAL_DIR, "switchboard-effects")
    if not os.path.exists(effects_dir):
        print("Downloading Switchboard Audio Effects...")
        effects_url = get_effects_url(arch)
        c.run(f"wget -q --show-progress -O /tmp/SwitchboardAudioEffects.zip '{effects_url}'")
        c.run(f"unzip -q /tmp/SwitchboardAudioEffects.zip -d {effects_dir}")
        c.run("rm /tmp/SwitchboardAudioEffects.zip")
    else:
        print("Switchboard Audio Effects already exists, skipping...")

    # Signalsmith Stretch
    stretch_dir = os.path.join(EXTERNAL_DIR, "signalsmith-stretch")
    if not os.path.exists(stretch_dir):
        print("Cloning signalsmith-stretch...")
        c.run(f"git clone --depth 1 https://github.com/Signalsmith-Audio/signalsmith-stretch.git {stretch_dir}")
    else:
        print("signalsmith-stretch already exists, skipping...")

    # Signalsmith Linear
    linear_dir = os.path.join(EXTERNAL_DIR, "signalsmith-linear")
    if not os.path.exists(linear_dir):
        print("Cloning signalsmith-linear...")
        c.run(f"git clone --depth 1 https://github.com/Signalsmith-Audio/linear.git {linear_dir}")
    else:
        print("signalsmith-linear already exists, skipping...")

    # Catch2
    catch2_dir = os.path.join(EXTERNAL_DIR, "Catch2")
    if not os.path.exists(catch2_dir):
        print("Cloning Catch2...")
        c.run(f"git clone --branch v3.4.0 --depth 1 https://github.com/catchorg/Catch2.git {catch2_dir}")
    else:
        print("Catch2 already exists, skipping...")

    print("All dependencies fetched successfully!")


@task
def clean_deps(c):
    """Remove all external dependencies."""
    if os.path.exists(EXTERNAL_DIR):
        print(f"Removing {EXTERNAL_DIR}/...")
        shutil.rmtree(EXTERNAL_DIR)
        print("Dependencies removed.")
    else:
        print("No dependencies to remove.")


@task
def setup(c, release=False):
    """Fetch dependencies and build the project (for fresh checkouts)."""
    fetch_deps(c)
    build(c, release=release)


@task
def configure(c, release=False):
    """Configure CMake build."""
    build_type = "Release" if release else "Debug"
    os.makedirs(BUILD_DIR, exist_ok=True)
    c.run(f"cmake -B {BUILD_DIR} -DCMAKE_BUILD_TYPE={build_type}")


@task(pre=[configure])
def build(c, release=False):
    """Build the project."""
    c.run(f"cmake --build {BUILD_DIR} -j$(nproc)")


@task
def clean(c):
    """Clean build artifacts."""
    c.run(f"rm -rf {BUILD_DIR}")


@task
def rebuild(c, release=False):
    """Clean and rebuild the project."""
    clean(c)
    build(c, release=release)


@task(pre=[build])
def test(c, filter=None):
    """Run tests.

    Args:
        filter: Optional test filter (e.g., "[PitchShiftNode]" or "[baseline]")
    """
    test_cmd = f"{BUILD_DIR}/VoiceChangerTests"
    if filter:
        test_cmd += f' "{filter}"'
    c.run(test_cmd)


@task(pre=[build])
def run(c):
    """Run the voice changer demo application."""
    c.run(f"{BUILD_DIR}/voice-changer-demo")


@task
def test_integration(c):
    """Run integration tests with Harvard sentences."""
    c.run(f'{BUILD_DIR}/VoiceChangerTests "[integration]"')


@task
def test_baseline(c):
    """Run baseline tests only."""
    c.run(f'{BUILD_DIR}/VoiceChangerTests "[baseline]"')


@task
def format(c):
    """Format source code with clang-format."""
    c.run("find src tests -name '*.cpp' -o -name '*.hpp' | xargs clang-format -i")


@task
def check(c):
    """Run static analysis with clang-tidy."""
    c.run(f"run-clang-tidy -p {BUILD_DIR} src/")
