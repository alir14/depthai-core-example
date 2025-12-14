#pragma once

#include <memory>
#include <string>
#include <functional>
#include <depthai/depthai.hpp>
#include "Types.h"

namespace oak {

// Callback types for data output
using FrameCallback = std::function<void(std::shared_ptr<dai::ImgFrame>)>;
using DetectionCallback = std::function<void(std::shared_ptr<dai::ImgDetections>)>;

class ModuleBase {
public:
    virtual ~ModuleBase() = default;

    // Configure the pipeline with camera node and output queues
    virtual bool configure(dai::Pipeline& pipeline, 
                          std::shared_ptr<dai::node::Camera> camera) = 0;

    // Get module name
    virtual std::string getName() const = 0;

    // Get module state type
    virtual ModuleState getStateType() const = 0;

    // Process loop - called in main processing thread
    virtual void process() = 0;

    // Cleanup resources
    virtual void cleanup() {}

    // Set callbacks
    void setFrameCallback(FrameCallback callback) { frame_callback_ = callback; }
    void setDetectionCallback(DetectionCallback callback) { detection_callback_ = callback; }

protected:
    ModuleBase() = default;
    
    FrameCallback frame_callback_;
    DetectionCallback detection_callback_;
};

} // namespace oak
