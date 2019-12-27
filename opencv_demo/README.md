# MMAL-OPENCV4 DEMO

Simple Raspberry Pi MMAL-OPENCV4 project. Based on [tasanakorn's work](https://github.com/tasanakorn/rpi-mmal-demo). The main difference lays in the fact this is for raspbian lite (no monitor) and independent from [userland](https://github.com/raspberrypi/userland.git). Another modification is the opencv_demo has been updated to work with opencv4. This demo captures the Video Port output and loads the raw data into a cv::Mat type to process that image with opencv.

## Getting Started
If you already have install OpenCV, modified Makefile. Following this steps: open it and look up the following line:
```
CFLAGS = `pkg-config --cflags --libs /installation/OpenCV-/lib/pkgconfig/opencv4.pc`
```
Change it for the directory where you installed opencv4.pc
```
CFLAGS = `pkg-config --cflags --libs /path/to/your/opencv4.pc`
```
then run
```
make
```
and you are ready to go.

## Requirements
There are two ways to install opencv: by apt-get or by compiling it:

### apt-get
simply type this on terminal
```
sudo apt-get install libopencv-dev
```
I have never installed opencv this way in any platform either raspberry, laptops or desktops.

### compiling 
This is quiet tricky, I'm writting a blog on this, but for now you can follow either the [official OpenCV tutorial](https://docs.opencv.org/4.1.2/d7/d9f/tutorial_linux_install.html) or [this one](https://www.learnopencv.com/install-opencv-4-on-raspberry-pi/) by Learnopencv.com. In the last one I recommend to add this flag when running CMake:
```
-DOPENCV_GENERATE_PKGCONFIG=ON
```
I also recommend to use two cores at compile time instead of the 4 the raspberry has
```
make -j2
```
This installed **opencv4.pc**, this used to call the opencv library when compiling by Makefile
## Acknowledgement 
* [Multi-Media Abstraction Layer (MMAL)](http://www.jvcref.com/files/PI/documentation/html/index.html). Draft Version 0.1.
* Picamera, [Camera Harware](https://picamera.readthedocs.io/en/release-1.10/fov.html) 
* OpenCV4 docs, [OpenCV modules](https://docs.opencv.org/4.1.2/index.html)
