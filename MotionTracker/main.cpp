#include <opencv2/opencv.hpp>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/video/video.hpp"
#include <windows.h>
#include <iostream>
#include <vector>

using namespace std;
using namespace cv;

Mat staticimg;

void GetStatic(String);
void MotionTracker(String, String, String);


/**
* We define our paths and call our two main functions that are then responsible of detecting
* and tracking the objects. To make this program work with other files we just need to change the
* values of insert and output.
*
*/
int main()
{
	staticimg = 0;
	String insert = "C:/walk_short.avi";			// Path to video that needs to be detected
	String output = "C:/walk__short_output.avi";	// Path to where detected video to be saved - Background Subtractor
	String output2 = "C:/walk_short_output2.avi";	// Path to where detected video to be saved - Absolute Difference

	//GetStatic(insert);
	MotionTracker(insert, output, output2);
	return 0;
}

/**
* GetStatic is responsible of eroding a video frame by frame until the end. It then creates an image
* based on the results. Any object that has been moving will slowly diappear while static objects will
* remain in the picture. The image will later on be used to detect moving objects.
*
* @param insert Path to a valid video that needs to be detected.
*/
void GetStatic(String insert) {

	Mat window, main, NoObject, detail;
	VideoCapture capSrc(insert);
	capSrc.read(window);

	BackgroundSubtractorMOG2 bg = BackgroundSubtractorMOG2();
	bg.set("history", 50);						// Sets the number of last frames that affect the background model
	bg.set("nmixtures", 3);						// Sets the number of gaussian components in the background model
	bg.set("backgroundRatio", 0.7);				// Sets the "background ratio" parameter of the algorithm
	bg.set("detectShadows", false);				// Enables or disables shadow detection

	while (capSrc.read(main)) {
		Mat diff, result;

		bg.operator()(main, detail);
		bg.getBackgroundImage(NoObject);
		erode(detail, detail, Mat());

		imshow("No Object", NoObject);
		imshow("Original", main);
		waitKey(1);
	}

	Sleep(2000);
	imwrite("C:/NoObjects.png", NoObject);
	capSrc.release();
	destroyAllWindows();
}


/**
* We will once again will go through our video frame by frame to find and detect the moving 
* objects. Here we will use our eroded image and find its difference from the current
* frame which will give us the objects that are changing their positions using a 
* BackgroundSubtractorMOG2 class. The result is a video that draws rectangles around objects
* that were not found in the NoObject.png
*
* @param insert Path to a valid video that needs to be detected.
* @param output Path to where the detection video should be saved. Background Subtractor
* @param output Path to where the detection video should be saved. Absolute Difference
*/
void MotionTracker(String insert, String output, String output2) {

	Mat window, main, main2, diff, NoObject, detail, detail2, dst;

	VideoCapture capSrc(insert);
	double fps = capSrc.get(CV_CAP_PROP_FPS);
	capSrc.read(window);
	double dWidth = window.cols;
	double dHeight = window.rows;

	Size frameSize(static_cast<int>(dWidth), static_cast<int>(dHeight));

	VideoWriter oVideoWriter(output, CV_FOURCC('P', 'I', 'M', '1'), fps, frameSize, true);
	VideoWriter oVideoWriter2(output2, CV_FOURCC('P', 'I', 'M', '1'), fps, frameSize, true);

	BackgroundSubtractorMOG2 bg = BackgroundSubtractorMOG2();
	bg.set("history", 50);
	bg.set("nmixtures", 3);
	bg.set("backgroundRatio", 0.7);
	bg.set("detectShadows", false);

	BackgroundSubtractorMOG2 bg2 = BackgroundSubtractorMOG2();
	bg2.set("history", 50);
	bg2.set("nmixtures", 3);
	bg2.set("backgroundRatio", 0.7);
	bg2.set("detectShadows", false);

	namedWindow("Background Subtractor");			// Original Video
	namedWindow("Absolute Difference");				// Original Video 2
	namedWindow("Detail");							// threshold of moving objects
	namedWindow("Shape");							// blured threshold of moving objects
	namedWindow("absdiff");							// absdiff of main frame and static image

	while (capSrc.read(main)) {
		Mat diff, result;
		main2 = main.clone();

		if (staticimg.empty()) {
			staticimg = imread("C:/NoObjects.png", CV_LOAD_IMAGE_UNCHANGED);
		}

		absdiff(main, staticimg, diff);
		threshold(diff, result, 60, 255, CV_THRESH_BINARY);
		Mat diff_original = diff.clone();
		cvtColor(diff_original, diff_original, CV_BGR2GRAY);
		threshold(diff_original, diff_original, 60, 255, CV_THRESH_BINARY);
		blur(diff_original, diff_original, Size(10, 10));
		imshow("absdiff", diff_original);

		Mat total = result;
		bg.operator()(total, detail);
		bg.getBackgroundImage(diff);
		imshow("Detail", detail);

		bg2.operator()(main, detail2);
		bg2.getBackgroundImage(NoObject);
		erode(detail2, detail2, Mat());

		dst = detail & detail2;
		blur(dst, dst, Size(10, 10));			// bluring multiple times in different method gives
		blur(dst, dst, Size(10, 10));			// better results
		blur(dst, dst, Size(10, 10));
		threshold(dst, dst, 20, 255, THRESH_BINARY);
		imshow("Shape", dst);

		vector<vector<Point>> contours;
		findContours(dst, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

		vector<vector<Point> > contours_poly(contours.size());
		vector<Rect> boundRect(contours.size());
		for (int i = 0; i < contours.size(); i++) {
			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
		}

		for (int i = 0; i< contours.size(); i++) {
			Scalar color = Scalar(0, 255, 0);
			drawContours(main, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point());
			rectangle(main, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
		}

		vector<vector<Point>> contours2;
		findContours(diff_original, contours2, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

		vector<vector<Point> > contours_poly2(contours2.size());
		vector<Rect> boundRect2(contours2.size());
		for (int i = 0; i < contours2.size(); i++) {
			approxPolyDP(Mat(contours2[i]), contours_poly2[i], 3, true);
			boundRect2[i] = boundingRect(Mat(contours_poly2[i]));
		}

		for (int i = 0; i< contours2.size(); i++) {
			Scalar color = Scalar(0, 255, 0);
			drawContours(main2, contours_poly2, i, color, 1, 8, vector<Vec4i>(), 0, Point());
			rectangle(main2, boundRect2[i].tl(), boundRect2[i].br(), color, 2, 8, 0);
		}

		oVideoWriter.write(main);
		oVideoWriter2.write(main2);
		imshow("Background Subtractor", main);
		imshow("Absolute Difference", main2);
		waitKey(1);
	}

	capSrc.release();
	destroyAllWindows();
}