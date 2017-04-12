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

#include "marker.h"

// Using a multimap for tracking labelled objects.
#include <map>

namespace marker {

void thresholdImage(cv::Mat &image, cv::Mat& grey, cv::Mat &binary, int windowSize, int C) {
    double maximum   = 1;

    // First convert the image to grayscale
    cv::cvtColor(image, grey, CV_BGR2GRAY);

    // Adaptive threshold on the image
    cv::adaptiveThreshold(grey, binary, maximum, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, windowSize, C);
}


void erode(cv::Mat &src, cv::Mat &dst, int erosionSize) {
  int erosionType = cv::MORPH_RECT;
  // erosionType = cv::MORPH_CROSS;
  // erosionType = cv::MORPH_ELLIPSE;

  cv::Mat element = cv::getStructuringElement( erosionType,
                                       Size( 2*erosionSize + 1, 2*erosionSize+1 ),
                                       Point( erosionSize, erosionSize ) );

  // Apply the erosion operation
  cv::erode( src, dst, element );
}

void dilate(cv::Mat &src, cv::Mat &dst, int dilationSize) {
  int dilationType = cv::MORPH_RECT;
  // dilationType = cv::MORPH_CROSS;
  // dilationType = cv::MORPH_ELLIPSE;

  cv::Mat element = cv::getStructuringElement( dilationType,
                                       Size( 2*dilationSize + 1, 2*dilationSize+1 ),
                                       Point( dilationSize, dilationSize ) );

  // Apply the dilation operation
  cv::dilate( src, dst, element );
}

void Scanner::findLabels(cv::Mat &image, cv::Mat &binary, int windowSize, int C) {
    double maximum   = 255;
    cv::Mat grey;
    cv::Mat tmp;
    cv::Mat labelImage;
    //cv::Mat labelInvertedImage;
    cv::Mat stats;
    cv::Mat centroid;
    //cv::Mat binaryInverted;
    int rows;

    // First convert the image to grayscale
    cv::cvtColor(image, grey, CV_BGR2GRAY);

    // Adaptive threshold on the image
    cv::adaptiveThreshold(grey, binary, maximum, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, windowSize, C);
	erode(binary, tmp, 1);
	dilate(tmp, binary, 1);
	cv::threshold(binary, binaryInvertedImage, 0, maximum, cv::THRESH_BINARY_INV);

	// Mark all the connected components in the binary image, and turn the binary image
	// into a grey scale image for debug.
	cv::connectedComponentsWithStats(binary, labelImage, stats, centroid, 4, CV_16U);
	rows = stats.rows;
	cv::connectedComponentsWithStats(binaryInvertedImage, labelInvertedImage, stats, centroid, 4, CV_16U);
	labelImage += (labelInvertedImage+rows);
	for (int y = 0; y < binary.rows; y++) {
		for (int x = 0; x < binary.cols; x++) {
			binary.at<uint8_t>(y,x) = (labelImage.at<short>(y,x)*37) % 256;
		}
	}
}

typedef struct {
	int area;
	int label;
	int parentLabel;
	int childCount;
	int totalChildCount;
	int children[5]; // Valid markers have 5 children
	cv::Point2f topLeft;
	cv::Point2f bottomRight;
	cv::Point2f center;
} Component;

/* Better implementation which uses Connected Components APIs for the labeling.  */
void Scanner::findMarkers(cv::Mat& frame, int windowSize, int C, std::vector<marker::Marker*>& markers) {
	cv::Mat stats, centroid;
	cv::Mat statsInverted, centroidInverted;
	std::multimap<int, int> componentsSortedByBoxArea;
	std::vector<Component> components;
	int maximum = 1;
	int openingSize = 1;

	/* Warning: it is the responsibility of the code calling this function to de-allocate the Markers in this vector. */
	markers.clear();

	/* Turn the image into a binary image, and also compute the inverted binary image.
	 *
	 * The window size used for the thresholding influences the size of the markers that
	 * can be discovered by the algorithm.
	 *
	 */
	thresholdImage(frame, greyImage, binaryImage, windowSize, C);
	/* Opening, should be optional for areas where we are trying to detect small size markers. */
	if (openingSize > 0) {
		erode(binaryImage, tmp, openingSize);
		dilate(tmp, binaryImage, openingSize);
	}
	cv::threshold(binaryImage, binaryInvertedImage, 0, maximum, cv::THRESH_BINARY_INV);

	// Mark all the connected components in the binary image.
	cv::connectedComponentsWithStats(binaryImage, labelImage, stats, centroid, 4, CV_16U);
	/* TODO Run these two "connected components" operations in separate threads. */
	cv::connectedComponentsWithStats(binaryInvertedImage, labelInvertedImage, statsInverted, centroidInverted, 4, CV_16U);
	/* Merge the two labelled images.  */
#ifndef DISABLE_MATRIX_OPS
	binaryInvertedImage.convertTo(binaryInvertedImage, CV_16U);
	labelInvertedImage += (binaryInvertedImage*stats.rows);
	labelImage += labelInvertedImage;
#else
	for (int y=0; y<labelInvertedImage.rows; y++) {
		for (int x=0; x<labelInvertedImage.cols; x++) {
			short value = labelInvertedImage.at<short>(y,x);
			if (value > 0) {
				labelImage.at<short>(y,x) += value + stats.rows;
			}
		}
	}
#endif
	// Combine the stats and centroid arrays.
	stats.push_back(statsInverted);
	centroid.push_back(centroidInverted);
	components.reserve(stats.rows);

	/* Only keep the components that do not touch any of the sides of the image, and
	   sort them by their bounding box area. */
	for (int label = 1; label<stats.rows; label++) {
		// label zero is skipped, as it is the background label of the first
		// pass of the connected component computation
		Component& component = components[label];
		component.parentLabel = -1;
		component.childCount = 0;
		component.totalChildCount = 0;
		component.label = label;
		component.children[0] = -1;
		if ( stats.at<int>(label,cv::CC_STAT_TOP) > 0
		  && stats.at<int>(label,cv::CC_STAT_LEFT) > 0
		  && (stats.at<int>(label,cv::CC_STAT_TOP)+stats.at<int>(label,cv::CC_STAT_HEIGHT)) < (binaryImage.rows-1)
		  && (stats.at<int>(label,cv::CC_STAT_LEFT)+stats.at<int>(label,cv::CC_STAT_WIDTH)) < (binaryImage.cols-1)) {
			int boxArea = stats.at<int>(label,cv::CC_STAT_WIDTH)*stats.at<int>(label,cv::CC_STAT_HEIGHT);
			componentsSortedByBoxArea.insert(std::pair<int,int>(boxArea, label));
			component.topLeft.x = stats.at<int>(label,cv::CC_STAT_LEFT);
			component.topLeft.y = stats.at<int>(label,cv::CC_STAT_TOP);
			component.bottomRight.x = stats.at<int>(label,cv::CC_STAT_LEFT)+stats.at<int>(label,cv::CC_STAT_WIDTH);
			component.bottomRight.y = stats.at<int>(label,cv::CC_STAT_TOP)+stats.at<int>(label,cv::CC_STAT_HEIGHT);
			component.center.x  = centroid.at<double>(label,0);
			component.center.y  = centroid.at<double>(label,1);
			component.area      = stats.at<int>(label,cv::CC_STAT_AREA);
		}
	}

	/* For each component determine which other component (if any) it is included in. */
	for (auto it = componentsSortedByBoxArea.begin(); it != componentsSortedByBoxArea.end(); it++) {
		int label = it->second;
		int x = components[label].topLeft.x - 1;
		int y = components[label].topLeft.y;
	    int currentLabel = labelImage.at<short>(y, x++);
	    int previousLabel = currentLabel;
	    int iterationCount = 0;

		while ((currentLabel = labelImage.at<short>(y, x++)) != label) {
			previousLabel = currentLabel;
			iterationCount++;
		}
		/* Record the parent of this label. */
		components[label].parentLabel = previousLabel;
		if (components[previousLabel].childCount < 5) {
			components[previousLabel].children[components[previousLabel].childCount] = label;
		}
		components[previousLabel].childCount++;
		components[previousLabel].totalChildCount += components[label].totalChildCount + 1;
		/* If the current label meets the marker requirements, record it for later use.  */
		if (components[label].childCount == 5 && components[label].totalChildCount == 11) {
			int histogram[] = {0, 0, 0, 0};
			for (int child = 0; child < 5; child++) {
				int childLabel = components[label].children[child];
				if (childLabel > 0 && components[childLabel].totalChildCount >= 0 && components[childLabel].totalChildCount < 4) {
					histogram[components[childLabel].totalChildCount]++;
				}
			}
			if (histogram[0] == 2 && histogram[1] == 1 && histogram[2] == 1 && histogram[3] == 1) {
				/* We have found what really looks like a marker, create a new Marker object */
				marker::Marker *newMarker = new Marker();
				newMarker->center = components[label].center;
				int zeroArea = 10000;
				for (int child = 0; child < 5; child++) {
					int childLabel = components[label].children[child];

					switch (components[childLabel].totalChildCount) {
					case 0:
						if (components[childLabel].area < zeroArea) {
							newMarker->zero.x = components[childLabel].center.x;
							newMarker->zero.y = components[childLabel].center.y;
							zeroArea = components[childLabel].area;
						}
						break;
					case 1:
						newMarker->one = components[components[childLabel].children[0]].center;
						break;
					case 2:
						newMarker->two[0] = components[components[childLabel].children[0]].center;
						newMarker->two[1] = components[components[childLabel].children[1]].center;
						break;
					case 3:
						newMarker->three[0] = components[components[childLabel].children[0]].center;
						newMarker->three[1] = components[components[childLabel].children[1]].center;
						newMarker->three[2] = components[components[childLabel].children[2]].center;
						break;
					}
				}
				newMarker->normalize();
				newMarker->cornerSubPix(greyImage, 5, -1);
				newMarker->readCode(greyImage, codeImage);
				markers.push_back(newMarker);
			}
		}
	}
	components.clear();
}
} /* End of namespace  */
