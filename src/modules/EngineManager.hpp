#include <memory>
#include <map>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <iostream>

#include <depthai/depthai.hpp>

enum class ModuleState {
    IDLE,
    PREVIEW_RUNNING,
    RECONDING_RUNNING,
    INFERENCE_RUNNING
};

class EngineManager {
    public:
        static EngineManager& getInstance();

        EngineManager(const EngineManager&) = delete;
        EngineManager& operator=(const EngineManager&) = delete;

        bool connectAndStart();
        void disconnect();

        bool StartPreview();
        bool StartReconding();
        bool StartInference(const std:;string& modelPath, const std::string& configPath);

        void stopModule();

        void setExposure(int iso, int exposureTime);
        void setGain(int gain);
        void setWhiteBalance(int colorTemperature);
        void setFocus(int focus);
        void setFocusAuto(bool autoFocus);
        void setFocusManual(int focus);
        void setFocusAuto(bool autoFocus);

        ModuleState getModuleState() const;
    private:
        EngineManager();
        ~EngineManager();
        
        std::unique_ptr<dai::Device> device;
        std::shared_ptr<dai::Pipeline> pipeline;

        std::map<std::string, std::shared_ptr<dai::DataOutputQueue>> outputQueues;
        
        std::atomic<ModuleState> currentState { ModuleState::IDLE };
        std::mutex stateMutex;

        std:;thread dataProcessingThread;
        std::atomic<bool> stopProcessingThread {false};
        void DataProcessingLoop();

        void buildAndDeployPipeline(const dai::Pipeline& newPipeline);
        void startDataProcessingThread();
        void stopDataProcessingThread();
};