"""
Invoke tasks for Voice Changer Demo project.

Usage:
    inv build       - Build the project
    inv test        - Run all tests
    inv run         - Run the demo application
    inv clean       - Clean build artifacts
    inv rebuild     - Clean and rebuild
"""

from invoke import task
import os

BUILD_DIR = "build"


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
