
#include "RecordModule.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem>

namespace oak {

RecordModule::RecordModule(const RecordConfig& config) 
    : config_(config) {
}

bool RecordModule::configure(dai::Pipeline& pipeline,
                             std::shared_ptr<dai::node::Camera> camera) {
    try {
        // Generate output filename with timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << config_.output_path << config_.filename_prefix << "_"
           << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
           << ".mp4";
        std::string relative_path = ss.str();

        // Create output directory if needed
        std::filesystem::create_directories(config_.output_path);

        // Convert to absolute path for RecordVideo node (safer for device filesystem)
        std::filesystem::path file_path = std::filesystem::absolute(relative_path);
        output_file_path_ = file_path.string();

        // Create video encoder
        // RecordVideo node only supports H264 encoding (as per DepthAI example)
        auto videoEncoder = pipeline.create<dai::node::VideoEncoder>();
        videoEncoder->setProfile(dai::VideoEncoderProperties::Profile::H264_MAIN);
        videoEncoder->setBitrate(config_.bitrate);
        videoEncoder->setKeyframeFrequency(static_cast<int>(config_.fps));

        // Camera output for encoder (NV12 is optimal for encoding)
        auto* encoderInput = camera->requestOutput(
            {config_.width, config_.height},
            dai::ImgFrame::Type::NV12,
            dai::ImgResizeMode::CROP,
            config_.fps,
            false
        );
        encoderInput->link(videoEncoder->input);

        // V3: Use RecordVideo node for on-device MP4 recording
        // Note: RecordVideo writes to the device filesystem, then transfers to host
        auto record = pipeline.create<dai::node::RecordVideo>();
        record->setRecordVideoFile(file_path);
        videoEncoder->out.link(record->input);

        // Separate preview stream for UI (lower res, doesn't affect recording)
        auto* previewOutput = camera->requestOutput(
            {640, 360},
            dai::ImgFrame::Type::BGR888i,
            dai::ImgResizeMode::CROP,
            config_.fps,
            false
        );
        preview_queue_ = previewOutput->createOutputQueue(4, false);

        start_time_ = std::chrono::steady_clock::now();

        std::cout << "RecordModule configured: " << config_.width << "x" << config_.height 
                  << " @ " << config_.fps << " fps -> " << output_file_path_ << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to configure RecordModule: " << e.what() << std::endl;
        return false;
    }
}

void RecordModule::process() {
    // Only handle preview - recording happens on-device automatically
    if (preview_queue_ && show_preview_) {
        auto previewFrame = preview_queue_->tryGet<dai::ImgFrame>();
        if (previewFrame) {
            if (frame_callback_) {
                frame_callback_(previewFrame);
            }

            cv::Mat frame = previewFrame->getCvFrame();
            
            // Recording indicator
            cv::circle(frame, cv::Point(30, 30), 15, cv::Scalar(0, 0, 255), -1);
            cv::putText(frame, "REC", cv::Point(50, 38), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
            
            // Elapsed time
            auto elapsed = std::chrono::steady_clock::now() - start_time_;
            auto secs = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
            std::string time_text = "Time: " + std::to_string(secs / 60) + ":" + 
                                    (secs % 60 < 10 ? "0" : "") + std::to_string(secs % 60);
            cv::putText(frame, time_text, cv::Point(10, frame.rows - 20),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);

            cv::imshow("Recording Preview", frame);
            
            int key = cv::waitKey(1);
            if (key == 'q' || key == 'Q' || key == 27) {
                show_preview_ = false;
                cv::destroyWindow("Recording Preview");
            }
        }
    }
}

void RecordModule::cleanup() {
    if (show_preview_) {
        cv::destroyAllWindows();
    }
    preview_queue_.reset();
    
    std::cout << "Recording saved: " << output_file_path_ << std::endl;
}

} // namespace oak