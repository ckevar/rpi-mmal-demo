# MMAL DEMO

Simple Raspberry Pi MMAL project. Based on [tasanakorn's work](https://github.com/tasanakorn/rpi-mmal-demo). The main difference lays in the fact this is for raspbian lite (no monitor) and independent from [userland](https://github.com/raspberrypi/userland.git). Another modification is the opencv_demo has been updated to work with opencv4.

## Getting Started

Most of the files here do not require another library, but one, that is inside the opencv_demo folder. 

By typing the fallowing on terminal it'll create the executables for each demo

```
make
```
* *buffer_demo*    
    
# Acknowledgement 
* [Multi-Media Abstraction Layer (MMAL)](http://www.jvcref.com/files/PI/documentation/html/index.html). Draft Version 0.1.
* Picamera, [Camera Harware](https://picamera.readthedocs.io/en/release-1.10/fov.html) 
* OpenCV4 docs, [OpenCV modules](https://docs.opencv.org/4.1.2/index.html)
