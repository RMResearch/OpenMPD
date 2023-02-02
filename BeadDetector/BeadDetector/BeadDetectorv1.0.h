#include "opencv2/core.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/calib3d.hpp"

#include <iostream>
#include <vector>
#define _USE_MATH_DEFINES
#include <math.h>
#define HAVE_STRUCT_TIMESPEC
#include <pthread.h>

#define BEAD_DETECTOR_EXPORT __declspec(dllexport)

class BEAD_DETECTOR_EXPORT Detector {
public:
	enum estates { BACKGROUND_DETECT = 0, RUNNING, STOPPED } curEstate;

	//0. Background related parameters (bgnd substraction and homography correction)
	std::vector<cv::Point3d> cornerPoints;
	int numBeads = 6;
	float* curPointsPtr;
	float pixelsPerMeter;
	float sphericity, minRadius;
	//0.1. Derived parameters
	cv::Point3d _horizontalAxis, _verticalAxis, _imageOrigin;
	float _imageWidth, _imageHeight;
	//1. Image processing related parameters:
	cv::Mat backgroundImage;	//Empty frame for background removal
	cv::Mat homography;			//Homography adjusting camera image to corrected coordinates
	cv::Mat matchImage;         //Image output after calibration - analysed when detecting.
	int threshold, erodeDilate;	//Theshold value and noise removal value (erode/dilate)
	//2. Thread-related parameters
	pthread_t thread;
	pthread_mutex_t curPointsAccess;
	std::vector<cv::Point3d> curPoints;
	int webcamID;
	cv::VideoCapture cap;

	Detector() { curPointsPtr = NULL; };

	inline void setSphericity(float s) { sphericity = s; }
	inline void setMinRadius(float minRadiusInMeters) { minRadius = minRadiusInMeters; }
	inline void setThreshold(int value) { threshold = value; }

	void initInstance(int cameraID, float p1_World[3], float p2_World[3], float p3_World[3], float p4_World[3], int pixelsPerMeter = 5000, int threshold = 100, int erodeDilate = 2, float sphericity = 0.4f, float minRadiusInMeters = 0.001f) {
		webcamID = cameraID;
		Detector::pixelsPerMeter = pixelsPerMeter;
		Detector::sphericity = sphericity;
		Detector::minRadius = minRadius;
		Detector::threshold = threshold;
		Detector::erodeDilate = erodeDilate;
		Detector::curEstate = BACKGROUND_DETECT;
		updatePositionBuffer(numBeads);
		
		pthread_mutex_init(&curPointsAccess, NULL);

		//Store the position (relative to OpenGLFramework) where the four corners are placed
		cornerPoints.push_back(cv::Point3d(p1_World[0], p1_World[0], p1_World[0]));
		cornerPoints.push_back(cv::Point3d(p2_World[0], p2_World[0], p2_World[0]));
		cornerPoints.push_back(cv::Point3d(p3_World[0], p3_World[0], p3_World[0]));
		cornerPoints.push_back(cv::Point3d(p4_World[0], p4_World[0], p4_World[0]));
		//Store derived parameters
		_horizontalAxis = cv::Point3d(p2_World[0] - p3_World[0], p2_World[1] - p3_World[1], p2_World[2] - p3_World[2]);
		_verticalAxis = cv::Point3d(p4_World[0] - p3_World[0], p4_World[1] - p3_World[1], p4_World[2] - p3_World[2]);
		_imageOrigin = cv::Point3d(p3_World[0], p3_World[1], p3_World[2]);
		_imageWidth = (int)(pixelsPerMeter * sqrt(_horizontalAxis.x * _horizontalAxis.x + _horizontalAxis.y * _horizontalAxis.y + _horizontalAxis.z * _horizontalAxis.z));
		_imageHeight = (int)(pixelsPerMeter * sqrt(_verticalAxis.x * _verticalAxis.x + _verticalAxis.y * _verticalAxis.y + _verticalAxis.z * _verticalAxis.z));
	}

	cv::Mat capImage();
	void calibrateDetector();
	void getCurrentBeadPositions(float* arrayPointer);
	void updatePositionBuffer(int numBeads);
	void detectBeads();
	std::vector<std::pair<cv::Point, int>> getNMostFrequent(std::vector<cv::Point> points, int n);
	void printCurrentPoint(cv::Point2i point);
	void showWindow(cv::Mat imgToShow);

protected:
	cv::Mat correctImage(cv::Mat sourceImage);
};
