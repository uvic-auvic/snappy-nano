#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float32.hpp>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string>
#include <sstream>
#include <chrono>

class DepthSensorNode : public rclcpp::Node
{
public:
    DepthSensorNode() : Node("depth_sensor_node"), serial_fd_(-1), read_buffer_("")
    {
        // Publishes a plain float — your state estimator reads this directly
        publisher_ = this->create_publisher<std_msgs::msg::Float32>("depth_data", 10);

        serial_fd_ = open("/dev/ttyUSB0", O_RDONLY | O_NOCTTY | O_NONBLOCK);
        if (serial_fd_ < 0) {
            RCLCPP_ERROR(this->get_logger(), "Failed to open /dev/ttyUSB0");
            return;
        }

        struct termios tty;
        tcgetattr(serial_fd_, &tty);

        // Arduino sketch uses 9600 baud — must match
        cfsetispeed(&tty, B115200);
        cfsetospeed(&tty, B115200);

        // this enables the reciever and ignores the control lines
        tty.c_cflag |= (CLOCAL | CREAD);
        // this ensures no parity bit
        tty.c_cflag &= ~PARENB;
        // this ensures 1 stop bit
        tty.c_cflag &= ~CSTOPB;

        // this ensures 8 data bits
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;

        // Canonical mode: read() returns one complete line at a time (\n terminated)
        // This prevents partial reads and is exactly how the Python node works
        // kernel buffers bytes until a newline is received
        tty.c_lflag |= ICANON;
        // no echo, no special characters signals (ctrl + c and more won't break it)
        tty.c_lflag &= ~(ECHO | ECHOE | ISIG);

        // disable the software flow control (XON/XOFF) and other special characters
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);

        // output processing
        // raw output no \n translation -> \r\n translation on send
        tty.c_oflag &= ~OPOST;
        // this ensure no drain wait, and ensures changes take effect immediately
        tcsetattr(serial_fd_, TCSANOW, &tty);

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&DepthSensorNode::read_and_publish, this));

        RCLCPP_INFO(this->get_logger(), "Depth sensor node started on /dev/ttyUSB0 at  baud");
        // RCLCPP_INFO(this->get_logger(), "HIT");
    }

    ~DepthSensorNode()
    {
        if (serial_fd_ >= 0) {
            close(serial_fd_);
        }
    }

private:
    // Parses "D 1.23\n" → 1.23, returns false if format doesn't match
    bool parse_depth_line(const std::string & line, double & depth_out)
    {
        // Expected format from Arduino: "D <value>"
        const std::string prefix = "D ";

        // RCLCPP_INFO(this->get_logger(), "HIT1");
        if (line.rfind(prefix, 0) != 0) {
            // RCLCPP_INFO(this->get_logger(), "HIT2");
            return false;  // Line doesn't start with "D "
        }

        std::string trimmed = line;
        // Strip trailing whitespace / \r
        while (!trimmed.empty() && (trimmed.back() == '\r' || trimmed.back() == '\n' || trimmed.back() == ' ')) {
            trimmed.pop_back();
            // RCLCPP_INFO(this->get_logger(), "HIT3");
        }

        if (trimmed.size() <= prefix.size()) {
            // RCLCPP_INFO(this->get_logger(), "HIT4");
            return false;
        }

        // Extract the numeric part
        std::string num_str = trimmed.substr(prefix.size());
            // RCLCPP_INFO(this->get_logger(), "HIT5");

        try {
            depth_out = std::stod(num_str);
            // RCLCPP_INFO(this->get_logger(), "HIT6");
            return true;
        } catch (...) {
            // RCLCPP_INFO(this->get_logger(), "HIT7");
            return false;
        }
    }

    void read_and_publish()
    {

        //RCLCPP_INFO(this->get_logger(), "serial_fd_: %d", serial_fd_);
        if (serial_fd_ < 0) return;

        char buffer[256];
        // In canonical mode, read() blocks until a full line (\n) is ready
        // and returns exactly that line — no partial read issues
	std::string latest_line;
	bool got_line = false;
	while(true) {
		int bytes_read = read(serial_fd_, buffer, sizeof(buffer) - 1);
		if(bytes_read > 0) {
		buffer[bytes_read] = '\0';
		latest_line = buffer;
		got_line = true;
		continue;
		}

		if (bytes_read < 0) {
		    if(errno == EAGAIN || errno == EWOULDBLOCK)break; 
			// No data available right now, not an error
		    RCLCPP_DEBUG(this->get_logger(), "No data available to read");
		    return;
		}
		break;
	}

	if (!got_line) return;

        double depth = 0.0;
        if (parse_depth_line(latest_line, depth)) {
//            RCLCPP_INFO(this->get_logger(), "HIT8");
            auto message = std_msgs::msg::Float32();
            message.data = depth;
            publisher_->publish(message);
            RCLCPP_INFO(this->get_logger(), "Depth: %.4f m", depth);
        } else {
            // Silently ignore non-depth lines (e.g. "Starting" on boot)
            RCLCPP_DEBUG(this->get_logger(), "Ignored line: %s", latest_line.c_str());
            // RCLCPP_INFO(this->get_logger(), "HIT9");
        }
    }

    rclcpp::Publisher<std_msgs::msg::Float32>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    int serial_fd_;
    std::string read_buffer_;
};

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DepthSensorNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
