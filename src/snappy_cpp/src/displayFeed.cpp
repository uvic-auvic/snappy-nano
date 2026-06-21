// displayFeed.cpp
// Standalone manual-launch visualization node. Subscribes to BOTH the front
// (D455) and bottom (D405) camera image + detection topics, annotates each
// frame independently, stacks them vertically, and shows the combined result
// in a single OpenCV window.
//
// Run manually (NOT part of the main launch file):
//   ros2 run snappy_cpp displayFeed

#include <rclcpp/rclcpp.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>

#include <mutex>
#include <string>
#include <vector>

#include <sensor_msgs/msg/image.hpp>
#include "snappy_cpp/msg/detection_array.hpp"

class DisplayFeed : public rclcpp::Node
{
public:
    DisplayFeed()
    : Node("display_feed")
    {
        display_window_name_ = "snappy_camera_feeds";
        display_width_ = this->declare_parameter<int>("display_width", 960);

        cv::namedWindow(display_window_name_, cv::WINDOW_AUTOSIZE);

        // Front camera (D455)
        front_image_sub_.subscribe(this, "/d455/color/image_raw", rclcpp::SensorDataQoS().get_rmw_qos_profile());
        front_detection_sub_.subscribe(this, "/d455/detections", rclcpp::QoS(10).get_rmw_qos_profile());
        front_sync_ = std::make_shared<Synchronizer>(SyncPolicy(10), front_image_sub_, front_detection_sub_);
        front_sync_->registerCallback(
            std::bind(&DisplayFeed::front_callback, this,
                      std::placeholders::_1, std::placeholders::_2));

        // Bottom camera (D405)
        bottom_image_sub_.subscribe(this, "/d405/color/image_rect_raw", rclcpp::SensorDataQoS().get_rmw_qos_profile());
        bottom_detection_sub_.subscribe(this, "/d405/detections", rclcpp::QoS(10).get_rmw_qos_profile());
        bottom_sync_ = std::make_shared<Synchronizer>(SyncPolicy(10), bottom_image_sub_, bottom_detection_sub_);
        bottom_sync_->registerCallback(
            std::bind(&DisplayFeed::bottom_callback, this,
                      std::placeholders::_1, std::placeholders::_2));

        // Render on a timer rather than directly in each callback, so a slow
        // or missing feed on one camera doesn't block updates from the other.
        render_timer_ = this->create_wall_timer(
            std::chrono::milliseconds(33),  // ~30Hz display refresh
            std::bind(&DisplayFeed::render_combined, this));

        RCLCPP_INFO(get_logger(), "DisplayFeed ready (front: /d455, bottom: /d405)");
    }

    ~DisplayFeed() override
    {
        cv::destroyWindow(display_window_name_);
    }

private:
    using SyncPolicy = message_filters::sync_policies::ApproximateTime<
        sensor_msgs::msg::Image, snappy_cpp::msg::DetectionArray>;
    using Synchronizer = message_filters::Synchronizer<SyncPolicy>;

    // ---- Per-camera annotation (shared logic) ------------------------------

    static cv::Mat annotate_frame(
        const sensor_msgs::msg::Image::ConstSharedPtr & image_msg,
        const snappy_cpp::msg::DetectionArray::ConstSharedPtr & det_msg,
        const rclcpp::Logger & logger)
    {
        cv::Mat annotated;
        try {
            annotated = cv_bridge::toCvShare(image_msg, "bgr8")->image.clone();
        } catch (const cv_bridge::Exception & e) {
            RCLCPP_ERROR(logger, "cv_bridge error: %s", e.what());
            return cv::Mat();
        }

        for (const auto & d : det_msg->detections) {
            cv::Rect rect(
                static_cast<int>(d.bounding_box.x),
                static_cast<int>(d.bounding_box.y),
                static_cast<int>(d.bounding_box.width),
                static_cast<int>(d.bounding_box.height));
            cv::rectangle(annotated, rect, cv::Scalar(0, 255, 0), 2);

            for (const auto & poly : d.mask_polygons) {
                if (poly.points.size() < 2) continue;
                std::vector<cv::Point> pts;
                pts.reserve(poly.points.size());
                for (const auto & p : poly.points) {
                    pts.emplace_back(static_cast<int>(p.x), static_cast<int>(p.y));
                }
                const cv::Point * pts_data = pts.data();
                int npts = static_cast<int>(pts.size());
                cv::polylines(annotated, &pts_data, &npts, 1, true, cv::Scalar(0, 200, 255), 2, cv::LINE_AA);
            }

            std::string distance_str = (d.distance_m > 0.0f)
                ? std::to_string(static_cast<int>(d.distance_m * 100.0f)) + "cm"
                : "N/A";
            std::string label = d.object_class + " " +
                std::to_string(static_cast<int>(d.confidence * 100.0f)) + "% [" + distance_str + "]";

            int baseLine = 0;
            cv::Size textSize = cv::getTextSize(label, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, &baseLine);
            int textX = static_cast<int>(d.bounding_box.x);
            int textY = static_cast<int>(d.bounding_box.y) - 5;

            cv::rectangle(annotated,
                cv::Point(textX, textY - textSize.height - baseLine - 4),
                cv::Point(textX + textSize.width + 4, textY + baseLine),
                cv::Scalar(0, 255, 0), cv::FILLED);
            cv::putText(annotated, label, cv::Point(textX + 2, textY - baseLine - 2),
                cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);
        }

        std::string info = "det: " + std::to_string(det_msg->detections.size()) +
                           "  inference: " + std::to_string(det_msg->inference_time_ms) + "ms";
        cv::putText(annotated, info, {10, 30},
            cv::FONT_HERSHEY_SIMPLEX, 0.8, {0, 255, 255}, 2, cv::LINE_AA);

        return annotated;
    }

    // ---- Per-camera callbacks: just cache the latest annotated frame -------

    void front_callback(
        const sensor_msgs::msg::Image::ConstSharedPtr & image_msg,
        const snappy_cpp::msg::DetectionArray::ConstSharedPtr & det_msg)
    {
        cv::Mat annotated = annotate_frame(image_msg, det_msg, get_logger());
        if (annotated.empty()) return;
        std::lock_guard<std::mutex> lock(frame_mutex_);
        front_frame_ = std::move(annotated);
    }

    void bottom_callback(
        const sensor_msgs::msg::Image::ConstSharedPtr & image_msg,
        const snappy_cpp::msg::DetectionArray::ConstSharedPtr & det_msg)
    {
        cv::Mat annotated = annotate_frame(image_msg, det_msg, get_logger());
        if (annotated.empty()) return;
        std::lock_guard<std::mutex> lock(frame_mutex_);
        bottom_frame_ = std::move(annotated);
    }

    // ---- Combine + display --------------------------------------------------

    void render_combined()
    {
        cv::Mat front_copy, bottom_copy;
        {
            std::lock_guard<std::mutex> lock(frame_mutex_);
            front_copy = front_frame_;
            bottom_copy = bottom_frame_;
        }

        // Note: deliberately NOT returning early when both are empty —
        // the window should appear immediately on startup showing "no
        // signal" placeholders, rather than waiting for the first frame
        // from either camera.

        cv::Mat front_resized = resize_to_width(front_copy, display_width_, "FRONT (D455)");
        cv::Mat bottom_resized = resize_to_width(bottom_copy, display_width_, "BOTTOM (D405)");

        cv::Mat combined;
        cv::vconcat(front_resized, bottom_resized, combined);

        cv::imshow(display_window_name_, combined);
        if (cv::waitKey(1) == 27) {
            RCLCPP_INFO(get_logger(), "ESC pressed, shutting down");
            rclcpp::shutdown();
        }
    }

    // Resize (or synthesize a placeholder if empty) to a fixed display width,
    // preserving aspect ratio, and label which camera it is.
    cv::Mat resize_to_width(const cv::Mat & frame, int target_width, const std::string & label) const
    {
        cv::Mat out;
        if (frame.empty()) {
            // No frame received yet for this camera — show a placeholder
            // rather than skipping it, so the stack size stays consistent.
            out = cv::Mat(target_width * 9 / 16, target_width, CV_8UC3, cv::Scalar(40, 40, 40));
            cv::putText(out, label + " - no signal", {20, out.rows / 2},
                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 255), 2, cv::LINE_AA);
            return out;
        }

        double scale = static_cast<double>(target_width) / frame.cols;
        cv::resize(frame, out, cv::Size(target_width, static_cast<int>(frame.rows * scale)));
        cv::putText(out, label, {10, 20},
            cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
        return out;
    }

    std::string display_window_name_;
    int display_width_ = 960;

    message_filters::Subscriber<sensor_msgs::msg::Image> front_image_sub_;
    message_filters::Subscriber<snappy_cpp::msg::DetectionArray> front_detection_sub_;
    std::shared_ptr<Synchronizer> front_sync_;

    message_filters::Subscriber<sensor_msgs::msg::Image> bottom_image_sub_;
    message_filters::Subscriber<snappy_cpp::msg::DetectionArray> bottom_detection_sub_;
    std::shared_ptr<Synchronizer> bottom_sync_;

    rclcpp::TimerBase::SharedPtr render_timer_;

    std::mutex frame_mutex_;
    cv::Mat front_frame_;
    cv::Mat bottom_frame_;
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DisplayFeed>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
