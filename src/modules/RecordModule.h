#pragma once

#include "../engine/ModuleBase.h"
#include "../engine/Types.h"
#include <fstream>
#include <opencv2/opencv.hpp>

namespace oak {

class RecordModule : public ModuleBase {
public:
    explicit RecordModule(const RecordConfig& config);
    ~RecordModule() override = default;

    bool configure(dai::Pipeline& pipeline, 
                  std::shared_ptr<dai::node::Camera> camera) override;
    
    std::string getName() const override { return "RecordModule"; }
    ModuleState getStateType() const override { return ModuleState::RECORD; }
    
    void process() override;
    void cleanup() override;

    // Recording control
    std::string getOutputFilePath() const { return output_file_path_; }
    uint64_t getRecordedFrames() const { return recorded_frames_; }

private:
    RecordConfig config_;
    std::shared_ptr<dai::OutputQueue> encoded_queue_;
    std::shared_ptr<dai::OutputQueue> preview_queue_;
    
    std::ofstream output_file_;
    std::string output_file_path_;
    uint64_t recorded_frames_ = 0;
    bool show_preview_ = true;
};

} // namespace oak
