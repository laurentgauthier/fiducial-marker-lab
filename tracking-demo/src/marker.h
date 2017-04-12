/*
 * marker.h
 *
 *  Created on: Jan 5, 2016
 *      Author: Laurent Gauthier <mr.l.gauthier@gmail.com>
 *
 * Find markers in a picture, and for each marker verify and decode an ID.
 *
 */

#ifndef SRC_MARKER_H_
#define SRC_MARKER_H_

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <vector>
#include "rs.hpp"

using namespace std;
using namespace cv;

namespace marker {


inline bool lineIntersection(cv::Point2f o1, cv::Point2f p1, cv::Point2f o2, cv::Point2f p2, cv::Point2f& r) {
	Point2f x = o2 - o1;
	Point2f d1 = p1 - o1;
	Point2f d2 = p2 - o2;

	float cross = d1.x*d2.y - d1.y*d2.x;
	if (std::abs(cross) < /* Epsilon */1e-8)
		return false;

	double t1 = (x.x * d2.y - x.y * d2.x)/cross;
	r = o1 + d1 * t1;
	return true;
}

class Marker {
public:
	cv::Point2f center;
	cv::Point2f zero;
	cv::Point2f one;
	cv::Point2f two[2];
	cv::Point2f three[3];
	/** Rectangle in which the code is included. */
	std::vector<cv::Point2f> codeCorners;
	bool        hasValidCode;
	uint8_t     codeValue[4];

	Marker () {
		hasValidCode = false;
		codeCorners.resize(4);
	}

	void normalize() {
		/* Re-order the points in groups of two and three so that they are normalized.
		 * This process starts with re-ordering the group of three points, and then
		 * re-ordering the group of two.
		 * */
		int distances[3];
		cv::Point reOrdered[3];

		for (int i=0; i<3; i++) {
			distances[i] = (zero.x-three[i].x)*(zero.x-three[i].x) + (zero.y-three[i].y)*(zero.y-three[i].y);
		}
		int min = distances[0], max = distances[0];
		for (int i = 1; i<3; i++) {
			if (distances[i] > max) {
				max = distances[i];
			}
			if (distances[i] < min) {
				min = distances[i];
			}
		}
		/* Re-order the points from the farthest to the closest from the zero corner. */
		for (int i=0; i<3; i++) {
			if (distances[i] == min) {
				reOrdered[2] = three[i];
			} else if (distances[i] == max) {
				reOrdered[0] = three[i];
			} else {
				reOrdered[1] = three[i];
			}
		}
		/* Now copy the sorted points back into the three array. */
		three[0] = reOrdered[0];
		three[1] = reOrdered[1];
		three[2] = reOrdered[2];

		/* We can now proceed with the re-ordering of the group of two points. */
		distances[0] = (three[0].x-two[0].x)*(three[0].x-two[0].x) + (three[0].y-two[0].y)*(three[0].y-two[0].y);
		distances[1] = (three[0].x-two[1].x)*(three[0].x-two[1].x) + (three[0].y-two[1].y)*(three[0].y-two[1].y);
		if (distances[1] > distances[0]) {
			cv::Point temp = two[1];
			/* In this case we must swap two[0] and two[1] */
			two[1] = two[0];
			two[0] = temp;
		}

		/* Guessing the position of the four corners of the code area requires
		 * making adjustments for the perspective. To achieve a good approximation
		 * we use intersections with diagonal lines:
		 *
		 *   1+---------+2
		 *    |\       /|
		 *    | \     / |
		 *    |  \   /  |
		 *    |   \ /   |
		 *   L+    +    +R
		 *    |   / \   |
		 *    |  /   \  |
		 *    | /     \ |
		 *    |/       \|
		 *   0+----+----+3
		 *         M
		 *
		 *
		 *
		 *   1+---------+2
		 *    |         |
		 *    |         |
		 *    |         |
		 *    |         |
		 *   0+---------+3
		 *
		 * This might not work well if the marker is large in the image and
		 * there is a strong distortion introduced by the lens, but it should
		 * be good enough in most cases.
		 */
		cv::Point2f middle, vanishingV, vanishingH, L, R, M;

		if (lineIntersection(one, three[0], zero, two[0], middle)) {
			// Compute the location of M.
			if (lineIntersection(one, zero, two[0], three[0], vanishingV)) {
				lineIntersection(middle, vanishingV, zero, three[0], M);
				// FIXME handle impossible error case here.
			} else {
				// The two lines are parallel.
				lineIntersection(middle, middle+(zero-one), zero, three[0], M);
				// FIXME handle impossible error case here.
			}
			// Compute the location of L and R.
			if (lineIntersection(one, two[0], zero, three[0], vanishingH)) {
				lineIntersection(middle, vanishingH, zero, one, L);
				lineIntersection(middle, vanishingH, two[0], three[0], R);
				// FIXME handle impossible error cases here.
			} else {
				// The two lines are parallel.
				lineIntersection(middle, middle+(three[0]-zero), zero, one, L);
				lineIntersection(middle, middle+(three[0]-zero), two[0], three[0], R);
				// FIXME handle impossible error cases here.
			}
		} else {
			// FIXME Should never happen, error handling here.
		}
		// Now compute the location of the four corners of the code area.
		lineIntersection(two[0], M, zero, one, codeCorners[0]);
		lineIntersection(R, M, zero, one, codeCorners[1]);
		lineIntersection(L, M, two[0], three[0], codeCorners[2]);
		lineIntersection(one, M, two[0], three[0], codeCorners[3]);
		// FIXME handle impossible error cases here.
	}

	void cornerSubPix(cv::Mat& greyImage, int windowSize, int zeroSize) {
		// Set the needed parameters to find the refined corners
		cv::Size winSize = Size( windowSize, windowSize );
		cv::Size zeroZone = Size( zeroSize, zeroSize );
		cv::TermCriteria criteria = cv::TermCriteria( /* CV_TERMCRIT_EPS + */ CV_TERMCRIT_ITER, 40, 0.001 );

		// Calculate the refined corner locations
		cv::cornerSubPix(greyImage, codeCorners, winSize, zeroZone, criteria);
	}

	bool readCode(cv::Mat& greyImage, cv::Mat& binaryImage) {
		// Define the destination image
		cv::Mat codeImage = cv::Mat::zeros(128, 256, CV_8UC1);
		int windowSize = 41, C = 10; // Make these parameters?

		// Corners of the destination image
		std::vector<cv::Point2f> fourPointArea;
		fourPointArea.push_back(cv::Point2f(0, codeImage.rows));
		fourPointArea.push_back(cv::Point2f(0, 0));
		fourPointArea.push_back(cv::Point2f(codeImage.cols, 0));
		fourPointArea.push_back(cv::Point2f(codeImage.cols, codeImage.rows));

		// Get transformation matrix
		cv::Mat transform = cv::getPerspectiveTransform(codeCorners, fourPointArea);

		// Apply perspective transformation
		cv::warpPerspective(greyImage, codeImage, transform, codeImage.size());
	    // Adaptive threshold on the image
	    cv::adaptiveThreshold(codeImage, binaryImage, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY, windowSize, C);
		uint8_t encodedCode[10];
		for (int x = 0; x < 10; x++) {
			encodedCode[x] = 0;
			for (int y = 0; y < 8; y++) {
				if (binaryImage.at<uint8_t>((int)(y*15.9+7.5), (int)(x*18.2+46)) == 0) {
					encodedCode[x] |= 1 << y;
				}
			}
		}
		for (float x = 0; x < 10; x++) {
			for (float y = 0; y < 8; y++) {
				cv::circle(binaryImage, cv::Point(x*18.2+46, y*15.9+7.5), 2, cv::Scalar(128));
			}
		}

		RS::ReedSolomon<4 /*Message length */, 6 /* ECC length*/> rs;
		if (rs.Decode(encodedCode, codeValue) == RESULT_SUCCESS) {
			hasValidCode = true;
			return true;
		} else {
			hasValidCode = false;
			return false;
		}
	}
	void drawGrey(cv::Mat &image) {
		cv::circle(image, center, 2, cv::Scalar(128));
		cv::circle(image, zero, 2, cv::Scalar(255));
		cv::circle(image, one, 2, cv::Scalar(255));
	    cv::circle(image, two[0], 2, cv::Scalar(255));
	    cv::circle(image, two[1], 2, cv::Scalar(128));
		cv::circle(image, three[0], 2, cv::Scalar(255));
		cv::circle(image, three[1], 2, cv::Scalar(128));
		cv::circle(image, three[2], 2, cv::Scalar(128));
		cv::circle(image,codeCorners[0], 2, cv::Scalar(128));
		cv::circle(image,codeCorners[1], 2, cv::Scalar(128));
		cv::circle(image,codeCorners[2], 2, cv::Scalar(128));
		cv::circle(image,codeCorners[3], 2, cv::Scalar(128));
	}
	void drawColor(cv::Mat &image) {
		int red = 1;
		int green = 1;
		if (hasValidCode) {
			red = 0;
		} else {
			green = 0;
		}
		cv::circle(image, center, 2, cv::Scalar(0,128*green,128*red));
		cv::circle(image, zero, 2, cv::Scalar(0,255*green,255*red));
		cv::circle(image, one, 2, cv::Scalar(0,255*green,255*red));
	    cv::circle(image, two[0], 2, cv::Scalar(0,255*green,255*red));
	    cv::circle(image, two[1], 2, cv::Scalar(0,128*green,128*red));
		cv::circle(image, three[0], 2, cv::Scalar(0,255*green,255*red));
		cv::circle(image, three[1], 2, cv::Scalar(0,128*green,128*red));
		cv::circle(image, three[2], 2, cv::Scalar(0,128*green,128*red));
		cv::circle(image,codeCorners[0], 2, cv::Scalar(0,128*green,128*red));
		cv::circle(image,codeCorners[1], 2, cv::Scalar(0,128*green,128*red));
		cv::circle(image,codeCorners[2], 2, cv::Scalar(0,128*green,128*red));
		cv::circle(image,codeCorners[3], 2, cv::Scalar(0,128*green,128*red));
	}
};

class Scanner {
public:
	cv::Mat greyImage;
	cv::Mat binaryImage;
	cv::Mat binaryInvertedImage;
	cv::Mat labelImage;
	cv::Mat labelInvertedImage;
	cv::Mat codeImage;
	cv::Mat tmp; // Used for dilation and erosion

	Scanner () {
		// Nothing for now.
	}
	void findMarkers(cv::Mat& frame, int windowSize, int C, std::vector<marker::Marker*>& markers);

	void findLabels(cv::Mat& image, cv::Mat& binary, int windowSize, int C);

	void ccLabels(cv::Mat& binary, cv::Mat& label);
};

} /* End of namespace marker */

#endif /* SRC_MARKER_H_ */
