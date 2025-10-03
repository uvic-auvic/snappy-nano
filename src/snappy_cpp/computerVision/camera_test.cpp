#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    try {
        // Create a RealSense pipeline
        rs2::pipeline pipe;
        
        // Create a configuration for configuring the pipeline with non-default profile
        rs2::config cfg;
        
        // Configure the pipeline to stream color frames
        cfg.enable_stream(RS2_STREAM_COLOR, 640, 480, RS2_FORMAT_BGR8, 30);
        cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
        
        // Start streaming with the configuration
        pipe.start(cfg);
        
        std::cout << "RealSense camera started successfully!" << std::endl;
        std::cout << "Press 'q' to quit or ESC to exit." << std::endl;
        
        //create a colorizer for depth frames with hisstogram equalization
        rs2::colorizer color_map;
        color_map.set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 1);

        // Create OpenCV window
        cv::namedWindow("RealSense Color Stream", cv::WINDOW_AUTOSIZE);
        //cv::namedWindow("RealSense Depth Stream", cv::WINDOW_AUTOSIZE);

        while (true) {
            // Wait for a coherent pair of frames: depth and color
            rs2::frameset frames = pipe.wait_for_frames();
            
            // Get the color frame
	    rs2::frame color_frame = frames.get_color_frame();
            rs2::frame depth_frame = frames.get_depth_frame();
	    rs2::depth_frame depth = frames.get_depth_frame();

            // Apply color map to depth frame
            rs2::frame depth_colorized = color_map.process(depth_frame);

            // Convert RealSense frame to OpenCV Mat
            const int w = color_frame.as<rs2::video_frame>().get_width();
            const int h = color_frame.as<rs2::video_frame>().get_height();

	    int width = depth.get_width();
	    int height = depth.get_height();

	    float dist_to_center = depth.get_distance(width / 2, height / 2);
            
            cv::Mat color_image(cv::Size(w, h), CV_8UC3, (void*)color_frame.get_data(), cv::Mat::AUTO_STEP);
            cv::Mat depth_colorized_image(cv::Size(w, h), CV_8UC3, (void*)depth_colorized.get_data(), cv::Mat::AUTO_STEP);

            // Display the images
            cv::imshow("RealSense Color Stream", color_image);
            cv::imshow("RealSense Depth Colorized Stream", depth_colorized_image);

	    std::cout << "The camera is facing an object " << dist_to_center << " meters away \r";
            
            // Check for key press
            char key = cv::waitKey(1);
            if (key == 'q' || key == 'Q' || key == 27) { // 'q' or ESC key
                break;
            }
        }
        
        // Clean up
        cv::destroyAllWindows();
        
    } catch (const rs2::error & e) {
        std::cerr << "RealSense error calling " << e.get_failed_function() 
                  << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
        return EXIT_FAILURE;
    } catch (const std::exception & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}
