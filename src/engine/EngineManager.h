#pragma once

#include <memory>
#include <map>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <depthai/depthai.hpp>

#include "Types.h"
#include "ModuleBase.h"

namespace oak {

class EngineManager {
public:
    static EngineManager& getInstance();

    // Delete copy/move constructors
    EngineManager(const EngineManager&) = delete;
    EngineManager& operator=(const EngineManager&) = delete;

    // Initialize engine with device
    bool initialize(const EngineConfig& config = EngineConfig{});
    
    // Shutdown engine
    void shutdown();

    // Module control
    bool startModule(std::shared_ptr<ModuleBase> module);
    bool stopModule();
    
    // Camera settings
    bool updateCameraSettings(const CameraSettings& settings);

    // State queries
    ModuleState getState() const { return state_; }
    std::string getActiveModuleName() const;
    bool isRunning() const { return running_; }
    
    // Device info
    std::string getDeviceId() const;
    bool isDeviceConnected() const { return device_ != nullptr; }

    // Get device reference (for modules)
    std::shared_ptr<dai::Device> getDevice() { return device_; }

private:
    EngineManager() = default;
    ~EngineManager();

    // Pipeline management
    bool deployPipeline(std::shared_ptr<dai::Pipeline> pipeline);
    void stopPipeline();
    
    // Processing thread
    void processingLoop();

    // Member variables
    std::shared_ptr<dai::Device> device_;
    std::shared_ptr<ModuleBase> active_module_;
    std::map<std::string, std::shared_ptr<dai::DataOutputQueue>> output_queues_;
    
    ModuleState state_ = ModuleState::IDLE;
    EngineConfig config_;
    CameraSettings camera_settings_;
    
    std::atomic<bool> running_{false};
    std::thread processing_thread_;
    mutable std::mutex mutex_;
};

} // namespace oak
