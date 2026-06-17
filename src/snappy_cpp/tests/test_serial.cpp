// Bring in my package's API, which is what I'm testing
#include "../src/include/Inc/serial.h"
// Bring in gtest
#include <gtest/gtest.h>
#include "rclcpp/rclcpp.hpp"

// Declare a test
TEST(Serial, constructors)
{
    serial s1;
    EXPECT_EQ(s1.getPortName(), "/dev/ttyTHS1");
    EXPECT_EQ(s1.getBaudRate(), 11520);

    serial s2("port", 69);
    EXPECT_EQ(s2.getPortName(), "port");
    EXPECT_EQ(s2.getBaudRate(), 69);
}

// Run all the tests that were declared with TEST()
int main(int argc, char **argv){
    testing::InitGoogleTest(&argc, argv);
    rclcpp::init(argc, argv);
    int result = RUN_ALL_TESTS();
    rclcpp::shutdown();
    return result;
}