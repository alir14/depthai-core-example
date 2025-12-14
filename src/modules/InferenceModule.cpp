#include "InferenceModule.h"
#include <iostream>
#include <fstream>

namespace oak {

    // Default COCO labels for YOLO models
    static const std::vector<std::string> COCO_LABELS = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck",
        "boat", "traffic light", "fire hydrant", "stop sign", "parking meter", "bench",
        "bird", "cat", "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra",
        "giraffe", "backpack", "umbrella", "handbag", "tie", "suitcase", "frisbee",
        "skis", "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
        "skateboard", "surfboard", "tennis racket", "bottle", "wine glass", "cup",
        "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich", "orange",
        "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair", "couch",
        "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse",
        "remote", "keyboard", "cell phone", "microwave", "oven", "toaster", "sink",
        "refrigerator", "book", "clock", "vase", "scissors", "teddy bear", "hair drier",
        "toothbrush"
    };

    InferenceModule::InferenceModule(const InferenceConfig& config)
        : config_(config), labels_(COCO_LABELS) {
    }

    bool InferenceModule::configure(dai::Pipeline& pipeline,
        std::shared_ptr<dai::node::Camera> camera) {
        try {
            // V3 API: Request camera output sized for neural network input
            auto* nnInput = camera->requestOutput(
                { config_.input_width, config_.input_height },
                dai::ImgFrame::Type::BGR888p,
                dai::ImgResizeMode::LETTERBOX,  // Preserve aspect ratio for NN
                30.0f,
                false
            );

            // Create detection network node
            // V3 API supports NNArchive for model loading
            auto detectionNetwork = pipeline.create<dai::node::DetectionNetwork>();

            // Check if model path is an archive (.tar.xz) or blob
            if (config_.model_path.find(".tar.xz") != std::string::npos ||
                config_.model_path.find(".tar") != std::string::npos) {
                // Use NNArchive for packaged models
                dai::NNArchive archive(config_.model_path);
                detectionNetwork->build(nnInput, archive);
            }
            else {
                // Use blob path directly
                detectionNetwork->setBlobPath(config_.model_path);
                nnInput->link(detectionNetwork->input);
            }

            detectionNetwork->setConfidenceThreshold(config_.confidence_threshold);

            // Create output queues
            detection_queue_ = detectionNetwork->out.createOutputQueue(4, false);

            // Create preview output for visualization
            auto* previewOutput = camera->requestOutput(
                { 640, 480 },
                dai::ImgFrame::Type::BGR888p,
                dai::ImgResizeMode::LETTERBOX,
                30.0f,
                false
            );
            preview_queue_ = previewOutput->createOutputQueue(4, false);

            std::cout << "InferenceModule configured: " << config_.model_path << std::endl;
            std::cout << "Input size: " << config_.input_width << "x" << config_.input_height << std::endl;

            return true;

        }
        catch (const std::exception& e) {
            std::cerr << "Failed to configure InferenceModule: " << e.what() << std::endl;
            return false;
        }
    }

    void InferenceModule::process() {
        cv::Mat frame;
        std::vector<dai::ImgDetection> detections;

        // Get preview frame
        if (preview_queue_) {
            auto previewFrame = preview_queue_->tryGet<dai::ImgFrame>();
            if (previewFrame) {
                frame = previewFrame->getCvFrame();

                if (frame_callback_) {
                    frame_callback_(previewFrame);
                }
            }
        }

        // Get detections
        if (detection_queue_) {
            auto detectionsMsg = detection_queue_->tryGet<dai::ImgDetections>();
            if (detectionsMsg) {
                detections = detectionsMsg->detections;

                if (detection_callback_) {
                    detection_callback_(detectionsMsg);
                }
            }
        }

        // Display with detections overlay
        if (!frame.empty() && show_preview_) {
            drawDetections(frame, detections);

            cv::imshow("Inference", frame);

            int key = cv::waitKey(1);
            if (key == 'q' || key == 'Q' || key == 27) {
                show_preview_ = false;
                cv::destroyWindow("Inference");
            }
        }
    }

    void InferenceModule::drawDetections(cv::Mat& frame,
        const std::vector<dai::ImgDetection>& detections) {
        for (const auto& det : detections) {
            // Calculate bounding box coordinates
            int x1 = static_cast<int>(det.xmin * frame.cols);
            int y1 = static_cast<int>(det.ymin * frame.rows);
            int x2 = static_cast<int>(det.xmax * frame.cols);
            int y2 = static_cast<int>(det.ymax * frame.rows);

            // Clamp to frame bounds
            x1 = std::max(0, std::min(x1, frame.cols - 1));
            y1 = std::max(0, std::min(y1, frame.rows - 1));
            x2 = std::max(0, std::min(x2, frame.cols - 1));
            y2 = std::max(0, std::min(y2, frame.rows - 1));

            // Get label
            std::string label;
            if (det.label < static_cast<int>(labels_.size())) {
                label = labels_[det.label];
            }
            else {
                label = "Class " + std::to_string(det.label);
            }

            // Generate color based on label
            cv::Scalar color(
                (det.label * 50) % 256,
                (det.label * 100 + 100) % 256,
                (det.label * 150 + 50) % 256
            );

            // Draw bounding box
            cv::rectangle(frame, cv::Point(x1, y1), cv::Point(x2, y2), color, 2);

            // Draw label background
            std::string text = label + " " + std::to_string(static_cast<int>(det.confidence * 100)) + "%";
            int baseline;
            cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);

            cv::rectangle(frame,
                cv::Point(x1, y1 - textSize.height - 5),
                cv::Point(x1 + textSize.width + 5, y1),
                color, cv::FILLED);

            // Draw label text
            cv::putText(frame, text, cv::Point(x1 + 2, y1 - 3),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(255, 255, 255), 1);
        }

        // Draw detection count
        std::string count_text = "Detections: " + std::to_string(detections.size());
        cv::putText(frame, count_text, cv::Point(10, 25),
            cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 255, 0), 2);
    }

    void InferenceModule::cleanup() {
        if (show_preview_) {
            cv::destroyAllWindows();
        }

        preview_queue_.reset();
        detection_queue_.reset();
    }

} // namespace oak
