#pragma once

#include "../engine/ModuleBase.h"
#include "../engine/Types.h"
#include <opencv2/opencv.hpp>

namespace oak {

    class PreviewModule : public ModuleBase {
    public:
        explicit PreviewModule(const OutputConfig& config);
        ~PreviewModule() override = default;

        bool configure(dai::Pipeline& pipeline,
            std::shared_ptr<dai::node::Camera> camera) override;

        std::string getName() const override { return "PreviewModule"; }
        ModuleState getStateType() const override { return ModuleState::PREVIEW; }

        void process() override;
        void cleanup() override;

    private:
        OutputConfig config_;
        std::shared_ptr<dai::MessageQueue> output_queue_;
        bool show_preview_ = true;
    };

} // namespace oak
