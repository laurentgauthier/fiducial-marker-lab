/*  
 * Copyright (C) 2016-2017 Laurent GAUTHIER <laurent.gauthier@soccasys.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 */

//#define DISABLE_GUI 1
#ifndef DISABLE_GUI
#include <opencv2/highgui.hpp>
#endif
//#include <opencv2/cv.h>
#include <opencv2/videoio/videoio.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include "marker.h"

using namespace std;
using namespace cv;
using namespace marker;


int main(int argc, char* argv[]) {
    std::string      windowName = "Camera";
    cv::Mat          frame;
    cv::Mat          binary;
    cv::Mat          label_image;
    cv::VideoCapture capture(1);
#ifndef DISABLE_GUI
    bool             showBinary = false;
#else
    bool             showBinary = true;
#endif
    bool             showThresholding = false;
    std::vector<marker::Marker *> markers;
    marker::Scanner  scanner;
    int              windowSize = 25, C = 10;

    if (!capture.isOpened()) {
        cout << "ERROR INITIALIZING VIDEO CAPTURE" << endl;
        return -1;
    }
    capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
    capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
    //capture.set(cv::CAP_PROP_FRAME_HEIGHT, 768);
    //capture.set(cv::CAP_PROP_FRAME_WIDTH, 1024);
    //capture.set(cv::CAP_PROP_FRAME_HEIGHT, 768);
    //capture.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    //capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    //capture.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    //capture.set(cv::CAP_PROP_FPS, 30);
    cout << "Camera FPS:   " << capture.get(cv::CAP_PROP_FPS) << endl;
    cout << "Frame Width:  " << capture.get(cv::CAP_PROP_FRAME_WIDTH) << endl;
    cout << "Frame Height: " << capture.get(cv::CAP_PROP_FRAME_HEIGHT) << endl;
    //cout << "Exposure:     " << capture.get(cv::CAP_PROP_EXPOSURE) << endl;
    cout << "Contrast:     " << capture.get(cv::CAP_PROP_CONTRAST) << endl;
    cout << "Brightness:   " << capture.get(cv::CAP_PROP_BRIGHTNESS) << endl;
    //cout << "Hue:          " << capture.get(cv::CAP_PROP_HUE) << endl;
    cout << "Gain:         " << capture.get(cv::CAP_PROP_GAIN) << endl;
    cout << "Focus:        " << capture.get(cv::CAP_PROP_FOCUS) << endl;
#ifndef DISABLE_GUI
    // Create a named window
    cv::namedWindow(windowName, CV_WINDOW_AUTOSIZE); //create a window to display our webcam feed
#endif
    while (1) {
    	auto t0 = std::chrono::high_resolution_clock::now();
        bool bSuccess = capture.read(frame); // read a new frame from camera feed
        if (!bSuccess) {
            // Test if the frame has been succesfully read
            cout << "ERROR READING FRAME FROM CAMERA FEED" << endl;
            break;
        }

        if (!showBinary) {
        	char message[256];
            // Show the image
        	auto t2 = std::chrono::high_resolution_clock::now();
        	sprintf(message, "%d ms", std::chrono::duration_cast<std::chrono::milliseconds>(t2-t0));
#ifndef DISABLE_GUI
            cv::putText(frame, message,Point(0,60),2,2,Scalar(0,0,255),2);
            cv::imshow(windowName, frame); //show the frame in "MyVideo" window
#else
            cout << message << endl;
#endif
        } else {
        	char message[256];
        	auto t1 = std::chrono::high_resolution_clock::now();
        	if (showThresholding) {
        		scanner.findLabels(frame, binary, windowSize, C);
            	auto t2 = std::chrono::high_resolution_clock::now();
            	sprintf(message, "%d us", std::chrono::duration_cast<std::chrono::microseconds>(t2-t1));
#ifndef DISABLE_GUI
                cv::putText(binary, message,Point(0,60),2,2,Scalar(128,128,128),2);
                // Show the threshold image
                cv::imshow(windowName, binary);
#else
                cout << message << endl;
#endif
        	} else {
        		scanner.findMarkers(frame, windowSize, C, markers);
            	auto t2 = std::chrono::high_resolution_clock::now();
            	sprintf(message, "%d us", std::chrono::duration_cast<std::chrono::microseconds>(t2-t1));
#ifndef DISABLE_GUI
                cv::putText(frame, message,Point(0,60),2,2,Scalar(128,128,128),2);
                // Draw the marker locations on the picture.
                for (int i = 0; i < markers.size(); i++) {
                	markers[i]->drawColor(frame);
                }
            	cv::imshow(windowName, frame);
#else
                cout << message << endl;
#endif
        	}
        }

#ifndef DISABLE_GUI
        // Listen for 10ms for a key to be pressed
        switch (cv::waitKey(100)) {
        case 27:
            // 'ESC' has been pressed (ASCII value is 27)
            return 0;
        case ' ':
            // Change which image is displayed
        	if (showBinary && !showThresholding) {
        		showThresholding = true;
        	} else {
        		showThresholding = false;
        		showBinary = !showBinary;
        	}
            break;
        }
#endif
    }
    cout << "Software stopped." << endl;
    return 0;
}
