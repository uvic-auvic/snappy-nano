// show_window.cpp
// Subscribes to Image + DetectionArray from Front (D455) and Bottom (D405) cameras
// Displays both feeds side-by-side in a single OpenCV window

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>
#include <message_filters/subscriber.h>
#include <message_filters/synchronizer.h>
#include <message_filters/sync_policies/approximate_time.h>
#include <memory>
#include <mutex>
#include <string>

#include "snappy_cpp/msg/detection_array.hpp"

using namespace std::chrono_literals;

class ShowWindow : public rclcpp::Node
{
public:
    ShowWindow() : Node("show_window")
    {
        // Parameters
        show_detections_ = this->declare_parameter<bool>("show_detections", true);
        show_labels_ = this->declare_parameter<bool>("show_labels", true);
        window_width_ = this->declare_parameter<int>("window_width", 1920);
        
        RCLCPP_INFO(get_logger(), "Show Window initialized");
        RCLCPP_INFO(get_logger(), "Subscribing to /d455 and /d405 feeds...");

        // --- Front Camera (D455) Setup ---
        // We sync the Image and the DetectionArray so they match temporally
        front_image_sub_.subscribe(this, "/d455/color/image_raw");
        front_det_sub_.subscribe(this, "/d455/detections");
        
        front_sync_ = std::make_shared<FrontSyncPolicy>(10); // Queue size 10
        front_sync_->connectInput(front_image_sub_, front_det_sub_);
        front_sync_->registerCallback(
            std::bind(&ShowWindow::front_callback, this, 
                     std::placeholders::_1, std::placeholders::_2));
        
        // --- Bottom Camera (D405) Setup ---
        // Note: D405 often uses rectified images, adjust topic if necessary
        bottom_image_sub_.subscribe(this, "/d405/color/image_rect_raw");
        bottom_det_sub_.subscribe(this, "/d405/detections");
        
        bottom_sync_ = std::make_shared<BottomSyncPolicy>(10);
        bottom_sync_->connectInput(bottom_image_sub_, bottom_det_sub_);
        bottom_sync_->registerCallback(
            std::bind(&ShowWindow::bottom_callback, this, 
                     std::placeholders::_1, std::placeholders::_2));
        
        // Create OpenCV window
        cv::namedWindow("Camera Feeds", cv::WINDOW_NORMAL);
        // Set initial aspect ratio (16:9 approx for combined view)
        cv::resizeWindow("Camera Feeds", window_width_, window_width_ / 2);
    }
    
    ~ShowWindow() override
    {
        cv::destroyAllWindows();
    }

private:
    // Define Sync Policies
    using FrontSyncPolicy = message_filters::sync_policies::ApproximateTime<
        sensor_msgs::msg::Image, 
        snappy_cpp::msg::DetectionArray>;
    
    using BottomSyncPolicy = message_filters::sync_policies::ApproximateTime<
        sensor_msgs::msg::Image, 
        snappy_cpp::msg::DetectionArray>;

    void front_callback(const sensor_msgs::msg::Image::ConstSharedPtr& image_msg,
                       const snappy_cpp::msg::DetectionArray::ConstSharedPtr& det_msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            auto cv_ptr = cv_bridge::toCvShare(image_msg, "bgr8");
            front_image_ = cv_ptr->image.clone();
            front_dets_ = det_msg->detections;
            front_ready_ = true;
            
            attempt_render();
        } catch (const cv_bridge::Exception& e) {
            RCLCPP_ERROR(get_logger(), "Front cv_bridge error: %s", e.what());
        }
    }
    
    void bottom_callback(const sensor_msgs::msg::Image::ConstSharedPtr& image_msg,
                        const snappy_cpp::msg::DetectionArray::ConstSharedPtr& det_msg)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            auto cv_ptr = cv_bridge::toCvShare(image_msg, "bgr8");
            bottom_image_ = cv_ptr->image.clone();
            bottom_dets_ = det_msg->detections;
            bottom_ready_ = true;
            
            attempt_render();
        } catch (const cv_bridge::Exception& e) {
            RCLCPP_ERROR(get_logger(), "Bottom cv_bridge error: %s", e.what());
        }
    }
    
    void attempt_render()
    {
        // Only render if we have received data from BOTH cameras
        if (!front_ready_ || !bottom_ready_) return;
        
        // 1. Resize both images to same height for clean side-by-side
        int target_height = 480; 
        cv::Mat front_resized, bottom_resized;
        
        resize_keep_aspect(front_image_, front_resized, target_height);
        resize_keep_aspect(bottom_image_, bottom_resized, target_height);
        
        // 2. Draw detections on the resized images
        if (show_detections_) {
            draw_boxes(front_resized, front_dets_, "FRONT");
            draw_boxes(bottom_resized, bottom_dets_, "BOTTOM");
        }
        
        // 3. Add Camera Labels
        if (show_labels_) {
            put_label(front_resized, "D455 (Front)");
            put_label(bottom_resized, "D405 (Bottom)");
        }
        
        // 4. Concatenate Horizontally
        cv::Mat combined;
        cv::hconcat(front_resized, bottom_resized, combined);
        
        // 5. Display
        cv::imshow("Camera Feeds", combined);
        cv::waitKey(1); // Required for OpenCV to update window
    }
    
    void resize_keep_aspect(const cv::Mat& src, cv::Mat& dst, int target_height)
    {
        float scale = static_cast<float>(target_height) / src.rows;
        int target_width = static_cast<int>(src.cols * scale);
        cv::resize(src, dst, cv::Size(target_width, target_height));
    }
    
    void draw_boxes(cv::Mat& img, 
                    const std::vector<snappy_cpp::msg::ObjectDetection>& dets,
                    const std::string& prefix)
    {
        for (const auto& det : dets) {
            const auto& bbox = det.bbox;
            
            // Safety check for valid coordinates
            if (bbox.x_max <= bbox.x_min || bbox.y_max <= bbox.y_min) continue;
            
            // Draw Rectangle
            cv::rectangle(img, 
                         cv::Point(static_cast<int>(bbox.x_min), static_cast<int>(bbox.y_min)), 
                         cv::Point(static_cast<int>(bbox.x_max), static_cast<int>(bbox.y_max)),
                         cv::Scalar(0, 255, 0), 2);
            
            if (show_labels_) {
                std::string text = prefix + " C:" + std::to_string(det.class_id) + 
                                 " (" + std::to_string(int(det.confidence * 100)) + "%)";
                
                // Text Background
                int baseline = 0;
                cv::Size text_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 
                                                    0.5, 1, &baseline);
                
                cv::rectangle(img, 
                             cv::Point(static_cast<int>(bbox.x_min), static_cast<int>(bbox.y_min) - text_size.height - 5),
                             cv::Point(static_cast<int>(bbox.x_min) + text_size.width, static_cast<int>(bbox.y_min)),
                             cv::Scalar(0, 255, 0), -1); // Filled rectangle
                
                // Text
                cv::putText(img, text, 
                           cv::Point(static_cast<int>(bbox.x_min), static_cast<int>(bbox.y_min) - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                           cv::Scalar(0, 0, 0), 1);
            }
        }
    }
    
    void put_label(cv::Mat& img, const std::string& text)
    {
        cv::putText(img, text, 
                   cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 
                   1.0, cv::Scalar(0, 255, 255), 2);
    }
    
    // Subscribers & Sync
    message_filters::Subscriber<sensor_msgs::msg::Image> front_image_sub_;
    message_filters::Subscriber<snappy_cpp::msg::DetectionArray> front_det_sub_;
    std::shared_ptr<FrontSyncPolicy> front_sync_;
    
    message_filters::Subscriber<sensor_msgs::msg::Image> bottom_image_sub_;
    message_filters::Subscriber<snappy_cpp::msg::DetectionArray> bottom_det_sub_;
    std::shared_ptr<BottomSyncPolicy> bottom_sync_;
    
    // State
    std::mutex mutex_;
    cv::Mat front_image_, bottom_image_;
    std::vector<snappy_cpp::msg::ObjectDetection> front_dets_, bottom_dets_;
    bool front_ready_ = false;
    bool bottom_ready_ = false;
    
    // Params
    bool show_detections_ = true;
    bool show_labels_ = true;
    int window_width_ = 1920;
};

int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<ShowWindow>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}