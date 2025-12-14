#pragma once

#include "../engine/ModuleBase.h"
#include "../engine/Types.h"
#include <opencv2/opencv.hpp>

namespace oak {

class InferenceModule : public ModuleBase {
public:
    explicit InferenceModule(const InferenceConfig& config);
    ~InferenceModule() override = default;

    bool configure(dai::Pipeline& pipeline, 
                  std::shared_ptr<dai::node::Camera> camera) override;
    
    std::string getName() const override { return "InferenceModule"; }
    ModuleState getStateType() const override { return ModuleState::INFERENCE; }
    
    void process() override;
    void cleanup() override;

private:
    void drawDetections(cv::Mat& frame, 
                       const std::vector<dai::ImgDetection>& detections);

    InferenceConfig config_;
    std::shared_ptr<dai::OutputQueue> preview_queue_;
    std::shared_ptr<dai::OutputQueue> detection_queue_;
    
    std::vector<std::string> labels_;
    bool show_preview_ = true;
};

} // namespace oak
