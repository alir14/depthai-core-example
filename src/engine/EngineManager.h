#pragma once

#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <depthai/depthai.hpp>

#include "Types.h"
#include "ModuleBase.h"
#include "CameraController.h"

namespace oak {

class EngineManager {
public:
    static EngineManager& getInstance();

    EngineManager(const EngineManager&) = delete;
    EngineManager& operator=(const EngineManager&) = delete;

    // Lifecycle
    bool initialize(const EngineConfig& config = EngineConfig{});
    void shutdown();

    // Module control
    bool startPreview(const OutputConfig& config = OutputConfig{});
    bool startRecording(const RecordConfig& config);
    bool startInference(const InferenceConfig& config);
    bool stopModule();

    // Camera settings
    bool updateCameraSettings(const CameraSettings& settings);
    CameraSettings getCameraSettings() const;

    // State queries
    ModuleState getState() const { return state_.load(); }
    std::string getActiveModuleName() const;
    bool isRunning() const { return running_.load(); }

    // Device info
    std::string getDeviceId() const;
    std::string getDeviceName() const;
    bool isDeviceConnected() const;
    std::vector<dai::CameraBoardSocket> getConnectedCameras() const;

    // Callbacks
    void setFrameCallback(FrameCallback callback);
    void setDetectionCallback(DetectionCallback callback);

private:
    EngineManager() = default;
    ~EngineManager();

    // Internal pipeline management
    bool buildAndStartPipeline(std::shared_ptr<ModuleBase> module);
    void stopPipeline();
    void processingLoop();

    // Device and pipeline (V3 style - pipeline takes device in constructor)
    std::shared_ptr<dai::Device> device_;
    std::unique_ptr<dai::Pipeline> pipeline_;
    
    // Camera node shared across modules
    std::shared_ptr<dai::node::Camera> camera_node_;
    
    // Control queue for camera settings (V3 uses InputQueue for input queues)
    std::shared_ptr<dai::InputQueue> control_queue_;
    
    // Camera controller
    CameraController camera_controller_;
    
    // Active module
    std::shared_ptr<ModuleBase> active_module_;

    // State
    std::atomic<ModuleState> state_{ModuleState::IDLE};
    EngineConfig config_;
    CameraSettings camera_settings_;
    OutputConfig current_output_config_;

    // Threading
    std::atomic<bool> running_{false};
    std::atomic<bool> pipeline_running_{false};
    std::thread processing_thread_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;

    // Callbacks
    FrameCallback frame_callback_;
    DetectionCallback detection_callback_;
};

} // namespace oak
