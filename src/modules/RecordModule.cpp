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
        // Create video encoder node
        auto videoEncoder = pipeline.create<dai::node::VideoEncoder>();
        
        // Configure encoder
        if (config_.use_h265) {
            videoEncoder->setDefaultProfilePreset(
                config_.fps,
                dai::VideoEncoderProperties::Profile::H265_MAIN
            );
        } else {
            videoEncoder->setDefaultProfilePreset(
                config_.fps,
                dai::VideoEncoderProperties::Profile::H264_MAIN
            );
        }
        videoEncoder->setBitrate(config_.bitrate);
        videoEncoder->setKeyframeFrequency(static_cast<int>(config_.fps)); // Keyframe every second

        // V3 API: Request camera output for encoder (NV12 is efficient for encoding)
        auto* cameraOutput = camera->requestOutput(
            {config_.width, config_.height},
            dai::ImgFrame::Type::NV12,
            dai::ImgResizeMode::CROP,
            config_.fps,
            false  // no undistortion needed for recording
        );

        // Link camera to encoder
        cameraOutput->link(videoEncoder->input);

        // Create output queue for encoded data
        encoded_queue_ = videoEncoder->out.createOutputQueue(30, true);

        // Also create a preview output for monitoring
        auto* previewOutput = camera->requestOutput(
            {640, 360},  // Lower resolution preview
            dai::ImgFrame::Type::BGR888p,
            dai::ImgResizeMode::CROP,
            config_.fps,
            false
        );
        preview_queue_ = previewOutput->createOutputQueue(4, false);

        // Generate output filename with timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << config_.output_path << config_.filename_prefix << "_"
           << std::put_time(std::localtime(&time_t), "%Y%m%d_%H%M%S")
           << (config_.use_h265 ? ".h265" : ".h264");
        output_file_path_ = ss.str();

        // Create output directory if needed
        std::filesystem::create_directories(config_.output_path);

        // Open output file
        output_file_.open(output_file_path_, std::ios::binary);
        if (!output_file_.is_open()) {
            std::cerr << "Failed to open output file: " << output_file_path_ << std::endl;
            return false;
        }

        std::cout << "RecordModule configured: " << config_.width << "x" << config_.height 
                  << " @ " << config_.fps << " fps"
                  << " -> " << output_file_path_ << std::endl;

        return true;

    } catch (const std::exception& e) {
        std::cerr << "Failed to configure RecordModule: " << e.what() << std::endl;
        return false;
    }
}

void RecordModule::process() {
    // Handle encoded frames
    if (encoded_queue_) {
        auto encodedFrame = encoded_queue_->tryGet<dai::ImgFrame>();
        if (encodedFrame) {
            auto& data = encodedFrame->getData();
            output_file_.write(reinterpret_cast<const char*>(data.data()), data.size());
            recorded_frames_++;
        }
    }

    // Handle preview
    if (preview_queue_ && show_preview_) {
        auto previewFrame = preview_queue_->tryGet<dai::ImgFrame>();
        if (previewFrame) {
            if (frame_callback_) {
                frame_callback_(previewFrame);
            }

            cv::Mat frame = previewFrame->getCvFrame();
            
            // Add recording indicator
            cv::circle(frame, cv::Point(30, 30), 15, cv::Scalar(0, 0, 255), -1);
            cv::putText(frame, "REC", cv::Point(50, 38), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
            
            // Frame counter
            std::string frame_text = "Frames: " + std::to_string(recorded_frames_);
            cv::putText(frame, frame_text, cv::Point(10, frame.rows - 20),
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
    if (output_file_.is_open()) {
        output_file_.flush();
        output_file_.close();
        std::cout << "Recording saved: " << output_file_path_ 
                  << " (" << recorded_frames_ << " frames)" << std::endl;
    }
    
    if (show_preview_) {
        cv::destroyAllWindows();
    }
    
    encoded_queue_.reset();
    preview_queue_.reset();
}

} // namespace oak
