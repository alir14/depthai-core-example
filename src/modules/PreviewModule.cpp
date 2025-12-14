#include "PreviewModule.h"
#include <iostream>

namespace oak {

PreviewModule::PreviewModule(const OutputConfig& config) 
    : config_(config) {
}

bool PreviewModule::configure(dai::Pipeline& pipeline,
                              std::shared_ptr<dai::node::Camera> camera) {
    try {
        // V3 API: Request output directly from camera node
        // This replaces the need for separate ImageManip nodes in many cases
        dai::ImgResizeMode resize_mode;
        switch (config_.resize_mode) {
            case ResizeMode::CROP:
                resize_mode = dai::ImgResizeMode::CROP;
                break;
            case ResizeMode::STRETCH:
                resize_mode = dai::ImgResizeMode::STRETCH;
                break;
            case ResizeMode::LETTERBOX:
                resize_mode = dai::ImgResizeMode::LETTERBOX;
                break;
            default:
                resize_mode = dai::ImgResizeMode::CROP;
        }

        // Request BGR output for OpenCV compatibility
        auto* output = camera->requestOutput(
            {config_.width, config_.height},
            dai::ImgFrame::Type::BGR888p,
            resize_mode,
            config_.fps,
            config_.enable_undistortion
        );

        // V3 API: Create output queue directly from node output
        output_queue_ = output->createOutputQueue(8, false);

        std::cout << "PreviewModule configured: " << config_.width << "x" << config_.height 
                  << " @ " << config_.fps << " fps" << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to configure PreviewModule: " << e.what() << std::endl;
        return false;
    }
}

void PreviewModule::process() {
    if (!output_queue_) {
        return;
    }

    // Try to get frame (non-blocking)
    auto imgFrame = output_queue_->tryGet<dai::ImgFrame>();
    
    if (imgFrame) {
        // Call callback if set
        if (frame_callback_) {
            frame_callback_(imgFrame);
        }

        // Display preview
        if (show_preview_) {
            cv::Mat frame = imgFrame->getCvFrame();
            cv::imshow("OAK Preview", frame);
            
            int key = cv::waitKey(1);
            if (key == 'q' || key == 'Q' || key == 27) {  // q or ESC
                show_preview_ = false;
                cv::destroyWindow("OAK Preview");
            }
        }
    }
}

void PreviewModule::cleanup() {
    if (show_preview_) {
        cv::destroyAllWindows();
    }
    output_queue_.reset();
}

} // namespace oak
