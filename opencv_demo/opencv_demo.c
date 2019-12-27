/*
 * File:   opencv_demo.c
 * Modification for opencv4: by ckevar
 * Modified date on Sat Dec 21, 2019, 12:58PM
 *
 * Author: Tasanakorn
 *
 * Created on May 22, 2013, 1:52 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

#include <opencv2/core/core_c.h>
#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "bcm_host.h"
#include "interface/vcos/vcos.h"
#include "interface/vcos/vcos.h"

#include "interface/mmal/mmal.h"
#include "interface/mmal/util/mmal_default_components.h"
#include "interface/mmal/util/mmal_connection.h"
#include "interface/mmal/util/mmal_util.h"
#include "interface/mmal/util/mmal_util_params.h"
//#include "vgfont.h"

#define MMAL_CAMERA_PREVIEW_PORT 0
#define MMAL_CAMERA_VIDEO_PORT 1
#define MMAL_CAMERA_CAPTURE_PORT 2

/* MMAL Works like this
// INPUT_PORT -> COMPONENT -> OUTPUT_PORT
*/

typedef struct {
    int video_width;
    int video_height;
    int preview_width;
    int preview_height;
    int opencv_width;
    int opencv_height;
    float video_fps;
    MMAL_POOL_T *camera_video_port_pool;
    cv::CascadeClassifier cascade;
    std::vector<cv::Rect> storage;
    //CvMemStorage* storage;
    cv::Mat image;
    cv::Mat image2;
    //IplImage* image;
    //IplImage* image2;
    VCOS_SEMAPHORE_T complete_semaphore;
} PORT_USERDATA;

int exitSwitch = 1;

void IntHandler(int sig){
  exitSwitch = 0;
}

static void video_buffer_callback(MMAL_PORT_T *port, MMAL_BUFFER_HEADER_T *buffer) {
    static int frame_count = 0;
    static int frame_post_count = 0;
    static struct timespec t1;
    struct timespec t2;
    MMAL_BUFFER_HEADER_T *new_buffer;
    PORT_USERDATA * userdata = (PORT_USERDATA *) port->userdata;
    MMAL_POOL_T *pool = userdata->camera_video_port_pool;

    if (frame_count == 0)
        clock_gettime(CLOCK_MONOTONIC, &t1);

    frame_count++;
    mmal_buffer_header_mem_lock(buffer);
    memcpy(userdata->image.ptr(0), buffer->data, userdata->video_width * userdata->video_height);
    mmal_buffer_header_mem_unlock(buffer);

    if (vcos_semaphore_trywait(&(userdata->complete_semaphore)) != VCOS_SUCCESS) {
        vcos_semaphore_post(&(userdata->complete_semaphore));
        frame_post_count++;
    }

    if (frame_count % 10 == 0) {
        // print framerate every n frame
        clock_gettime(CLOCK_MONOTONIC, &t2);
        float d = (t2.tv_sec + t2.tv_nsec / 1000000000.0) - (t1.tv_sec + t1.tv_nsec / 1000000000.0);
        float fps = 0.0;

        if (d > 0) {
            fps = frame_count / d;
        } else {
            fps = frame_count;
        }
        userdata->video_fps = fps;
        printf("  Frame = %d, Frame Post %d, Framerate = %.0f fps \n", frame_count, frame_post_count, fps);
    }

    mmal_buffer_header_release(buffer);
    // and send one back to the port (if still open)
    if (port->is_enabled) {
        MMAL_STATUS_T status;

        new_buffer = mmal_queue_get(pool->queue);

        if (new_buffer)
            status = mmal_port_send_buffer(port, new_buffer);

        if (!new_buffer || status != MMAL_SUCCESS)
            printf("Unable to return a buffer to the video port\n");
    }
}

int main(int argc, char** argv) {
    MMAL_COMPONENT_T *camera = 0;	// Camera component
    MMAL_COMPONENT_T *preview = 0;	// Preview component
    MMAL_ES_FORMAT_T *format;		// settings for the camera
    MMAL_STATUS_T status;		// to acquire error
    MMAL_PORT_T *camera_preview_port = NULL, *camera_video_port = NULL, *camera_still_port = NULL; // ports that can be connected to the components
    MMAL_PORT_T *preview_input_port = NULL; 	// component
    MMAL_POOL_T *camera_video_port_pool; 	// pool of buffers
    MMAL_CONNECTION_T *camera_preview_connection = 0; // To connect 2 ports
    PORT_USERDATA userdata;			// general data
    uint32_t display_width, display_height; 	// display data

    printf("Running...\n");

    bcm_host_init();  // init the bcm

    signal(SIGINT, IntHandler); // to leave the main WHILE

    // Image resolution to be displayed
    userdata.preview_width = 1280 / 1;
    userdata.preview_height = 720 / 1;
    // Image resolution to be acquired for processing
    userdata.video_width = 1280 / 1;
    userdata.video_height = 720 / 1;
    // Image resolution to process
    userdata.opencv_width = 1280 / 4;
    userdata.opencv_height = 720 / 4;

    graphics_get_display_size(0, &display_width, &display_height);

    /* Rescale parameter to draw the markers where are they displayed from image processed to image displayed
    float r_w, r_h;
    r_w = (float) display_width / (float) userdata.opencv_width;
    r_h = (float) display_height / (float) userdata.opencv_height;
    */
    printf("Display resolution = (%d, %d)\n", display_width, display_height);

    /* setup opencv */
    if(!userdata.cascade.load("/installation/OpenCV-/share/opencv4/haarcascades/haarcascade_frontalface_alt.xml")){printf("[ERROR:] couldnt load classifier\n"); return -1;}
    // This creates a continues (1-D array) set of data to allocate the image
    userdata.image.create(userdata.video_height, userdata.video_width, CV_8UC1);
    userdata.image2.create(userdata.opencv_height, userdata.opencv_width, CV_8UC1);

    // Validation checker
    if(!userdata.image.isContinuous())
	fprintf(stderr, "[ERROR:] image is not continuos\n");

    // Creates camera component
    status = mmal_component_create(MMAL_COMPONENT_DEFAULT_CAMERA, &camera);
    if (status != MMAL_SUCCESS) {
        printf("Error: create camera %x\n", status);
        return -1;
    }

    // The camera component has three output, and they are assigned like this:
    camera_preview_port = camera->output[MMAL_CAMERA_PREVIEW_PORT];	// this port is for the frame to be displayed
    camera_video_port = camera->output[MMAL_CAMERA_VIDEO_PORT];		// this port is for the frame to be capture as raw data
    camera_still_port = camera->output[MMAL_CAMERA_CAPTURE_PORT];	// this port is for a photographe (we wont use here, it's just for educational purposes)

    // Configure the camera component
    {
        MMAL_PARAMETER_CAMERA_CONFIG_T cam_config = {
            { MMAL_PARAMETER_CAMERA_CONFIG, sizeof (cam_config)},
            .max_stills_w = 1280,
            .max_stills_h = 720,
            .stills_yuv422 = 0,		// still capture disabled (I assume)
            .one_shot_stills = 0, 	// continues shooting disabled (I assume)
            .max_preview_video_w = 1280,
            .max_preview_video_h = 720,
            .num_preview_video_frames = 2,
            .stills_capture_circular_buffer_height = 0, // since the still capture is not enabled, this one either
            .fast_preview_resume = 1, 	// Allows fast preview resume, when a still frame is captured and then processed
            .use_stc_timestamp = MMAL_PARAM_TIMESTAMP_MODE_RESET_STC // Timestamp of First captured frame is zero, and then STC continues from that
        };

        mmal_port_parameter_set(camera->control, &cam_config.hdr);	// setting the parameters
    }

    format = camera_video_port->format;

    format->encoding = MMAL_ENCODING_I420;		// raw data, this is for opencv processing purposes
    format->encoding_variant = MMAL_ENCODING_I420; 	// variation of that encoder? nop, use raw data.

    format->es->video.width = userdata.video_width;
    format->es->video.height = userdata.video_width;
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = userdata.video_width;
    format->es->video.crop.height = userdata.video_height;
    format->es->video.frame_rate.num = 30;
    format->es->video.frame_rate.den = 1;

    camera_video_port->buffer_size = userdata.preview_width * userdata.preview_height * 12 / 8; // 1.5 times of the first channel
    camera_video_port->buffer_num = 1; // how many buffers of that size will be used
    printf("  Camera video buffer_size = %d\n", camera_video_port->buffer_size);

    status = mmal_port_format_commit(camera_video_port); // loa settings
    if (status != MMAL_SUCCESS) {
        printf("Error: unable to commit camera video port format (%u)\n", status);
        return -1;
    }
    // settings for the pewview port
    format = camera_preview_port->format;

    format->encoding = MMAL_ENCODING_OPAQUE;		// it doesnt return the actual image data
    format->encoding_variant = MMAL_ENCODING_I420;  	// full chroma

    format->es->video.width = userdata.preview_width;
    format->es->video.height = userdata.preview_height;
    format->es->video.crop.x = 0;
    format->es->video.crop.y = 0;
    format->es->video.crop.width = userdata.preview_width;
    format->es->video.crop.height = userdata.preview_height;

    status = mmal_port_format_commit(camera_preview_port);

    if (status != MMAL_SUCCESS) {
        printf("Error: camera viewfinder format couldn't be set\n");
        return -1;
    }

    // crate pool form camera video port
    camera_video_port_pool = (MMAL_POOL_T *) mmal_port_pool_create(camera_video_port, camera_video_port->buffer_num, camera_video_port->buffer_size);
    userdata.camera_video_port_pool = camera_video_port_pool;
    camera_video_port->userdata = (struct MMAL_PORT_USERDATA_T *) &userdata;

    status = mmal_port_enable(camera_video_port, video_buffer_callback);
    if (status != MMAL_SUCCESS) {
        printf("Error: unable to enable camera video port (%u)\n", status);
        return -1;
    }

    status = mmal_component_enable(camera);

    status = mmal_component_create(MMAL_COMPONENT_DEFAULT_VIDEO_RENDERER, &preview);
    if (status != MMAL_SUCCESS) {
        printf("Error: unable to create preview (%u)\n", status);
        return -1;
    }
    preview_input_port = preview->input[0];

    {
        MMAL_DISPLAYREGION_T param;
        param.hdr.id = MMAL_PARAMETER_DISPLAYREGION;
        param.hdr.size = sizeof (MMAL_DISPLAYREGION_T);
        param.set = MMAL_DISPLAY_SET_LAYER;
        param.layer = 0;
        param.set |= MMAL_DISPLAY_SET_FULLSCREEN;
        param.fullscreen = 1;
        status = mmal_port_parameter_set(preview_input_port, &param.hdr);
        if (status != MMAL_SUCCESS && status != MMAL_ENOSYS) {
            printf("Error: unable to set preview port parameters (%u)\n", status);
            return -1;
        }
    }

    status = mmal_connection_create(&camera_preview_connection, camera_preview_port, preview_input_port, MMAL_CONNECTION_FLAG_TUNNELLING | MMAL_CONNECTION_FLAG_ALLOCATION_ON_INPUT);
    if (status != MMAL_SUCCESS) {
        printf("Error: unable to create connection (%u)\n", status);
        return -1;
    }

    status = mmal_connection_enable(camera_preview_connection);
    if (status != MMAL_SUCCESS) {
        printf("Error: unable to enable connection (%u)\n", status);
        return -1;
    }

    if (1) {
        // Send all the buffers to the camera video port
        int num = mmal_queue_length(camera_video_port_pool->queue);
        int q;

        for (q = 0; q < num; q++) {
            MMAL_BUFFER_HEADER_T *buffer = mmal_queue_get(camera_video_port_pool->queue);

            if (!buffer) {
                printf("Unable to get a required buffer %d from pool queue\n", q);
            }

            if (mmal_port_send_buffer(camera_video_port, buffer) != MMAL_SUCCESS) {
                printf("Unable to send a buffer to encoder output port (%d)\n", q);
            }
        }


    }

    if (mmal_port_parameter_set_boolean(camera_video_port, MMAL_PARAMETER_CAPTURE, 1) != MMAL_SUCCESS) {
        printf("%s: Failed to start capture\n", __func__);
    }

    vcos_semaphore_create(&userdata.complete_semaphore, "mmal_opencv_demo-sem", 0);
    int opencv_frames = 0;
    struct timespec t1;
    struct timespec t2;
    clock_gettime(CLOCK_MONOTONIC, &t1);

    GRAPHICS_RESOURCE_HANDLE img_overlay;
//    GRAPHICS_RESOURCE_HANDLE img_overlay2;

//    gx_graphics_init("/opt/vc/src/hello_pi/hello_font");

 //   gx_create_window(0, userdata.opencv_width, userdata.opencv_height, GRAPHICS_RESOURCE_RGBA32, &img_overlay);
//    gx_create_window(0, 500, 200, GRAPHICS_RESOURCE_RGBA32, &img_overlay2);
 //   graphics_resource_fill(img_overlay, 0, 0, GRAPHICS_RESOURCE_WIDTH, GRAPHICS_RESOURCE_HEIGHT, GRAPHICS_RGBA32(0xff, 0, 0, 0x55));
//    graphics_resource_fill(img_overlay2, 0, 0, GRAPHICS_RESOURCE_WIDTH, GRAPHICS_RESOURCE_HEIGHT, GRAPHICS_RGBA32(0xff, 0, 0, 0x55));

 //   graphics_display_resource(img_overlay, 0, 1, 0, 0, display_width, display_height, VC_DISPMAN_ROT0, 1);
    char text[256];

    while (exitSwitch) {
        if (vcos_semaphore_wait(&(userdata.complete_semaphore)) == VCOS_SUCCESS) {
            opencv_frames++;
            float fps = 0.0;
            if (1) {
                clock_gettime(CLOCK_MONOTONIC, &t2);
                float d = (t2.tv_sec + t2.tv_nsec / 1000000000.0) - (t1.tv_sec + t1.tv_nsec / 1000000000.0);
                if (d > 0) {
                    fps = opencv_frames / d;
                } else {
                    fps = opencv_frames;
                }

                printf("  OpenCV Frame = %d, Framerate = %.2f fps \n", opencv_frames, fps);
            }

   //         graphics_resource_fill(img_overlay, 0, 0, GRAPHICS_RESOURCE_WIDTH, GRAPHICS_RESOURCE_HEIGHT, GRAPHICS_RGBA32(0, 0, 0, 0x00));
     //       graphics_resource_fill(img_overlay2, 0, 0, GRAPHICS_RESOURCE_WIDTH, GRAPHICS_RESOURCE_HEIGHT, GRAPHICS_RGBA32(0, 0, 0, 0x00));


            if (1) {
		cv::resize(userdata.image, userdata.image2, userdata.image2.size(), 0, 0, cv::INTER_LINEAR);
		userdata.cascade.detectMultiScale(userdata.image2, userdata.storage, 1.4, 3, 0, cv::Size(100, 100), cv::Size(150, 150));
		cv::imwrite("image.jpg", userdata.image);
		cv::imwrite("image2.jpg", userdata.image2);
                CvRect* r;
                // Loop through objects and draw boxes
		if (userdata.storage.size() > 0) {
                        int ii;
                        for (ii = 0; ii < userdata.storage.size(); ii++) {
			    //printf("   Face %d [%d, %d, %d, %d] [%d, %d, %d, %d]\n", ii, userdata.storage.x, \
			//userdata.storage.y, userdata.storage.width, userdata.storage.height)
                            //r = (CvRect*) cvGetSeqElem(objects, ii);

                            //printf("  Face %d [%d, %d, %d, %d] [%d, %d, %d, %d]\n", ii, r->x, r->y, r->width, r->height, (int) ((float) r->x * r_w), (int) (r->y * r_h), (int) (r->width * r_w), (int) (r->height * r_h));


                            //graphics_resource_fill(img_overlay, r->x * r_w, r->y * r_h, r->width * r_w, r->height * r_h, GRAPHICS_RGBA32(0xff, 0, 0, 0x88));
                            //graphics_resource_fill(img_overlay, r->x * r_w + 8, r->y * r_h + 8, r->width * r_w - 16 , r->height * r_h - 16, GRAPHICS_RGBA32(0, 0, 0, 0x00));
       //                     graphics_resource_fill(img_overlay, r->x, r->y, r->width, r->height, GRAPHICS_RGBA32(0xff, 0, 0, 0x88));
         //                   graphics_resource_fill(img_overlay, r->x + 4, r->y + 4, r->width - 8, r->height - 8, GRAPHICS_RGBA32(0, 0, 0, 0x00));


                        }
                } else {
                    printf("!! Face detectiona failed !!\n");
                }

            }

            sprintf(text, "Video = %.2f FPS, OpenCV = %.2f FPS", userdata.video_fps, fps);
            /*graphics_resource_render_text_ext(img_overlay2, 0, 0,
                    GRAPHICS_RESOURCE_WIDTH,
                    GRAPHICS_RESOURCE_HEIGHT,
                    GRAPHICS_RGBA32(0x00, 0xff, 0x00, 0xff), /* fg */
            /*        GRAPHICS_RGBA32(0, 0, 0, 0x00), /* bg */
            /*        text, strlen(text), 25);

            graphics_display_resource(img_overlay, 0, 1, 0, 0, display_width, display_height, VC_DISPMAN_ROT0, 1);
            graphics_display_resource(img_overlay2, 0, 2, 0, display_width / 16, GRAPHICS_RESOURCE_WIDTH, GRAPHICS_RESOURCE_HEIGHT, VC_DISPMAN_ROT0, 1);
*/
        }
	break;
    }

    mmal_component_destroy(camera);
    mmal_component_destroy(preview);
    mmal_pool_destroy(camera_video_port_pool);
    mmal_connection_destroy(camera_preview_connection);

    return 0;
}

