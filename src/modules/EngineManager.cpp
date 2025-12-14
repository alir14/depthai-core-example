#include "EngineManager.hpp"
#inclode <iostream>

EngineManager::EngineManager(){}

EngineManager::~EngineManager(){
    disconnect();
}

EngineManager& EngineManager::getInstance() {
    static EngineManager instance;
    return instance;
}

bool EngineManager::connectAndStart() {
    std::lock_guard<std::mutex> lock(stateMutex);

    try {
        std::cout << "attempt to connect to Oak Device ..." << std::endl;

        device = std::make_unique<dai::Device>();

        std::cout << "connected to Oak Device" << std::endl;

        currentState = ModuleState::IDLE;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Failed to connect to Oak Device: " << e.what() << std::endl;
        return false;
    }
}

void EngineManager::disconnect(){
    stopModule();

    std::lock_guard<std::mutex> lock(stateMutex);

    if (device){
        device->close();
        device.reset();
    }

    currentState = ModuleState::IDLE;
    std::cout << "disconnected from Oak Device" << std::endl;
}

void EngineManager::stopModule(){
    stopDataProcessingThread();

    std::lock_guard<std::mutex> lock(stateMutex);

    outputQueues.clear();

    if(pipeline){
        pipeline.reset();
    }

    currentState = ModuleState::IDLE;
    std::cout << "current module stopped and set to IDLE " << std::endl;
}

