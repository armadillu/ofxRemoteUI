# Project Generator reads this file when generating platform project files.
# Set OF_ROOT here if the openFrameworks installation is not found automatically.
OF_ROOT = ../../..

# The addon excludes this private ImGui copy from normal addon consumers.
PROJECT_EXTERNAL_SOURCE_PATHS = ../libs/imgui

export MAC_OS_MIN_VERSION = 10.15
export MAC_OS_CPP_VER = -std=c++17
