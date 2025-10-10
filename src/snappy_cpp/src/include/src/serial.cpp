#include "../Inc/serial.h"

using namespace std;
using namespace boost;

serial::serial(): _serial(_io), _portname("/dev/ttyTHS1"), _baud_rate(115200)
{
    _serial.open(_portname);
    // Set the baud rate
    _serial.set_option(
        asio::serial_port_base::baud_rate(_baud_rate));
}

serial::serial(const string& portname, unsigned int baud_rate): _serial(_io), _portname(portname), _baud_rate(baud_rate)
{
    _serial.open(_portname);
    // Set the baud rate
    _serial.set_option(
        asio::serial_port_base::baud_rate(_baud_rate));
}

// Function to read data from the serial port
string serial::readFromSerialPort()
{
    char buffer[100]; // Buffer to store incoming data
    system::error_code ec;
    // Read data from the serial port
    size_t len = asio::read(_serial, asio::buffer(buffer), ec);
    if (ec) {
        cerr << "Error reading from serial port: "
            << ec.message() << endl;
        return "";
    }
    // Return the read data as a string
    return string(buffer, len);
}

// Function to write data to the serial port
void serial::writeToSerialPort(const string& message)
{
    system::error_code ec;
    // Write data to the serial port
    asio::write(_serial, asio::buffer(message), ec);

    if (ec) {
        cerr << "Error writing to serial port: "
            << ec.message() << endl;
    }
}

int serial::getBaudRate()
{
    return _baud_rate;
}

string serial::getPortName()
{
    return _portname;
}

