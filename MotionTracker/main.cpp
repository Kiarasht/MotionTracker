#include "opencv2/highgui/highgui.hpp"
#include "opencv2/video/video.hpp"
#include <windows.h>
#include <iostream>
#include <vector>

using namespace std;
using namespace cv;

Mat staticimg;

void GetStatic(String);
void MotionTracker(String, String);


/**
* Start of the program where we start by defining a global variable that defines a static image.
* We also define our paths and call our two main functions that are then responsible of detecting
* and tracking the objects. To make this program work with other files we just need to change the
* values of insert and output.
*
* @param argc N/A
* @param argv N/A
*/
int main(int argc, const char** argv)
{
	staticimg = 0;
	String insert = "C:/funny.avi";
	String output = "C:/funny_output.avi";

	GetStatic(insert);
	Sleep(2000);
	MotionTracker(insert, output);
	return 0;
}

/**
* GetStatic is responsible of blending a video frame by frame until the end. It then creates an image
* based on the results. Any object that has been moving will slowly diappear while static objects will
* remain in the picture. The image will later on be used to detect moving objects.
*
* @param insert Path to a valid video that needs to be detected.
*/
void GetStatic(String insert){

	VideoCapture capSrc(insert);
	Mat inputframe, blendframe;
	double alpha = 0.01;
	double beta = 1 - alpha;

	namedWindow("Original", CV_WINDOW_AUTOSIZE);
	namedWindow("Difference", CV_WINDOW_AUTOSIZE);

	capSrc.read(blendframe);
	while (capSrc.read(inputframe)) {
		addWeighted(inputframe, alpha, blendframe, beta, 0.0, blendframe);
		staticimg = blendframe;

		imshow("Original", inputframe);
		imshow("Difference", blendframe);
		waitKey(1);
	}

	imwrite("C:/static.png", staticimg);
	capSrc.release();
	Sleep(2000);
	destroyAllWindows();
}


/**
* We will once again will go through our video frame by frame to find and detect the moving 
* objects. Here we will use our staticimg and find its difference from the current
* frame which will give us the objects that are changing their positions using a 
* BackgroundSubtractorMOG2 class. Another one is also created to use the erode function
* which will help in elimating anything that is static. The results from these two classes
* are then combined and we also get a picture that is the view without any of the moving objects.
*
* @param insert Path to a valid video that needs to be detected.
* @param output Path to where the detection video should be saved.
*/
void MotionTracker(String insert, String output){

	Mat window, main, diff, NoObject, detail, detail2, dst;
	vector<vector<Point>> contours;

	VideoCapture capSrc(insert);
	double fps = capSrc.get(CV_CAP_PROP_FPS);
	capSrc.read(window);
	double dWidth = window.cols;
	double dHeight = window.rows;

	Size frameSize(static_cast<int>(dWidth), static_cast<int>(dHeight));

	VideoWriter oVideoWriter(output, CV_FOURCC('P', 'I', 'M', '1'), fps, frameSize, true);

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

	namedWindow("Main");
	namedWindow("Detail");
	namedWindow("Shape");
	namedWindow("NoObject");

	while (capSrc.read(main)){
		Mat diff, result;

		if (staticimg.empty()){
			staticimg = imread("C:/static.png", CV_LOAD_IMAGE_UNCHANGED);
		}

		absdiff(main, staticimg, diff);
		threshold(diff, result, 60, 255, CV_THRESH_BINARY);

		Mat total = result;
		bg.operator()(total, detail);
		bg.getBackgroundImage(diff);
		imshow("Detail", detail);

		bg2.operator()(main, detail2);
		bg2.getBackgroundImage(NoObject);
		erode(detail2, detail2, Mat());

		dst = detail & detail2;
		blur(dst, dst, Size(10, 10));
		blur(dst, dst, Size(10, 10));
		blur(dst, dst, Size(10, 10));
		threshold(dst, dst, 20, 255, THRESH_BINARY);
		imshow("Shape", dst);

		findContours(dst, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_NONE);

		vector<vector<Point> > contours_poly(contours.size());
		vector<Rect> boundRect(contours.size());
		for (int i = 0; i < contours.size(); i++){
			approxPolyDP(Mat(contours[i]), contours_poly[i], 3, true);
			boundRect[i] = boundingRect(Mat(contours_poly[i]));
		}

		for (int i = 0; i< contours.size(); i++){
			Scalar color = Scalar(0, 255, 0);
			drawContours(main, contours_poly, i, color, 1, 8, vector<Vec4i>(), 0, Point());
			rectangle(main, boundRect[i].tl(), boundRect[i].br(), color, 2, 8, 0);
		}

		oVideoWriter.write(main);
		imshow("Main", main);
		imshow("NoObject", NoObject);
		waitKey(1);
	}

	imwrite("C:/NoObjects.png", NoObject);
	capSrc.release();
	destroyAllWindows();
}