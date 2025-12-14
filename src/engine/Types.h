#pragma once

#include <string>

namespace oak {

enum class ModuleState {
    IDLE,
    PREVIEW,
    RECORD,
    INFERENCE
};

struct EngineConfig {
    std::string device_id = "";  // Empty = auto-detect first device
    int preview_width = 1920;
    int preview_height = 1080;
    int preview_fps = 30;
};

struct CameraSettings {
    int iso = -1;           // -1 = auto
    int exposure = -1;      // microseconds, -1 = auto
    int focus = -1;         // 0-255, -1 = auto
    int brightness = 0;     // -10 to 10
    int contrast = 0;       // -10 to 10
    int saturation = 0;     // -10 to 10
    int sharpness = 0;      // 0-4
};

inline std::string moduleStateToString(ModuleState state) {
    switch (state) {
        case ModuleState::IDLE: return "IDLE";
        case ModuleState::PREVIEW: return "PREVIEW";
        case ModuleState::RECORD: return "RECORD";
        case ModuleState::INFERENCE: return "INFERENCE";
        default: return "UNKNOWN";
    }
}

} // namespace oak
