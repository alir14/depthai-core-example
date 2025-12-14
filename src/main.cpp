#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>
#include <atomic>

#include "engine/EngineManager.h"
#include "engine/Types.h"

std::atomic<bool> g_running{true};

void signalHandler(int signum) {
    std::cout << "\nInterrupt signal (" << signum << ") received." << std::endl;
    g_running = false;
}

void printUsage() {
    std::cout << "\nOAK Camera Service Engine - Interactive Demo" << std::endl;
    std::cout << "==============================================" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  p - Start Preview" << std::endl;
    std::cout << "  r - Start Recording" << std::endl;
    std::cout << "  i - Start Inference (requires model)" << std::endl;
    std::cout << "  s - Stop current module" << std::endl;
    std::cout << "  q - Quit" << std::endl;
    std::cout << "  ? - Show this help" << std::endl;
    std::cout << std::endl;
}

void printStatus(oak::EngineManager& engine) {
    std::cout << "\n--- Status ---" << std::endl;
    std::cout << "Device: " << engine.getDeviceName() << std::endl;
    std::cout << "Device ID: " << engine.getDeviceId() << std::endl;
    std::cout << "State: " << oak::moduleStateToString(engine.getState()) << std::endl;
    std::cout << "Active Module: " << engine.getActiveModuleName() << std::endl;
    std::cout << "--------------\n" << std::endl;
}

int main(int argc, char* argv[]) {
    // Setup signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::cout << "OAK Camera Service Engine (OCSE)" << std::endl;
    std::cout << "Using DepthAI V3 API" << std::endl;
    std::cout << "=================================" << std::endl;

    // Get engine instance
    auto& engine = oak::EngineManager::getInstance();

    // Configure engine
    oak::EngineConfig config;
    // config.device_id = "";  // Auto-detect
    // config.use_poe = true;  // Uncomment for PoE devices

    // Check for command line device ID
    if (argc > 1) {
        config.device_id = argv[1];
        std::cout << "Using device ID from command line: " << config.device_id << std::endl;
    }

    // Initialize engine
    std::cout << "\nInitializing engine..." << std::endl;
    if (!engine.initialize(config)) {
        std::cerr << "Failed to initialize engine" << std::endl;
        return 1;
    }

    printStatus(engine);
    printUsage();

    // Set frame callback (optional - for custom frame processing)
    engine.setFrameCallback([](std::shared_ptr<dai::ImgFrame> frame) {
        // Custom frame processing can be done here
        // For example: streaming to network, saving frames, etc.
    });

    // Set detection callback (optional - for inference results)
    engine.setDetectionCallback([](std::shared_ptr<dai::ImgDetections> detections) {
        // Process detections here
        // For example: sending to REST API, logging, etc.
    });

    // Interactive loop
    while (g_running) {
        std::cout << "Enter command (? for help): ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            break;
        }

        if (input.empty()) continue;

        char cmd = input[0];
        
        switch (cmd) {
            case 'p':
            case 'P': {
                std::cout << "Starting preview..." << std::endl;
                oak::OutputConfig previewConfig;
                previewConfig.width = 1280;
                previewConfig.height = 720;
                previewConfig.fps = 30.0f;
                previewConfig.resize_mode = oak::ResizeMode::CROP;
                
                if (engine.startPreview(previewConfig)) {
                    std::cout << "Preview started. Press 'q' in window or 's' here to stop." << std::endl;
                } else {
                    std::cout << "Failed to start preview" << std::endl;
                }
                break;
            }
            
            case 'r':
            case 'R': {
                std::cout << "Starting recording..." << std::endl;
                oak::RecordConfig recordConfig;
                recordConfig.output_path = "recordings/";
                recordConfig.filename_prefix = "oak_recording";
                recordConfig.width = 1920;
                recordConfig.height = 1080;
                recordConfig.fps = 30.0f;
                recordConfig.use_h265 = true;
                recordConfig.bitrate = 8000000;  // 8 Mbps
                
                if (engine.startRecording(recordConfig)) {
                    std::cout << "Recording started. Press 's' to stop and save." << std::endl;
                } else {
                    std::cout << "Failed to start recording" << std::endl;
                }
                break;
            }
            
            case 'i':
            case 'I': {
                std::cout << "Enter model path (.blob or .tar.xz): ";
                std::string modelPath;
                std::getline(std::cin, modelPath);
                
                if (modelPath.empty()) {
                    std::cout << "Model path required for inference" << std::endl;
                    break;
                }
                
                std::cout << "Starting inference..." << std::endl;
                oak::InferenceConfig inferConfig;
                inferConfig.model_path = modelPath;
                inferConfig.input_width = 640;
                inferConfig.input_height = 640;
                inferConfig.confidence_threshold = 0.5f;
                
                if (engine.startInference(inferConfig)) {
                    std::cout << "Inference started. Press 'q' in window or 's' here to stop." << std::endl;
                } else {
                    std::cout << "Failed to start inference" << std::endl;
                }
                break;
            }
            
            case 's':
            case 'S':
                std::cout << "Stopping module..." << std::endl;
                engine.stopModule();
                std::cout << "Module stopped" << std::endl;
                break;
            
            case 'q':
            case 'Q':
                g_running = false;
                break;
            
            case '?':
                printUsage();
                printStatus(engine);
                break;
            
            default:
                std::cout << "Unknown command. Press '?' for help." << std::endl;
                break;
        }
    }

    // Cleanup
    std::cout << "\nShutting down..." << std::endl;
    engine.shutdown();
    std::cout << "Goodbye!" << std::endl;

    return 0;
}
