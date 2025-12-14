#include "CameraController.h"
#include <iostream>

namespace oak {

void CameraController::applySettings(std::shared_ptr<dai::InputQueue> controlQueue,
                                     const CameraSettings& settings) {
    if (!controlQueue) {
        std::cerr << "Control queue not available" << std::endl;
        return;
    }

    current_settings_ = settings;

    // Exposure control
    if (settings.auto_exposure) {
        setAutoExposure(controlQueue);
    } else if (settings.exposure_us && settings.iso) {
        setManualExposure(controlQueue, *settings.exposure_us, *settings.iso);
    }

    // Focus control
    if (settings.auto_focus) {
        setAutoFocus(controlQueue);
    } else if (settings.focus) {
        setManualFocus(controlQueue, *settings.focus);
    }

    // White balance
    if (settings.auto_white_balance) {
        setAutoWhiteBalance(controlQueue);
    }

    // Other settings
    setBrightness(controlQueue, settings.brightness);
    setContrast(controlQueue, settings.contrast);
    setSaturation(controlQueue, settings.saturation);
    setSharpness(controlQueue, settings.sharpness);
}

void CameraController::setManualExposure(std::shared_ptr<dai::InputQueue> controlQueue,
                                         int exposure_us, int iso) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setManualExposure(exposure_us, iso);
    controlQueue->send(ctrl);
}

void CameraController::setAutoExposure(std::shared_ptr<dai::InputQueue> controlQueue) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setAutoExposureEnable();
    controlQueue->send(ctrl);
}

void CameraController::setManualFocus(std::shared_ptr<dai::InputQueue> controlQueue, 
                                      int lens_pos) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setManualFocus(lens_pos);
    controlQueue->send(ctrl);
}

void CameraController::setAutoFocus(std::shared_ptr<dai::InputQueue> controlQueue) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setAutoFocusMode(dai::CameraControl::AutoFocusMode::CONTINUOUS_VIDEO);
    controlQueue->send(ctrl);
}

void CameraController::triggerAutoFocus(std::shared_ptr<dai::InputQueue> controlQueue) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setAutoFocusTrigger();
    controlQueue->send(ctrl);
}

void CameraController::setManualWhiteBalance(std::shared_ptr<dai::InputQueue> controlQueue,
                                             int temp_k) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setManualWhiteBalance(temp_k);
    controlQueue->send(ctrl);
}

void CameraController::setAutoWhiteBalance(std::shared_ptr<dai::InputQueue> controlQueue) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setAutoWhiteBalanceMode(dai::CameraControl::AutoWhiteBalanceMode::AUTO);
    controlQueue->send(ctrl);
}

void CameraController::setBrightness(std::shared_ptr<dai::InputQueue> controlQueue, 
                                     int value) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setBrightness(value);
    controlQueue->send(ctrl);
}

void CameraController::setContrast(std::shared_ptr<dai::InputQueue> controlQueue, 
                                   int value) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setContrast(value);
    controlQueue->send(ctrl);
}

void CameraController::setSaturation(std::shared_ptr<dai::InputQueue> controlQueue, 
                                     int value) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setSaturation(value);
    controlQueue->send(ctrl);
}

void CameraController::setSharpness(std::shared_ptr<dai::InputQueue> controlQueue, 
                                    int value) {
    auto ctrl = std::make_shared<dai::CameraControl>();
    ctrl->setSharpness(value);
    controlQueue->send(ctrl);
}

} // namespace oak
