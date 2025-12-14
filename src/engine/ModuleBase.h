#pragma once

#include <memory>
#include <map>
#include <string>
#include <depthai/depthai.hpp>

namespace oak {

class ModuleBase {
public:
    virtual ~ModuleBase() = default;

    // Build and return a pipeline configuration
    virtual std::shared_ptr<dai::Pipeline> buildPipeline() = 0;

    // Get output queue names this module creates
    virtual std::vector<std::string> getOutputQueueNames() const = 0;

    // Process output queues (called in loop by engine)
    virtual void processOutputs(
        std::map<std::string, std::shared_ptr<dai::DataOutputQueue>>& queues
    ) = 0;

    // Get module name
    virtual std::string getName() const = 0;

    // Cleanup before stopping
    virtual void cleanup() {}

protected:
    ModuleBase() = default;
};

} // namespace oak
