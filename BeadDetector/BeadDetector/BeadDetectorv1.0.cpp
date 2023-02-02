#include "BeadDetector.h"
#include <map>

//#include <conio.h>
//The mouse callback will fill in these four points, when the user click on the image. 
std::vector<cv::Point2d> imageCorners;
std::vector<cv::Point2d> clickedCorners;
bool correctedWindowOpened = false;

static void selectFrameCorners(int event, int x, int y, int, void* pVoidSourceMat) {
	cv::Mat& sourceImage = *((cv::Mat*)pVoidSourceMat);
	//0.  Check it is a mouse click event
	if (event != cv::EVENT_LBUTTONDOWN)
		return;
	//1. Add the point to the vector: 
	clickedCorners.push_back(cv::Point2d(x, y));
	//2. Draw a circle on the point we clicked: 
	cv::circle(sourceImage, cv::Point2d(x, y), 2, cv::Scalar(0, 0, 255), 2);

	//3. if it's the last corner, reorder them.
	if (clickedCorners.size() == 4) {
		cv::Point2d largestPoint;
		std::vector<cv::Point2d> temp;
		int smallest = clickedCorners[0].x + clickedCorners[0].y;
		int largest = clickedCorners[0].x + clickedCorners[0].y;
		int smallPtr = 0;
		int largePtr = 0;
		//3.1 Find the largest and smallest values.
		for (int i = 1; i < 4; i++) {
			if (clickedCorners[i].x + clickedCorners[i].y < smallest) {
				smallest = clickedCorners[i].x + clickedCorners[i].y;
				smallPtr = i;
			}
			else if (clickedCorners[i].x + clickedCorners[i].y > largest) {
				largest = clickedCorners[i].x + clickedCorners[i].y;
				largePtr = i;
			}
		}
		temp.push_back(clickedCorners[smallPtr]);
		largestPoint = clickedCorners[largePtr];
		if (smallPtr < largePtr) { largePtr--; }
		clickedCorners.erase(clickedCorners.begin() + (smallPtr));
		clickedCorners.erase(clickedCorners.begin() + (largePtr));
		if (clickedCorners[0].x < clickedCorners[1].x) {
			temp.push_back(clickedCorners[0]);
			clickedCorners.erase(clickedCorners.begin());
		}
		else {
			temp.push_back(clickedCorners[1]);
			clickedCorners.erase(clickedCorners.begin()+1);
		}
		temp.push_back(largestPoint);
		temp.push_back(clickedCorners[0]);

		clickedCorners = temp;
		//3.2 make sure x is smaller on clickedCorners[1] than on [2] (making it the bottom left corner
		//3.3 swap CC[2] and CC[3]
	}
}

void Detector::calibrateDetector() {
	//0. Load image
	//cv::VideoCapture tmpCap(1);	// to avoid opening Leap Motion, first try to open a device assigned to 1 (this might need to be changed if you have other cameras e.g. inner cam)
	cv::VideoCapture tmpCap(webcamID);	
	if (tmpCap.isOpened())
		cap = tmpCap;
	else
		cap = cv::VideoCapture(0);

	//1. Discard a few frames (when you wake the camera, the first few frames are darked)
	int discardFirstFrames = 20;
	cv::Mat sourceImage;

	while (discardFirstFrames--)
		cap >> sourceImage;
	//sourceImage = cv::imread("testCase7.jpg");
	//sourceImage.copyTo(backgroundImage);
	//0. Notify the user of the steps to follow:
	system("cls");
	printf("Please click on four corners of the stage, in the order indicated on the frame\n");
	//1. Create a window showing our frame and 
	// register a callback to detect user clicking on the four corners of the stage: 
	cv::namedWindow("callibrateFrame", cv::WINDOW_FREERATIO);
	cv::imshow("callibrateFrame", sourceImage);
	cv::setMouseCallback("callibrateFrame", selectFrameCorners, (void*)&sourceImage);

	//2. Wait for corners to be selected
	while (/*this->curEstate == BACKGROUND_DETECT && */clickedCorners.size() != 4) {
		cv::imshow("callibrateFrame", sourceImage);
		cvWaitKey(1);
	}

	cv::Mat frame;
	int skipFrames = 10;
	while (skipFrames-- > 0)
		cap >> frame;
	cv::cvtColor(frame, backgroundImage, cv::COLOR_BGR2GRAY);
	
	//3. Build homography: 
	cv::destroyWindow("callibrateFrame");
	imageCorners.push_back(cv::Point2d(_imageWidth - 1, _imageHeight - 1));
	imageCorners.push_back(cv::Point2d(_imageWidth - 1, 0));
	imageCorners.push_back(cv::Point2d(0, 0)); 
	imageCorners.push_back(cv::Point2d(0, _imageHeight - 1));
	//this->homography = cv::findHomography(clickedCorners, imageCorners);
	homography = cv::findHomography(clickedCorners, imageCorners);
	cap.release();
}

cv::Mat Detector::correctImage(cv::Mat sourceImage) {
	cv::cvtColor(sourceImage, sourceImage, cv::COLOR_BGR2GRAY);

	cv::Mat differences;
	sourceImage.copyTo(differences);

	//3. Apply homography to correct the image
	cv::Mat correctedImage(_imageWidth, _imageHeight, differences.type());

	warpPerspective(differences, correctedImage, homography, correctedImage.size());
	//cv::imshow("CORRECTED", correctedImage);

	return correctedImage;
}

cv::Mat Detector::capImage() {
	//0. Load image
	//cv::VideoCapture temp = cv::VideoCapture(1);	// to avoid opening Leap Motion, first try to open a device assigned to 1 (this might need to be changed if you have other cameras e.g. inner cam)
	cv::VideoCapture temp(webcamID);	
	if (!temp.isOpened())
		temp = cv::VideoCapture(0);

	//1. Discard a few frames (when you wake the camera, the first few frames are darked)
	int discardFirstFrames = 20;
	cv::Mat sourceImage;

	while (discardFirstFrames--)
		temp >> sourceImage;


	return correctImage(sourceImage);
}

void Detector::detectBeads() {

	std::vector<cv::Point3d> bead_positions;
	std::vector<cv::Point> point_potential;

	//1. Define Mats for:  sourceImg, Template and ImageToShow (and result for testing)
	int match_method[] = { CV_TM_SQDIFF, CV_TM_SQDIFF_NORMED, CV_TM_CCORR, CV_TM_CCORR_NORMED, CV_TM_CCOEFF, CV_TM_CCOEFF_NORMED};
	cv::Mat img; cv::Mat img_display; cv::Mat result; cv::Mat resultRGB;
	capImage().copyTo(img);
	img.copyTo(img_display);
	cv::Mat img_displayRGB;
	cvtColor(img_display, img_displayRGB, CV_GRAY2RGB);
	//correctImage(cv::imread("testCase7.jpg")).copyTo(img); -- for debugging: if webcam/ levitator unavailable this image can act as a 'capture'

	//------------------------------------------
	cv::waitKey(1);

	//2. Load in template (do only once) and init template + img Mats - Make them greyscale
	static cv::Mat templ = cv::imread("Template2.jpg");
	static bool doOnce = true;
	if (doOnce) {
		cv::cvtColor(templ, templ, cv::COLOR_BGR2GRAY);
		doOnce = false;
	}


	cv::Point matchLoc;
	int secondMostCommon = 0;
	int mostCommon = 0;
	int offset = 12;
	int curIndex = 0;
	//3. Loop through match methods and record result from each in potential pos vector.
	//for (int method = 1; method < 2; method++) {
	static int method = 2;{//Quick debug (change value of 'method' to see how results change 
		// 4. Create the result matrix
		int result_cols = img.cols - templ.cols + 1;
		int result_rows = img.rows - templ.rows + 1;
		result.create(result_rows, result_cols, CV_32FC1);

		// 5. Match Template and Normalize
		matchTemplate(img, templ, result, match_method[method]);
		normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());

		/*
			from here, obtain the n max values from the 'result' matrix, where n is the inputted num of beads.
			(or min vlues in the case of SQDIFF algorithms)
		*/

		double threshold = 0.8;

		cv::threshold(result, result, threshold, 1.f, CV_THRESH_BINARY);

		cv::Mat resb;
		result.convertTo(resb, CV_8U, 255);

		std::vector<std::vector<cv::Point>> contours;
		findContours(resb, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

		std::vector<std::vector<cv::Point>> contours_poly(contours.size());

		std::vector<cv::Rect> boundRect(contours.size());
		cvtColor(result, resultRGB, CV_GRAY2RGB);
		
		for (int i = 0; i < contours.size() ; ++i)
		{
			cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
			boundRect[i] = cv::boundingRect(cv::Mat(contours_poly[i]));
			cv::Point2d pixelCoords;
			pixelCoords.x = templ.cols/2 + (boundRect[i].br().x + boundRect[i].tl().x) / 2;
			pixelCoords.y = templ.rows/2 + (boundRect[i].br().y + boundRect[i].tl().y) / 2;
			cv::Point2d tl = boundRect[i].tl(); tl.x += templ.cols/2; tl.y += templ.rows/2;
			cv::Point2d br = boundRect[i].br(); br.x += templ.cols/2; br.y += templ.rows/2;

			//Filter non-square results:
			float r_width = fabsf(boundRect[i].br().x - boundRect[i].tl().x);
			float r_height = fabsf(boundRect[i].br().y - boundRect[i].tl().y);
			if (r_width / r_height < 0.6f || r_width / r_height > 1/0.6f) {
				rectangle(img_displayRGB, tl, br, cv::Scalar(0, 0, 255), 2, 8, 0);
				continue;
			}
			//Add the result:
			cv::Point3d worldCoords=_imageOrigin
				+ pixelCoords.x * _horizontalAxis / _imageWidth
				+ pixelCoords.y * _verticalAxis / _imageHeight;;
			if(curIndex< numBeads){
				curPointsPtr[3*curIndex] = worldCoords.x;
				curPointsPtr[3*curIndex+1] = worldCoords.y;
				curPointsPtr[3*curIndex+2] = worldCoords.z;	
				curIndex++;
				rectangle(img_displayRGB, tl, br, cv::Scalar(0, 255, 0), 2, 8, 0);
			}
			else {
				rectangle(img_displayRGB, tl, br, cv::Scalar(255, 0, 0), 2, 8, 0);
			}
		}
	}	
	showWindow(img_displayRGB);
}

/*
Monty's initial implementation: It sometimes gets stuck in getNMostFrequent
*/
//void Detector::detectBeads() {
//
//	std::vector<cv::Point3d> bead_positions;
//	std::vector<cv::Point> point_potential;
//
//	//1. Define Mats for:  sourceImg, Template and ImageToShow (and result for testing)
//	int match_method[] = { CV_TM_SQDIFF, CV_TM_SQDIFF_NORMED, CV_TM_CCORR, CV_TM_CCORR_NORMED, CV_TM_CCOEFF, CV_TM_CCOEFF_NORMED};
//	cv::Mat img; cv::Mat img_display; cv::Mat result; cv::Mat resultRGB;
//	capImage().copyTo(img);
//	//correctImage(cv::imread("testCase7.jpg")).copyTo(img); -- for debugging: if webcam/ levitator unavailable this image can act as a 'capture'
//
//	//------------------------------------------
//	cv::waitKey(1);
//
//	//2. Load in template and init template + img Mats - Make them greyscale
//	static cv::Mat templ = cv::imread("Template2.jpg");
//	static bool doOnce = true;
//	if (doOnce) {
//		cv::cvtColor(templ, templ, cv::COLOR_BGR2GRAY);
//		doOnce = false;
//	}
//	img.copyTo(img_display);
//
//	cv::Mat img_displayRGB;
//	cvtColor(img_display, img_displayRGB, CV_GRAY2RGB);
//
//	cv::Point matchLoc;
//
//	int secondMostCommon = 0;
//	int mostCommon = 0;
//	int offset = 12;
//
//	//3. Loop through match methods and record result from each in potential pos vector.
//	for (int i = 0; i < 6; i++) {
//		// 4. Create the result matrix
//		int result_cols = img.cols - templ.cols + 1;
//		int result_rows = img.rows - templ.rows + 1;
//		result.create(result_rows, result_cols, CV_32FC1);
//
//		// 5. Match Template and Normalize
//		matchTemplate(img, templ, result, match_method[i]);
//		normalize(result, result, 0, 1, cv::NORM_MINMAX, -1, cv::Mat());
//
//		/*
//			from here, obtain the n max values from the 'result' matrix, where n is the inputted num of beads.
//			(or min vlues in the case of SQDIFF algorithms)
//		*/
//
//		double threshold = 0.8;
//
//		cv::threshold(result, result, threshold, 1.f, CV_THRESH_BINARY);
//
//		cv::Mat resb;
//		result.convertTo(resb, CV_8U, 255);
//
//		std::vector<std::vector<cv::Point>> contours;
//		findContours(resb, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);
//
//		std::vector<std::vector<cv::Point>> contours_poly(contours.size());
//
//		std::vector<cv::Rect> boundRect(contours.size());
//		cvtColor(result, resultRGB, CV_GRAY2RGB);
//
//		for (int i = 0; i < contours.size(); ++i)
//		{
//			cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
//			boundRect[i] = cv::boundingRect(cv::Mat(contours_poly[i]));
//			point_potential.push_back(cv::Point(cv::Point2d(boundRect[i].br().x, boundRect[i].tl().y)));
//		}
//	}
//	int ptr = 0;
//	std::vector<std::pair<cv::Point,int>> results = getNMostFrequent(point_potential, numBeads);
//	//showWindow(img_displayRGB);   -- leave in for debugging.
//	for (auto point : results) {
//		cv::Point3d coords = _imageOrigin
//			+ point.first.x * _horizontalAxis / _imageWidth
//			+ point.first.y * _verticalAxis / _imageHeight;
//		bead_positions.push_back(coords);
//		curPointsPtr[ptr] = coords.x;
//		curPointsPtr[ptr+1] = coords.y;
//		curPointsPtr[ptr+2] = coords.z;
//
//		ptr += 3;
//		rectangle(img_displayRGB, point.first, cv::Point(point.first.x + offset * 2, point.first.y + offset * 2), cv::Scalar(0, 0, 255), 2, 8, 0);
//	}
//	showWindow(img_displayRGB);
//}
//
//std::vector<std::pair<cv::Point, int>> Detector::getNMostFrequent(std::vector<cv::Point> points, int n){
//	std::map<std::pair<float, float>, int> pointOccurance;
//	std::vector<std::pair<cv::Point, int>> result;
//
//	int beadSearchDistance = 5;
//	bool satisfied = false;
//	
//	while (!satisfied) {
//		pointOccurance.clear();
//		result.clear();
//		//For every point in points - if the point is already recorded, increment the value in map - otherwise, add it with a value of 1.
//		for (cv::Point point : points) {
//			std::pair<float, float> temp(point.x, point.y);
//			pointOccurance[temp]++;
//		}
//		//Go through the map and delete any duplicated values (points with a distance of less than x pixels).
//		std::pair<float, float> keyToDelete(0, 0);
//		for (auto kvp : pointOccurance) {
//			for (auto kvp2 : pointOccurance) {
//				if (keyToDelete != std::pair<float, float>(0, 0)) {
//					pointOccurance.erase(keyToDelete);
//					keyToDelete = std::pair<float, float>(0, 0);
//				}
//				cv::Point a(kvp.first.first, kvp.first.second);
//				cv::Point b(kvp2.first.first, kvp2.first.second);
//				if (cv::norm(cv::Mat(a), cv::Mat(b)) != 0 && cv::norm(cv::Mat(a), cv::Mat(b)) < beadSearchDistance) {
//					pointOccurance[kvp.first] += pointOccurance[kvp2.first];
//					keyToDelete = kvp2.first;
//				}
//			}
//		}
//		//Go through the map and record the points which were found more than once. Delete them from the map.
//		for (auto kvp : pointOccurance) {
//			if (kvp.second != 1 && (kvp.first.first != 0 && kvp.first.second != 0) && result.size() < n) {
//				keyToDelete = std::pair<float, float>(kvp.first);
//				std::pair<cv::Point, int> temp(cv::Point(kvp.first.first, kvp.first.second), kvp.second);
//				result.push_back(temp);
//			}
//		}
//		if (result.size() == n) {
//			satisfied = true;
//		}
//		else if (result.size() < n) {
//			beadSearchDistance += 5;
//		}
//		//std::cout << "Search Distance: " << beadSearchDistance << "   Number of beads found: " << result.size() << std::endl;
//	}
//	return result;
//}

void Detector::printCurrentPoint(cv::Point2i point) {
	;// std::cout << "{" << point.x << ", " << point.y << "}" << std::endl;
}

void Detector::getCurrentBeadPositions(float* pointArray) {
	//Same than bellow, but much better performance.
	memcpy(pointArray, curPointsPtr, 3 * numBeads * sizeof(float));
	/*for (int i = 0; i < numBeads * 3; i++) {
		pointArray[i] = curPointsPtr[i];
	}*/
}

void Detector::updatePositionBuffer(int unityNumBeads)
{
	numBeads = unityNumBeads;
	float* temp = new float[numBeads * 3];
	//delete previously allocated 
	if (curPointsPtr!=NULL) 
		delete curPointsPtr;
	curPointsPtr = temp;
}

void Detector::showWindow(cv::Mat imgToShow) {
	//cv::namedWindow("What Image", cv::WINDOW_AUTOSIZE); // Eventually obsolete
	cv::imshow("What Image", imgToShow);
	cv::waitKey(1);
	//cv::waitKey(0);
	//cv::destroyWindow("What Image");
}