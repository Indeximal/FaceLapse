#include "opencv2/objdetect.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgproc.hpp"

#include "Cascades.h"
#include "Header.h"

#include <iostream>

namespace facelapse {
	cv::CascadeClassifier face_cascade;
	cv::CascadeClassifier eyes_cascade;

	CoordinatePair findEyeCoords(std::string path) {
		// Load image
		cv::Mat fullFrame = cv::imread(path, CV_LOAD_IMAGE_COLOR);
		if (!fullFrame.data) {
			return CoordinatePair(); // Couldnt load
		}

		// Scale for better performance
		float scale = 400.0f / fullFrame.rows;
		cv::Mat frame_gray;
		cv::resize(fullFrame, frame_gray, cv::Size(), scale, scale);
		//std::cout << frame_gray.rows << "h w" << frame_gray.cols << std::endl;

		// Convert to gray
		cv::cvtColor(frame_gray, frame_gray, cv::COLOR_BGR2GRAY);
		cv::equalizeHist(frame_gray, frame_gray);

		// Find faces
		std::vector<cv::Rect> faces;
		face_cascade.detectMultiScale(frame_gray, faces, 1.1, 10);

		// Assure its only 1 face
		if (faces.size() != 1) {
			//std::cout << "faces" << std::endl;
			return CoordinatePair(); // Too many or too few faces
		}

		// Face Rectangle
		cv::Rect faceRect = faces[0];
		//cv::rectangle(frame, faceRect, cv::Scalar(128, 128, 128), 3); // DEBUG
		cv::Mat faceROI = frame_gray(faceRect);

		// Find 2 eyes, using a Haar Cascade with a binary search for the paramenter
		std::vector<cv::Rect> eyes;
		int minE=2, maxE=100;
		while (eyes.size() != 2) {
			int avg = (minE + maxE) / 2;
			if (avg == maxE || avg == minE) break;
			eyes_cascade.detectMultiScale(faceROI, eyes, 1.3, avg);
			//std::cout << "  Eyes parma=" << avg << " -> " << eyes.size() << std::endl; // DEBUG
			if (eyes.size() > 2) {
				minE = avg;
			} else if (eyes.size() < 2) {
				maxE = avg;
			}
		}

		// Assure there are 2 eyes
		if (eyes.size() != 2) {
			//std::cout << "eyes" << eyes.size() << std::endl;
			return CoordinatePair(); // Detected too many or too few eyes 
		}

		// Eye informations
		CoordinatePair cp;
		for (size_t k = 0; k < eyes.size(); k++) {
			float fx = (eyes[k].x + eyes[k].width/2.0) / faceRect.width;

			// Pointers for independency
			float *xCoord, *yCoord;
			if (fx < 0.5) { // Right Eyd
				xCoord = &cp.rX;
				yCoord = &cp.rY;
			} else { // Left Eye
				xCoord = &cp.lX;
				yCoord = &cp.lY;
			}

			// Eye Rectangle
			const float zoom = 0.75;
			cv::Rect rect = cv::Rect(faceRect.x + eyes[k].x + eyes[k].width * (1-zoom) / 2, faceRect.y + eyes[k].y + eyes[k].height * (1-zoom) / 2, eyes[k].width*zoom, eyes[k].height*zoom);
			cv::Rect fullRect = cv::Rect(rect.x / scale, rect.y / scale, rect.width / scale, rect.height / scale);

			// std::cout << fullRect.height << "h w" << fullRect.width << std::endl;

			// Gray, blurred ROI
			cv::Mat eyeROI = fullFrame(fullRect);
			float eyeScale = 70.0f / fullRect.height;
			cv::resize(eyeROI, eyeROI, cv::Size(), eyeScale, eyeScale);
			cv::cvtColor(eyeROI, eyeROI, cv::COLOR_BGR2GRAY);
			cv::equalizeHist(eyeROI, eyeROI);
			// cv::medianBlur(eyeROI, eyeROI, 5);
			cv::threshold(eyeROI, eyeROI, 40, 255, cv::THRESH_BINARY);
			// cv::imshow("eye", eyeROI);

			// One eye per eye please
			const int wantedCircles = 1;
			const float minRadF = 0.17;
			const float maxRadF = 0.33;

			// Find the center in the gray scale image, using HoughCircles with a binary search for param2
			std::vector<cv::Vec3f> circles;
			int min = 5, max = 50;
			while (circles.size() != wantedCircles) {
				int avg = (min + max) / 2;
				if (avg == max || avg == min) break; 
				// Actual search
				cv::HoughCircles(eyeROI, circles, CV_HOUGH_GRADIENT, 1, eyeROI.rows, 50, avg, eyeROI.rows * minRadF, eyeROI.rows * maxRadF);
				//std::cout << "  Gray parma2=" << avg << " -> " << circles.size() << std::endl; // DEBUG
				if (circles.size() > wantedCircles) {
					min = avg;
				} else if (circles.size() < wantedCircles) {
					max = avg;
				}
			}

			if (circles.size() != wantedCircles) {
				continue;
			}

			//std::cout << "  sum : " <<  avg[0] << " "<< avg[1] << " r" << (float)avg[2]/eyeROI.rows << std::endl;
			*xCoord = circles[0][0] / eyeScale + fullRect.x;
			*yCoord = circles[0][1] / eyeScale + fullRect.y;
		}
		//std::cout << cp.rX << " " << cp.rY << std::endl;

		return cp;
	}

	void initCascades() {
		cv::FileStorage fsEye(EYE_CASCADE_STR, cv::FileStorage::MEMORY);
		eyes_cascade.read(fsEye.getFirstTopLevelNode());

		cv::FileStorage fsFace(FACE_CASCADE_STR, cv::FileStorage::MEMORY);
		face_cascade.read(fsFace.getFirstTopLevelNode());
	}

	/*int emain(int argc, const char** argv) {

		// Find eyes
		//cv::String image_name = parser.get<cv::String>("image");
		
		for (int i = 1; i < argc; ++i) {
			cv::String path(argv[i]);
			auto ret = findEyeCoords(path, 0.5);

			if (!ret.success) {
				std::cerr << "Error in picutre " << path << ": " << ret.errorStr << std::endl;
			} else {
				std::cout << path << ": R(" << ret.rightX << "," << ret.rightY << ") L(" << ret.leftX << "," << ret.leftY << ")" << std::endl;
				
				cv::Mat img = cv::imread(path, CV_LOAD_IMAGE_COLOR);
				const float zoom = 0.3;
				cv::resize(img, img, cv::Size(), zoom, zoom);
				cv::circle(img, cv::Point(ret.rightX*zoom, ret.rightY*zoom), 10, cv::Scalar(255, 0, 0), 2);
				cv::circle(img, cv::Point(ret.leftX*zoom, ret.leftY*zoom), 10, cv::Scalar(0, 0, 255), 2);
				cv::imshow("Display", img);
				if (cv::waitKey(0) == 27) {
					break;
				}
			}
		}

		return 0;
	}*/
}

