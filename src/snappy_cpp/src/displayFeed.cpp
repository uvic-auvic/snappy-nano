    void render_display(const CameraContext& cam, const cv::Mat& color_image,
                        const DetectionResult& det_result)
    {
        cv::Mat annotated = color_image.clone();
        for (const auto& d : det_result.detections) {
            cv::Rect rect((int)d.box.x, (int)d.box.y, (int)d.box.width, (int)d.box.height);
            cv::rectangle(annotated, rect, cv::Scalar(0, 255, 0), 2);

            std::string distance_str = (d.distance_m > 0) ?
                std::to_string((int)(d.distance_m * 100)) + "cm" : "N/A";
            std::string label = CLASS_NAMES[d.class_id] + " " +
                                std::to_string((int)(d.conf * 100)) + "% [" + distance_str + "]";

            int baseLine = 0;
            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, &baseLine);
            int textX = (int)d.box.x;
            int textY = (int)d.box.y - 5;
            cv::rectangle(annotated,
                cv::Point(textX, textY - textSize.height - baseLine - 4),
                cv::Point(textX + textSize.width + 4, textY + baseLine),
                cv::Scalar(0, 255, 0), cv::FILLED);
            cv::putText(annotated, label, cv::Point(textX + 2, textY - baseLine - 2),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
        }

        std::string info = "Infer: " + std::to_string((int)det_result.inference_time_ms) +
                           "ms (GPU)  det: " + std::to_string(det_result.detections.size());
        cv::putText(annotated, info, {10, 30},
            cv::FONT_HERSHEY_SIMPLEX, 0.8, {0, 255, 255}, 2, cv::LINE_AA);

        cv::imshow(cam.display_window_name, annotated);
        cv::moveWindow(cam.display_window_name, static_cast<int>(cam.index) * 680, 0);
        if (cv::waitKey(1) == 27) {
            RCLCPP_INFO(get_logger(), "ESC pressed, shutting down");
            rclcpp::shutdown();
        }
    }