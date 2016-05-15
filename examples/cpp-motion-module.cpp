// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////
// librealsense tutorial #1 - Accessing depth data //
/////////////////////////////////////////////////////

// First include the librealsense C++ header file
#include <librealsense/rs.hpp>
#include <cstdio>

using namespace rs;

class display_buf
{
public:
    display_buf(size_t size): buf(nullptr),size(size){ buf = new char[size]; };
    ~display_buf(void){ if (buf) delete[] buf; buf = nullptr;};

    char* get_data(){ return buf;};
    size_t get_size(){ return size;};

private:
    display_buf();  // avoid default and copy constructors
    display_buf(const display_buf &);  // avoid default and copy constructors

    char * buf;
    size_t size;

};

int main() try
{
    // Create a context object. This object owns the handles to all connected realsense devices.
    rs::context ctx;
    printf("There are %d connected RealSense devices.\n", ctx.get_device_count());
    if(ctx.get_device_count() == 0) return EXIT_FAILURE;

    // This tutorial will access only a single device, but it is trivial to extend to multiple devices
    rs::device * dev = ctx.get_device(0);
    printf("\nUsing device 0, an %s\n", dev->get_name());
    printf("    Serial number: %s\n", dev->get_serial());
    printf("    Firmware version: %s\n", dev->get_firmware_version());

    // Configure depth to run at VGA resolution at 30 frames per second
    //dev->enable_stream(rs::stream::depth, 640, 480, rs::format::z16, 30);
    rs::stream stream_type = rs::stream::depth;
    dev->enable_stream(stream_type, preset::best_quality);  // auto-select based on the actual camera type
    // Ev - IMU data will be parsed and handled in client code
    //if (dev->supports_channel(transport::usb_interrupt, channel::sensor_data))
     //   dev->enable_channel(transport::usb_interrupt, channel::sensor_data, 30/*, usr_calback_func*/);


    // Ev modify device start to include IMU channel activation
    dev->start();

    // Determine depth value corresponding to one meter
    const uint16_t one_meter = static_cast<uint16_t>(1.0f / dev->get_depth_scale());

    // retrieve actual frame size at runtime
    rs_intrinsics depth_intrin = dev->get_stream_intrinsics(stream_type);
    int width = depth_intrin.width;
    int height = depth_intrin.height;

    /* Will print a simple text-based representation of the image, by breaking it into 10x20 pixel regions and and approximating the coverage of pixels within one meter */
    int rows = (height / 20);
    int row_lenght = (width / 10);
    int display_size = (rows+1) * (row_lenght+1);

    display_buf buffer(display_size*sizeof(char));

    while(true)
    {
        // This call waits until a new coherent set of frames is available on a device
        // Calls to get_frame_data(...) and get_frame_timestamp(...) on a device will return stable values until wait_for_frames(...) is called
        dev->wait_for_frames();
        //printf("Frame arrived at %d \n", (int)clock());


        // Retrieve depth data, which was previously configured as a 640 x 480 image of 16-bit depth values
        const uint16_t * depth_frame = reinterpret_cast<const uint16_t *>(dev->get_frame_data(rs::stream::depth));

        // Print a simple text-based representation of the image, by breaking it into 10x20 pixel regions and and approximating the coverage of pixels within one meter

        char * out = buffer.get_data();
        int coverage[row_lenght] = {};       //The buffer will suffice up to 256*10  pixels width
        for(int y=0; y<height; ++y)
        {
            for(int x=0; x<width; ++x)
            {
                int depth = *depth_frame++;
                if(depth > 0 && depth < one_meter) ++coverage[x/10];
            }

            if(y%20 == 19)
            {
                for(int & c : coverage)
                {
                    *out++ = " .:nhBXWW"[c/25];
                    c = 0;
                }
                *out++ = '\n';
            }
            //printf("line %d is finished", y);
        }
        *out++ = 0;

        char *abc = new char[buffer.get_size()];
        memcpy(abc,buffer.get_data(),buffer.get_size());
        printf("\n%s", abc);
        delete[] abc;
        //printf("\n%s", buffer.get_data());
    }
    
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    // Method calls against librealsense objects may throw exceptions of type rs::error
    printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(), e.get_failed_args().c_str());
    printf("    %s\n", e.what());
    return EXIT_FAILURE;
}
catch(...)
{
    printf("Unhandled excepton occured'n");
}
