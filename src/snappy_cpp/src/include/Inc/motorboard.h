#ifndef MOTORBOARD_HPP
#define QUEUE_HPP

namespace motor{
    template <class T>
    class motorboard {
        public:

            using value_type = T;

            using size_type = std::size_t;

            enum class turbine {
                front = 0,
                back = 1,
                left = 2,
                right = 3,
                front_left = 4,
                front_right = 5,
                back_left = 6,
                back_right = 7,
            };

            motorboard() = delete;

            motorboard(const size_type& baudrate, const std::string& port);

            // Initialize the motorboard
            void on();
            
            // Uninitialize the motorboard
            void off();

            // Set all motor speeds to 0
            void stop();

            void forward(const int8_t& speed);

            void backward(const int8_t& speed);

            void right(const int8_t& speed);

            void left(const int8_t& speed);

            void up(const int8_t& speed);

            void down(const int8_t& speed);

            void yaw(const int8_t& speed);

            void roll(const int8_t& speed);

            void pitch(const int8_t& speed);

            /*
            8 integers:
            0: Front
            1: Back
            2: Left
            3: Right
            4: Front left
            5: Front right
            6: Back left
            7: Back right
            */
            void set_motors(const int8_t* speeds);

            void set_leds(const int& led, const int& mode); // Which LED and blink mode
    };
}

#endif //MOTORBOARD_HPP