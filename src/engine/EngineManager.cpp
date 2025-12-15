#include "EngineManager.h"
#include "../modules/PreviewModule.h"
#include "../modules/RecordModule.h"
#include "../modules/InferenceModule.h"
#include <iostream>
#include <chrono>
#include <thread>

namespace oak {

EngineManager& EngineManager::getInstance() {
    static EngineManager instance;
    return instance;
}

EngineManager::~EngineManager() {
    shutdown();
}

bool EngineManager::initialize(const EngineConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (device_) {
        std::cerr << "Engine already initialized" << std::endl;
        return false;
    }

    config_ = config;

    try {
        // Connect to device
        if (config_.device_id.empty()) {
            std::cout << "Connecting to first available device..." << std::endl;
            device_ = std::make_shared<dai::Device>();
        } else {
            std::cout << "Connecting to device: " << config_.device_id << std::endl;
            dai::DeviceInfo info(config_.device_id);
            device_ = std::make_shared<dai::Device>(info);
        }

        std::cout << "Connected to device: " << device_->getDeviceName() << std::endl;
        std::cout << "MxId: " << device_->getMxId() << std::endl;
        
        // List connected cameras
        auto cameras = device_->getConnectedCameras();
        std::cout << "Connected cameras: ";
        for (const auto& cam : cameras) {
            std::cout << static_cast<int>(cam) << " ";
        }
        std::cout << std::endl;

        state_ = ModuleState::IDLE;
        running_ = true;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to initialize device: " << e.what() << std::endl;
        device_.reset();
        return false;
    }
}

void EngineManager::shutdown() {
    std::cout << "Shutting down engine..." << std::endl;

    stopModule();

    std::lock_guard<std::mutex> lock(mutex_);
    running_ = false;
    
    if (device_) {
        device_->close();
        device_.reset();
    }

    state_ = ModuleState::IDLE;
    std::cout << "Engine shutdown complete" << std::endl;
}

bool EngineManager::startPreview(const OutputConfig& config) {
    if (!device_) {
        std::cerr << "Device not initialized" << std::endl;
        return false;
    }

    stopModule();

    auto module = std::make_shared<PreviewModule>(config);
    module->setFrameCallback(frame_callback_);
    
    if (!buildAndStartPipeline(module)) {
        return false;
    }

    state_ = ModuleState::PREVIEW;
    current_output_config_ = config;
    return true;
}

bool EngineManager::startRecording(const RecordConfig& config) {
    if (!device_) {
        std::cerr << "Device not initialized" << std::endl;
        return false;
    }

    stopModule();

    auto module = std::make_shared<RecordModule>(config);
    module->setFrameCallback(frame_callback_);
    
    if (!buildAndStartPipeline(module)) {
        return false;
    }

    state_ = ModuleState::RECORD;
    return true;
}

bool EngineManager::startInference(const InferenceConfig& config) {
    if (!device_) {
        std::cerr << "Device not initialized" << std::endl;
        return false;
    }

    stopModule();

    auto module = std::make_shared<InferenceModule>(config);
    module->setFrameCallback(frame_callback_);
    module->setDetectionCallback(detection_callback_);
    
    if (!buildAndStartPipeline(module)) {
        return false;
    }

    state_ = ModuleState::INFERENCE;
    return true;
}

bool EngineManager::buildAndStartPipeline(std::shared_ptr<ModuleBase> module) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        std::cout << "[DEBUG] Building pipeline for module: " << module->getName() << std::endl;
        
        // Check device state
        if (!device_) {
            std::cerr << "[DEBUG] ERROR: device_ is null!" << std::endl;
            return false;
        }
        std::cout << "[DEBUG] Device pointer is valid" << std::endl;
        
        // Create pipeline with device (V3 API style)
        // Note: Don't access device methods here as device may be in transition state
        std::cout << "[DEBUG] Creating new pipeline with device..." << std::endl;
        pipeline_ = std::make_unique<dai::Pipeline>(device_);
        std::cout << "[DEBUG] Pipeline created successfully" << std::endl;

        // Create camera node
        std::cout << "[DEBUG] Creating camera node..." << std::endl;
        camera_node_ = pipeline_->create<dai::node::Camera>();
        std::cout << "[DEBUG] Building camera node..." << std::endl;
        camera_node_->build(dai::CameraBoardSocket::CAM_A);
        std::cout << "[DEBUG] Camera node built successfully" << std::endl;

        // Configure module with pipeline and camera
        std::cout << "[DEBUG] Configuring module..." << std::endl;
        if (!module->configure(*pipeline_, camera_node_)) {
            std::cerr << "[DEBUG] Failed to configure module" << std::endl;
            return false;
        }
        std::cout << "[DEBUG] Module configured successfully" << std::endl;

        // Create control queue for camera settings BEFORE starting pipeline
        // V3 API: createInputQueue must be called before pipeline->start()
        std::cout << "[DEBUG] Creating control queue..." << std::endl;
        control_queue_ = camera_node_->inputControl.createInputQueue();
        std::cout << "[DEBUG] Control queue created successfully" << std::endl;

        // Start pipeline (V3 API)
        std::cout << "[DEBUG] Starting pipeline..." << std::endl;
        pipeline_->start();
        std::cout << "[DEBUG] Pipeline started successfully" << std::endl;

        active_module_ = module;
        pipeline_running_ = true;

        // Start processing thread
        processing_thread_ = std::thread(&EngineManager::processingLoop, this);

        std::cout << "Module started: " << module->getName() << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to build pipeline: " << e.what() << std::endl;
        pipeline_.reset();
        camera_node_.reset();
        return false;
    }
}

bool EngineManager::stopModule() {
    std::cout << "[DEBUG] stopModule() called" << std::endl;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_module_) {
            std::cout << "[DEBUG] No active module, returning" << std::endl;
            return true;
        }

        std::cout << "Stopping module: " << active_module_->getName() << std::endl;
        pipeline_running_ = false;
    }

    // Notify and wait for processing thread
    std::cout << "[DEBUG] Notifying processing thread and waiting..." << std::endl;
    cv_.notify_all();
    if (processing_thread_.joinable()) {
        processing_thread_.join();
        std::cout << "[DEBUG] Processing thread joined" << std::endl;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (active_module_) {
        std::cout << "[DEBUG] Cleaning up active module..." << std::endl;
        active_module_->cleanup();
        active_module_.reset();
        std::cout << "[DEBUG] Active module cleaned up" << std::endl;
    }

    stopPipeline();
    
    state_ = ModuleState::IDLE;
    std::cout << "Module stopped" << std::endl;
    return true;
}

void EngineManager::stopPipeline() {
    std::cout << "[DEBUG] stopPipeline() called" << std::endl;
    
    // Reset queues first
    std::cout << "[DEBUG] Resetting control queue..." << std::endl;
    control_queue_.reset();
    std::cout << "[DEBUG] Control queue reset" << std::endl;
    
    // Reset camera node
    std::cout << "[DEBUG] Resetting camera node..." << std::endl;
    camera_node_.reset();
    std::cout << "[DEBUG] Camera node reset" << std::endl;
    
    // Stop and wait for pipeline
    if (pipeline_) {
        try {
            std::cout << "[DEBUG] Stopping pipeline..." << std::endl;
            pipeline_->stop();
            std::cout << "[DEBUG] Pipeline stop() called, waiting..." << std::endl;
            // Wait for pipeline to fully stop before destroying (prevents segfault when restarting)
            pipeline_->wait();
            std::cout << "[DEBUG] Pipeline wait() completed" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[DEBUG] Error stopping pipeline: " << e.what() << std::endl;
        }
        std::cout << "[DEBUG] Resetting pipeline..." << std::endl;
        pipeline_.reset();
        std::cout << "[DEBUG] Pipeline reset complete" << std::endl;
    } else {
        std::cout << "[DEBUG] Pipeline was already null" << std::endl;
    }
    
    // Close and reopen device to reset state for next pipeline
    // This is necessary because in DepthAI V3, stopping a pipeline leaves the device
    // in a state that prevents creating a new pipeline with the same device
    if (device_) {
        std::cout << "[DEBUG] Closing device to reset state..." << std::endl;
        try {
            // Use config device_id (we stored it during initialize)
            device_->close();
            std::cout << "[DEBUG] Device closed, reopening..." << std::endl;
            
            // Small delay to ensure device is fully released
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            // Reopen device using the same config
            if (config_.device_id.empty()) {
                device_ = std::make_shared<dai::Device>();
            } else {
                dai::DeviceInfo info(config_.device_id);
                device_ = std::make_shared<dai::Device>(info);
            }
            std::cout << "[DEBUG] Device reopened successfully" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "[DEBUG] Error closing/reopening device: " << e.what() << std::endl;
            // If reopening fails, device will be null and next start will fail gracefully
            device_.reset();
        }
    }
    
    std::cout << "[DEBUG] stopPipeline() finished" << std::endl;
}

void EngineManager::processingLoop() {
    std::cout << "Processing loop started" << std::endl;

    while (pipeline_running_ && running_) {
        try {
            std::shared_ptr<ModuleBase> module;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                module = active_module_;
            }

            if (module && pipeline_ && pipeline_->isRunning()) {
                module->process();
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

        } catch (const std::exception& e) {
            std::cerr << "Error in processing loop: " << e.what() << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    std::cout << "Processing loop stopped" << std::endl;
}

bool EngineManager::updateCameraSettings(const CameraSettings& settings) {
    std::lock_guard<std::mutex> lock(mutex_);

    camera_settings_ = settings;

    if (control_queue_) {
        camera_controller_.applySettings(control_queue_, settings);
        return true;
    }

    std::cout << "Camera settings stored (will apply on next pipeline start)" << std::endl;
    return true;
}

CameraSettings EngineManager::getCameraSettings() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return camera_settings_;
}

std::string EngineManager::getActiveModuleName() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (active_module_) {
        return active_module_->getName();
    }
    return "NONE";
}

std::string EngineManager::getDeviceId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (device_) {
        return device_->getMxId();
    }
    return "";
}

std::string EngineManager::getDeviceName() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (device_) {
        return device_->getDeviceName();
    }
    return "";
}

bool EngineManager::isDeviceConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return device_ != nullptr;
}

std::vector<dai::CameraBoardSocket> EngineManager::getConnectedCameras() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (device_) {
        return device_->getConnectedCameras();
    }
    return {};
}

void EngineManager::setFrameCallback(FrameCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    frame_callback_ = callback;
    if (active_module_) {
        active_module_->setFrameCallback(callback);
    }
}

void EngineManager::setDetectionCallback(DetectionCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    detection_callback_ = callback;
    if (active_module_) {
        active_module_->setDetectionCallback(callback);
    }
}

} // namespace oak
