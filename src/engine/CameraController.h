#pragma once

#include <memory>
#include <depthai/depthai.hpp>
#include "Types.h"

namespace oak {

    class CameraController {
    public:
        CameraController() = default;
        ~CameraController() = default;

        // Apply camera settings via control queue
        // V3 API uses MessageQueue for input queues (created from node inputs)
        void applySettings(std::shared_ptr<dai::MessageQueue> controlQueue,
            const CameraSettings& settings);

        // Individual control methods
        void setManualExposure(std::shared_ptr<dai::MessageQueue> controlQueue,
            int exposure_us, int iso);
        void setAutoExposure(std::shared_ptr<dai::MessageQueue> controlQueue);

        void setManualFocus(std::shared_ptr<dai::MessageQueue> controlQueue, int lens_pos);
        void setAutoFocus(std::shared_ptr<dai::MessageQueue> controlQueue);
        void triggerAutoFocus(std::shared_ptr<dai::MessageQueue> controlQueue);

        void setManualWhiteBalance(std::shared_ptr<dai::MessageQueue> controlQueue, int temp_k);
        void setAutoWhiteBalance(std::shared_ptr<dai::MessageQueue> controlQueue);

        void setBrightness(std::shared_ptr<dai::MessageQueue> controlQueue, int value);
        void setContrast(std::shared_ptr<dai::MessageQueue> controlQueue, int value);
        void setSaturation(std::shared_ptr<dai::MessageQueue> controlQueue, int value);
        void setSharpness(std::shared_ptr<dai::MessageQueue> controlQueue, int value);

    private:
        CameraSettings current_settings_;
    };

} // namespace oak
