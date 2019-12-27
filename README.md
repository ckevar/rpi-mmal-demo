# MMAL DEMO

Simple Raspberry Pi MMAL project. Based on [tasanakorn's work](https://github.com/tasanakorn/rpi-mmal-demo). The main difference lays in the fact this is for raspbian lite (no monitor) and independent from [userland](https://github.com/raspberrypi/userland.git). Another modification is the opencv_demo has been updated to work with opencv4.

## Getting Started

Most of the files here do not require another library, but one, that is inside the opencv_demo folder. 

By typing the fallowing on terminal it'll create the executables for each demo

```
make
```
* *buffer_demo* Captures Video port output in a buffer
* *main* Connects camera component and preview component to watch the video (no tested, but compiles withou errors)
* *video_record* the camera video port output is caught and sent to h264-Encoder, the output enconder can be save in a file like this
```
./video_record > my_video.h264
```
Remember, h264 is an encoder not a container. In order to play the video use VLC, and you can use ffmpeg for wrapping it into a MP4 container.

## Requirements
Install the kernel headers in case it's not
```
sudo apt-get install raspberrypi-kernel
```
for OpenCV, enter to the folder for more details

## Acknowledgement 
* [Multi-Media Abstraction Layer (MMAL)](http://www.jvcref.com/files/PI/documentation/html/index.html). Draft Version 0.1.
* Picamera, [Camera Harware](https://picamera.readthedocs.io/en/release-1.10/fov.html) 
* OpenCV4 docs, [OpenCV modules](https://docs.opencv.org/4.1.2/index.html)
