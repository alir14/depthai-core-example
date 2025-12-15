#pragma once

#include "../engine/ModuleBase.h"
#include "../engine/Types.h"
#include <filesystem>
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

    std::string getOutputFilePath() const { return output_file_path_; }

private:
    RecordConfig config_;
    std::shared_ptr<dai::MessageQueue> preview_queue_;
    std::string output_file_path_;
    bool show_preview_ = true;
    std::chrono::steady_clock::time_point start_time_;
};

} // namespace oak