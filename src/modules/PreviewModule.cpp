#include "PreviewModule.h"
#include <iostream>

namespace oak {

PreviewModule::PreviewModule(int width, int height, int fps)
    : width_(width), height_(height), fps_(fps) {
}

std::shared_ptr<dai::Pipeline> PreviewModule::buildPipeline() {
    auto pipeline = std::make_shared<dai::Pipeline>();

    // Create color camera node
    auto camRgb = pipeline->create<dai::node::ColorCamera>();
    camRgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    camRgb->setVideoSize(width_, height_);
    camRgb->setFps(fps_);
    camRgb->setInterleaved(false);
    camRgb->setPreviewSize(width_, height_);

    // Create output node
    auto xoutVideo = pipeline->create<dai::node::XLinkOut>();
    xoutVideo->setStreamName("preview");

    // Link camera to output
    camRgb->video.link(xoutVideo->input);

    std::cout << "PreviewModule pipeline built: " << width_ << "x" << height_ 
              << " @ " << fps_ << " fps" << std::endl;

    return pipeline;
}

std::vector<std::string> PreviewModule::getOutputQueueNames() const {
    return {"preview"};
}

void PreviewModule::processOutputs(
    std::map<std::string, std::shared_ptr<dai::DataOutputQueue>>& queues) {
    
    auto it = queues.find("preview");
    if (it == queues.end()) {
        return;
    }

    auto queue = it->second;
    auto imgFrame = queue->tryGet<dai::ImgFrame>();

    if (imgFrame) {
        // Get OpenCV frame
        cv::Mat frame = imgFrame->getCvFrame();
        
        // Display frame
        cv::imshow("Preview", frame);
        
        // Check for 'q' key to stop
        int key = cv::waitKey(1);
        if (key == 'q' || key == 'Q') {
            std::cout << "Preview window closed by user" << std::endl;
        }
        
        // Could also publish to RTSP/WebRTC here in future
    }
}

} // namespace oak
