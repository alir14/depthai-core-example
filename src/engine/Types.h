#pragma once

#include <string>
#include <cstdint>
#include <optional>

namespace oak {

enum class ModuleState {
    IDLE,
    PREVIEW,
    RECORD,
    INFERENCE
};

enum class ResizeMode {
    CROP,
    STRETCH,
    LETTERBOX
};

struct EngineConfig {
    std::string device_id = "";  // Empty = auto-detect first device
    bool use_poe = false;        // Use PoE connection
};

struct OutputConfig {
    uint32_t width = 1280;
    uint32_t height = 720;
    float fps = 30.0f;
    ResizeMode resize_mode = ResizeMode::CROP;
    bool enable_undistortion = false;
};

struct CameraSettings {
    std::optional<int> iso;              // nullopt = auto
    std::optional<int> exposure_us;      // microseconds, nullopt = auto
    std::optional<int> focus;            // 0-255, nullopt = auto
    int brightness = 0;                  // -10 to 10
    int contrast = 0;                    // -10 to 10
    int saturation = 0;                  // -10 to 10
    int sharpness = 0;                   // 0-4
    bool auto_focus = true;
    bool auto_exposure = true;
    bool auto_white_balance = true;
};

struct RecordConfig {
    std::string output_path = "recordings/";
    std::string filename_prefix = "recording";
    uint32_t width = 1920;
    uint32_t height = 1080;
    float fps = 30.0f;
    int bitrate = 8000000; // 8 Mbps
    // Note: RecordVideo node only supports H264 encoding
};

struct InferenceConfig {
    std::string model_path;
    uint32_t input_width = 640;
    uint32_t input_height = 640;
    float confidence_threshold = 0.5f;
    bool sync_nn_with_preview = true;
};

inline std::string moduleStateToString(ModuleState state) {
    switch (state) {
        case ModuleState::IDLE:      return "IDLE";
        case ModuleState::PREVIEW:   return "PREVIEW";
        case ModuleState::RECORD:    return "RECORD";
        case ModuleState::INFERENCE: return "INFERENCE";
        default:                     return "UNKNOWN";
    }
}

inline ModuleState moduleStateFromString(const std::string& str) {
    if (str == "PREVIEW")   return ModuleState::PREVIEW;
    if (str == "RECORD")    return ModuleState::RECORD;
    if (str == "INFERENCE") return ModuleState::INFERENCE;
    return ModuleState::IDLE;
}

} // namespace oak
