# Running the Jetson in Headless Mode
This disables the GUI and saves RAM.
Based on this webpage: [NVIDIA Jetson Orin Nano headless setup and pytorch](https://codepitbull.medium.com/nvidia-jetson-orin-nano-headless-setup-and-pytorch-cf99cd34523a)

## 1. Connect to Jetson via SSH
run ``ssh username@jetsonIP``\
and enter the password

## Disable the GUI
Disabling the GUI forces the Jetson to run in text-only mode, saving RAM.
run ``sudo systemctl set-default multi-user.target``

## 3. Reboot
run ``sudo reboot``

## 4. Turn the GUI Back On
To turn the GUI back on once headless mode is no longer needed, run ``sudo systemctl set-default graphical.target``

### WARNING
Closing the SSH window or losing Wifi kills any scripts that may be running
