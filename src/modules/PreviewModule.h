#pragma once

#include "../engine/ModuleBase.h"
#include "../engine/Types.h"
#include <opencv2/opencv.hpp>

namespace oak {

class PreviewModule : public ModuleBase {
public:
    PreviewModule(int width = 1920, int height = 1080, int fps = 30);
    ~PreviewModule() override = default;

    std::shared_ptr<dai::Pipeline> buildPipeline() override;
    std::vector<std::string> getOutputQueueNames() const override;
    void processOutputs(std::map<std::string, std::shared_ptr<dai::DataOutputQueue>>& queues) override;
    std::string getName() const override { return "PreviewModule"; }

private:
    int width_;
    int height_;
    int fps_;
};

} // namespace oak
