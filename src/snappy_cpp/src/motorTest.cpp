#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

int main() {
    // Open serial port - equivalent to: ser = serial.Serial('/dev/ttyTHS1',115200, timeout=3)
    int serial_port = open("/dev/ttyTHS1", O_RDWR);
    if (serial_port < 0) {
        std::cerr << "Error: Could not open serial port /dev/ttyTHS1" << std::endl;
        return -1;
    }
    
    // Configure the serial port settings
    struct termios tty;
    if (tcgetattr(serial_port, &tty) != 0) {
        std::cerr << "Error: Could not get terminal attributes" << std::endl;
        close(serial_port);
        return -1;
    }
    
    // Set baud rate to 115200
    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);
    
    // Configure 8N1 (8 data bits, no parity, 1 stop bit)
    tty.c_cflag &= ~PARENB;        // No parity
    tty.c_cflag &= ~CSTOPB;        // One stop bit
    tty.c_cflag &= ~CSIZE;         // Clear size bits
    tty.c_cflag |= CS8;            // 8 bits per byte
    tty.c_cflag &= ~CRTSCTS;       // Disable hardware flow control
    tty.c_cflag |= CREAD | CLOCAL; // Enable reading and ignore control lines
    
    // Set timeout to 3 seconds
    tty.c_cc[VTIME] = 30;    // 3 second timeout (30 deciseconds)
    tty.c_cc[VMIN] = 0;      // Return as soon as data is available
    
    // Apply configuration
    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        std::cerr << "Error: Could not set terminal attributes" << std::endl;
        close(serial_port);
        return -1;
    }
    
    ////////////////////////#### Important ###########################
    // Set byte = 'Y' for ASCII version of a char or byte = 0b00011000 to set bits directly
    /////////////////////////////////////////////////////////////////
    
    // Change the message here - equivalent to Python's ord('I'), ord('N'), ord('I')
    unsigned char byte1 = 'I';  // Byte 1 to be sent
    unsigned char byte2 = 'N';  // Byte 2 to be sent  
    unsigned char byte3 = 'I';  // Byte 3 to be sent
    
    // Alternative: You can also set bytes directly with binary or hex values:
    // unsigned char byte1 = 0b01001001;  // Binary for 'I' (73 decimal)
    // unsigned char byte2 = 0x4E;        // Hex for 'N' (78 decimal)
    // unsigned char byte3 = 73;          // Decimal for 'I'
    
    // Create data array - equivalent to: data_to_send = bytes([byte1, byte2, byte3])
    unsigned char data_to_send[3] = {byte1, byte2, byte3};
    
    // Send data - equivalent to: ser.write(data_to_send)
    ssize_t bytes_written = write(serial_port, data_to_send, 3);
    
    if (bytes_written < 0) {
        std::cerr << "Error: Failed to write to serial port" << std::endl;
        close(serial_port);
        return -1;
    } else if (bytes_written != 3) {
        std::cout << "Warning: Only wrote " << bytes_written << " out of 3 bytes" << std::endl;
    } else {
        std::cout << "Successfully sent 3 bytes: '" 
                  << (char)byte1 << (char)byte2 << (char)byte3 
                  << "' to /dev/ttyTHS1" << std::endl;
    }
    
    // Clean up - close the serial port
    close(serial_port);
    
    return 0;
}
