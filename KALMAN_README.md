
Build: colcon build --packages-select snappy_cpp
Run test_kalman (need to have .csv files):  ./build/snappy_cpp/test_kalman 

Once you run test_kalman can graph and compare using: ./plot_kalman.py state_estimator_outputs/kalman_1783280617402906537.csv kalman_replay_1783280617402906537.csv

2D over head plot: ./plot_path.py state_estimator_outputs/kalman_1783280617402906537.csv --trim --heading-up
