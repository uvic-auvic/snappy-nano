#include <boost/asio.hpp>
#include <iostream>
#include <string>

class serial {

    public:
        serial();
        serial(const std::string& portname, unsigned int baud_rate);

        // Function to read data from the serial port
        std::string readFromSerialPort();

        // Function to write data to the serial port
        void writeToSerialPort(const std::string& message);

        int getBaudRate();

        std::string getPortName();

    private:
        boost::asio::io_context _io;
        boost::asio::serial_port _serial;
        const std::string _portname;
        const unsigned int _baud_rate;

};