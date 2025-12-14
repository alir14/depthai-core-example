#include <iostream>
#include <thread>
#include <chrono>

#include "engine/EngineManager.h"
#include "engine/Types.h"
#include "modules/PreviewModule.h"

int main() {
    std::cout << "OAK Camera Service Engine - Test Application" << std::endl;
    std::cout << "=============================================" << std::endl;

    // Get engine instance
    auto& engine = oak::EngineManager::getInstance();

    // Initialize engine
    oak::EngineConfig config;
    config.preview_width = 1280;
    config.preview_height = 720;
    config.preview_fps = 30;

    if (!engine.initialize(config)) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return 1;
    }

    std::cout << "Engine initialized successfully" << std::endl;
    std::cout << "Device ID: " << engine.getDeviceId() << std::endl;
    std::cout << std::endl;

    // Create and start preview module
    auto previewModule = std::make_shared<oak::PreviewModule>(
        config.preview_width, 
        config.preview_height, 
        config.preview_fps
    );

    std::cout << "Starting preview module..." << std::endl;
    if (!engine.startModule(previewModule)) {
        std::cerr << "Failed to start preview module" << std::endl;
        engine.shutdown();
        return 1;
    }

    std::cout << "Preview module started" << std::endl;
    std::cout << "State: " << oak::moduleStateToString(engine.getState()) << std::endl;
    std::cout << "Active Module: " << engine.getActiveModuleName() << std::endl;
    std::cout << std::endl;
    std::cout << "Press 'q' in preview window or Ctrl+C to stop..." << std::endl;

    // Run for demonstration (in real application, this would be controlled by API)
    std::this_thread::sleep_for(std::chrono::seconds(30));

    // Shutdown
    std::cout << std::endl;
    std::cout << "Stopping preview module..." << std::endl;
    engine.stopModule();
    
    std::cout << "Shutting down engine..." << std::endl;
    engine.shutdown();

    std::cout << "Application finished" << std::endl;
    return 0;
}
