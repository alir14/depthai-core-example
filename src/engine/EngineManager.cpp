#include "EngineManager.h"
#include <iostream>
#include <chrono>

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
        
        state_ = ModuleState::IDLE;
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
    if (device_) {
        device_->close();
        device_.reset();
    }
    
    state_ = ModuleState::IDLE;
    std::cout << "Engine shutdown complete" << std::endl;
}

bool EngineManager::startModule(std::shared_ptr<ModuleBase> module) {
    if (!device_) {
        std::cerr << "Device not initialized" << std::endl;
        return false;
    }

    if (!module) {
        std::cerr << "Invalid module" << std::endl;
        return false;
    }

    // Stop existing module
    stopModule();

    std::lock_guard<std::mutex> lock(mutex_);

    try {
        std::cout << "Starting module: " << module->getName() << std::endl;

        // Build pipeline
        auto pipeline = module->buildPipeline();
        if (!pipeline) {
            std::cerr << "Failed to build pipeline" << std::endl;
            return false;
        }

        // Deploy pipeline
        if (!deployPipeline(pipeline)) {
            return false;
        }

        // Create output queues
        auto queue_names = module->getOutputQueueNames();
        for (const auto& name : queue_names) {
            try {
                auto queue = device_->getOutputQueue(name, 8, false);
                output_queues_[name] = queue;
                std::cout << "Created output queue: " << name << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Failed to create queue " << name << ": " << e.what() << std::endl;
            }
        }

        active_module_ = module;
        running_ = true;

        // Start processing thread
        processing_thread_ = std::thread(&EngineManager::processingLoop, this);

        std::cout << "Module started successfully: " << module->getName() << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to start module: " << e.what() << std::endl;
        active_module_.reset();
        output_queues_.clear();
        return false;
    }
}

bool EngineManager::stopModule() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!active_module_) {
            return true; // Already stopped
        }

        std::cout << "Stopping module: " << active_module_->getName() << std::endl;
        running_ = false;
    }

    // Wait for processing thread to finish
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }

    std::lock_guard<std::mutex> lock(mutex_);

    // Cleanup module
    if (active_module_) {
        active_module_->cleanup();
        active_module_.reset();
    }

    // Clear queues
    output_queues_.clear();

    // Stop pipeline
    stopPipeline();

    state_ = ModuleState::IDLE;
    std::cout << "Module stopped" << std::endl;
    
    return true;
}

bool EngineManager::deployPipeline(std::shared_ptr<dai::Pipeline> pipeline) {
    try {
        std::cout << "Deploying pipeline..." << std::endl;
        device_->startPipeline(*pipeline);
        std::cout << "Pipeline deployed successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to deploy pipeline: " << e.what() << std::endl;
        return false;
    }
}

void EngineManager::stopPipeline() {
    if (!device_) return;
    
    try {
        // DepthAI automatically stops pipeline when new one starts
        // or when device is closed
        std::cout << "Pipeline stopped" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error stopping pipeline: " << e.what() << std::endl;
    }
}

void EngineManager::processingLoop() {
    std::cout << "Processing loop started" << std::endl;

    while (running_) {
        try {
            std::shared_ptr<ModuleBase> module;
            std::map<std::string, std::shared_ptr<dai::DataOutputQueue>> queues;

            {
                std::lock_guard<std::mutex> lock(mutex_);
                module = active_module_;
                queues = output_queues_;
            }

            if (module && !queues.empty()) {
                module->processOutputs(queues);
            } else {
                // No active module, sleep briefly
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
    
    // TODO: Send camera control commands to active pipeline
    // This requires having a CameraControl input queue
    std::cout << "Camera settings updated (not yet applied to pipeline)" << std::endl;
    
    return true;
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

} // namespace oak
