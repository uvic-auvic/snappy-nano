#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string>
#include <chrono>

class PressureSensorNode : public rclcpp::Node
{
public:
    PressureSensorNode() : Node("pressure_sensor_node"), serial_fd_(-1)
    {
        publisher_ = this->create_publisher<std_msgs::msg::String>("pressure_data", 10);
        
        // Open serial port with non-blocking flag
        serial_fd_ = open("/dev/ttyUSB0", O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (serial_fd_ < 0) {
            RCLCPP_ERROR(this->get_logger(), "Failed to open /dev/ttyUSB0");
            return;
        }
        
        // Configure serial port
        struct termios tty;
        tcgetattr(serial_fd_, &tty);
        //this is the line to set baud rate, B9600 is what the arduino code from blue robotics is set to 
        // thus it must match
        cfsetispeed(&tty, B9600);
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_oflag &= ~OPOST;
        tcsetattr(serial_fd_, TCSANOW, &tty);
        
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&PressureSensorNode::read_and_publish, this));
        // this is here just for debugging purposes    
        RCLCPP_INFO(this->get_logger(), "Pressure sensor node started");
    }

    ~PressureSensorNode() {
        if (serial_fd_ >= 0) {
            close(serial_fd_);
        }
    }

private:
    void read_and_publish()
    {
        if (serial_fd_ >= 0) {
            char buffer[256];
            int bytes_read = read(serial_fd_, buffer, sizeof(buffer) - 1);
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                std::string data(buffer);
                
                auto message = std_msgs::msg::String();
                message.data = data;
                publisher_->publish(message);
                RCLCPP_INFO(this->get_logger(), "Published: %s", data.c_str());
            } else if (bytes_read == 0) {
                // No data available
                RCLCPP_INFO(this->get_logger(), "No data available");
            } else {
                // Error occurred
                RCLCPP_WARN(this->get_logger(), "Read error from serial port");
            }
        }
    }
    
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    int serial_fd_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    // Create and spin the node
    auto node = std::make_shared<PressureSensorNode>();
    rclcpp::spin(node);
    // shutdown for cleanup and ctrl+c handling
    rclcpp::shutdown();
    return 0;
}

