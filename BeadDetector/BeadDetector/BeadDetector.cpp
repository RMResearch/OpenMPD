#include "BeadDetector.h"
#include <map>

//#include <conio.h>
//The mouse callback will fill in these four points, when the user click on the image. 
std::vector<cv::Point2d> imageCorners;
std::vector<cv::Point2d> clickedCorners;
bool correctedWindowOpened = false;

void (*printMessage)(const char* msg) = NULL;
void (*printWarning)(const char* msg) = NULL;
void (*printError)(const char* msg) = NULL;

static void selectFrameCorners(int event, int x, int y, int, void* pVoidSourceMat) {
	cv::Mat& sourceImage = *((cv::Mat*)pVoidSourceMat);
	//0.  Check it is a mouse click event
	if (event != cv::EVENT_LBUTTONDOWN)
		return;
	if (clickedCorners.size() <= 4) {
		//1. Add the point to the vector: 
		clickedCorners.push_back(cv::Point2d(x, y));
		//2. draw a circle
		cv::circle(sourceImage, cv::Point2d(x, y), 5, cv::Scalar(0, 255, 0), 2);
		
	}
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
	cv::Mat sourceImage;
	sourceImage = capImage(false);
	//0. Notify the user of the steps to follow:
	system("cls");
	printf("Please click on four corners of the stage, in the order indicated on the frame\n");
	//1. Create a window showing our frame and 
	// register a callback to detect user clicking on the four corners of the stage: 
	cv::namedWindow("callibrateFrame", cv::WINDOW_AUTOSIZE);
	cv::imshow("callibrateFrame", sourceImage);
	cv::setMouseCallback("callibrateFrame", selectFrameCorners, (void*)&sourceImage);

	//2. Wait for corners to be selected
	while (/*this->curEstate == BACKGROUND_DETECT && */clickedCorners.size() != 4) {
			cv::imshow("callibrateFrame", sourceImage);
			cvWaitKey(33);		
	}
	cv::destroyWindow("callibrateFrame");
	_printMessage("[BeadDetector] Detector calibrated\n");
	//3. Capture background
	cv::Mat frame;
	int skipFrames = 10;
	while (skipFrames-- > 0)
		frame=capImage(false);
	cv::cvtColor(frame, backgroundImage, cv::COLOR_BGR2GRAY);
	_printMessage("[BeadDetector] Background acquired\n");
	//3. Build homography: 
	imageCorners.push_back(cv::Point2d(_imageWidth - 1, _imageHeight - 1));
	imageCorners.push_back(cv::Point2d(_imageWidth - 1, 0));
	imageCorners.push_back(cv::Point2d(0, 0)); 
	imageCorners.push_back(cv::Point2d(0, _imageHeight - 1));
	//this->homography = cv::findHomography(clickedCorners, imageCorners);
	homography = cv::findHomography(clickedCorners, imageCorners);
	_printMessage("[BeadDetector] Homography computed\n");
}

void Detector::_printMessage(const char* msg) {
	if (printMessage)
		printMessage(msg);
}
void Detector::_printWarning(const char* msg){
	if (printWarning)
		printWarning(msg);
}
void Detector::_printError(const char* msg){
	if (printError)
		printError(msg);
}

void Detector::RegisterPrintFuncs(void(*p_Message)(const char *), void(*p_Warning)(const char *), void(*p_Error)(const char *))
{
	printMessage = p_Message;
	printWarning = p_Warning;
	printError = p_Error;
	_printMessage("[BeadDetector] Print functions registered\n");
}

cv::Mat Detector::correctImage(cv::Mat sourceImage) {
	cv::cvtColor(sourceImage, sourceImage, cv::COLOR_BGR2GRAY);
	cv::Mat differences;
	cv::subtract(sourceImage, backgroundImage, sourceImage);
	//2. Apply thresholding + erode/dilate
	//2.1. Threshold
	cv::threshold(sourceImage, differences, 1.0f*threshold, 255, cv::THRESH_BINARY);//transform to binary Black/White
	//2.2. Erode/dilate
	cv::Mat element = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2 * erodeDilate + 1, 2 * erodeDilate + 1),
		cv::Point(erodeDilate, erodeDilate));
	cv::erode(differences, differences, element);
	cv::dilate(differences, differences, element);
	//3. Apply homography to correct the image
	cv::Mat correctedImage(_imageWidth, _imageHeight, differences.type());

	warpPerspective(differences, correctedImage, homography, correctedImage.size());
	//cv::imshow("CORRECTED", correctedImage);

	return correctedImage;
}

cv::Mat Detector::capImage(bool corrected) {
	cv::Mat sourceImage;
	//0. Open the stream (if not done already)
	if (!cap.isOpened()) {
		cv::VideoCapture temp(webcamID);		
		if (!temp.isOpened()) {
			webcamID = 0;
			temp = cv::VideoCapture(webcamID);
		}
		cap = temp;
		cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280); // valueX = your wanted width
		cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720); // valueY = your wanted heigth
		char msg[512];
		sprintf_s(msg,"[BeadDetector] Camera %d initialized\n", webcamID);
		_printMessage(msg);
		//1. Discard a few frames (when you wake the camera, the first few frames are darked)
		int discardFirstFrames = 20;		
		while (discardFirstFrames--)
			cap >> sourceImage;
	}	
	cap >> sourceImage;
	cap >> sourceImage;
	
	//Return corrected or uncorrected image.
	if (corrected)
		return correctImage(sourceImage);
	else
		return sourceImage;
}

int Detector::detectBeads() {
	char msg[512];
	std::vector<cv::Point3d> result;
	cv::Mat correctedImage = capImage(true);//Capture corrected image
	cv::Mat correctedImageRGB; 
	cvtColor(correctedImage, correctedImageRGB, CV_GRAY2RGB);
	_printMessage("[BeadDetector] Converted to Grayscale\n");
	//2. Detect beads:
	//1. Process corrected frame looking for blobs:
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(correctedImage, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));
	sprintf_s(msg, "[BeadDetector] Found %d contours\n", contours.size());
	_printMessage(msg);

	//2. Check if any blob is our bead
	for (size_t i = 0; i < contours.size(); i++)
	{
		//2.1. Check some of its relevant parameters
		cv::Scalar color = cv::Scalar(128, 255, 128);
		double area = cv::contourArea(contours[i]);
		double perimeter = cv::arcLength(contours[i], true);
		cv::Point2f circleCentre;
		float circleRadiusInPix, circleRadiusInMeters;
		cv::minEnclosingCircle(contours[i], circleCentre, circleRadiusInPix);
		circleRadiusInMeters = circleRadiusInPix / pixelsPerMeter;
		//2.2. Circularity test
		double circularity = 4 * M_PI*area / (perimeter*perimeter);
		if (circleRadiusInMeters > this->minRadius && circleRadiusInMeters <= this->maxRadius  && circularity > sphericity
			) {
			cv::drawContours(correctedImageRGB, contours, (int)i, color, 2, 8, hierarchy, 0, cv::Point());
			cv::Point3d coords = _imageOrigin
				+ circleCentre.x * _horizontalAxis / _imageWidth
				+ circleCentre.y * _verticalAxis / _imageHeight;
			result.push_back(coords);
			sprintf_s(msg, "[BeadDetector] Found valid contour!\n");
			_printMessage(msg);
		}
	}
	//3. Store the results (can be queried by calling getCurrentBeadPositions)
	this->updatePositionBuffer(result.size());
	for (int p = 0; p < result.size(); p++) {
		curPointsPtr[3*p] = result[p].x;
		curPointsPtr[3*p+1] = result[p].y;
		curPointsPtr[3*p+2] = result[p].z;	
	}
	//4. Show if required
	if (visualize) {
		cv::imshow("CORRECTED", correctedImageRGB);
		cv::waitKey(1);
	}
	//5. Return number of beads detected.
	sprintf_s(msg,"[BeadDetector] %d particles detected\n", result.size());
	_printMessage(msg);
	return result.size();
}

//
//void Detector::detectBeads() {
//
//	std::vector<cv::Point3d> bead_positions;
//	std::vector<cv::Point> point_potential;
//
//	//1. Define Mats for:  sourceImg, Template and ImageToShow (and result for testing)
//	int match_method[] = { CV_TM_SQDIFF, CV_TM_SQDIFF_NORMED, CV_TM_CCORR, CV_TM_CCORR_NORMED, CV_TM_CCOEFF, CV_TM_CCOEFF_NORMED};
//	cv::Mat img; cv::Mat img_display; cv::Mat result; cv::Mat resultRGB;
//	capImage().copyTo(img);
//	img.copyTo(img_display);
//	cv::Mat img_displayRGB;
//	cvtColor(img_display, img_displayRGB, CV_GRAY2RGB);
//	//correctImage(cv::imread("testCase7.jpg")).copyTo(img); -- for debugging: if webcam/ levitator unavailable this image can act as a 'capture'
//
//	//------------------------------------------
//	cv::waitKey(1);
//
//	//2. Load in template (do only once) and init template + img Mats - Make them greyscale
//	static cv::Mat templ = cv::imread("Template2.jpg");
//	static bool doOnce = true;
//	if (doOnce) {
//		cv::cvtColor(templ, templ, cv::COLOR_BGR2GRAY);
//		doOnce = false;
//	}
//
//
//	cv::Point matchLoc;
//	int secondMostCommon = 0;
//	int mostCommon = 0;
//	int offset = 12;
//	int curIndex = 0;
//	//3. Loop through match methods and record result from each in potential pos vector.
//	//for (int method = 1; method < 2; method++) {
//	static int method = 2;{//Quick debug (change value of 'method' to see how results change 
//		// 4. Create the result matrix
//		int result_cols = img.cols - templ.cols + 1;
//		int result_rows = img.rows - templ.rows + 1;
//		result.create(result_rows, result_cols, CV_32FC1);
//
//		// 5. Match Template and Normalize
//		matchTemplate(img, templ, result, match_method[method]);
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
//		for (int i = 0; i < contours.size() ; ++i)
//		{
//			cv::approxPolyDP(cv::Mat(contours[i]), contours_poly[i], 3, true);
//			boundRect[i] = cv::boundingRect(cv::Mat(contours_poly[i]));
//			cv::Point2d pixelCoords;
//			pixelCoords.x = templ.cols/2 + (boundRect[i].br().x + boundRect[i].tl().x) / 2;
//			pixelCoords.y = templ.rows/2 + (boundRect[i].br().y + boundRect[i].tl().y) / 2;
//			cv::Point2d tl = boundRect[i].tl(); tl.x += templ.cols/2; tl.y += templ.rows/2;
//			cv::Point2d br = boundRect[i].br(); br.x += templ.cols/2; br.y += templ.rows/2;
//
//			//Filter non-square results:
//			float r_width = fabsf(boundRect[i].br().x - boundRect[i].tl().x);
//			float r_height = fabsf(boundRect[i].br().y - boundRect[i].tl().y);
//			if (r_width / r_height < 0.6f || r_width / r_height > 1/0.6f) {
//				rectangle(img_displayRGB, tl, br, cv::Scalar(0, 0, 255), 2, 8, 0);
//				continue;
//			}
//			//Add the result:
//			cv::Point3d worldCoords=_imageOrigin
//				+ pixelCoords.x * _horizontalAxis / _imageWidth
//				+ pixelCoords.y * _verticalAxis / _imageHeight;;
//			if(curIndex< numBeads){
//				curPointsPtr[3*curIndex] = worldCoords.x;
//				curPointsPtr[3*curIndex+1] = worldCoords.y;
//				curPointsPtr[3*curIndex+2] = worldCoords.z;	
//				curIndex++;
//				rectangle(img_displayRGB, tl, br, cv::Scalar(0, 255, 0), 2, 8, 0);
//			}
//			else {
//				rectangle(img_displayRGB, tl, br, cv::Scalar(255, 0, 0), 2, 8, 0);
//			}
//		}
//	}	
//	showWindow(img_displayRGB);
//}

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


void Detector::getCurrentBeadPositions(float* pointArray) {
	memcpy(pointArray, curPointsPtr, 3 * numBeads * sizeof(float));
	char msg[512];
	sprintf_s(msg,"[BeadDetector] %d particle positions returned\n", numBeads);
	_printMessage(msg);

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