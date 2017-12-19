#include <stdio.h>
#include <raspicam/raspicam_cv.h>
#include "myfunc.cc"
//merge lines so can find out exactly where the puzzle is.
using namespace cv;
using namespace cv::ml;
using namespace raspicam;


int main(){

	for (int i=0; i<10; i++){
		//read image and check it
		
		RaspiCam_Cv cam;
		Mat rImage;
		Mat image;

		cam.set(CV_CAP_PROP_FORMAT, CV_8UC3);
		cam.set(CV_CAP_PROP_FRAME_WIDTH, 640);
		cam.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
		if(!cam.open()){
			printf("*** ALERT *** CAM ERROR !!!!!\n\n");
			return -1;

		}
		cam.grab();


	  if (i){
			cam.retrieve(rImage);
			imwrite("shot.jpg", rImage);
			image = imread("shot.jpg", 0);
		}
		else image = imread("image.jpg", 0);

		if ( !image.data ) { printf("No image data \n"); return -1; }
		namedWindow("Display Image", WINDOW_AUTOSIZE );
		imshow("Display Image", image);
		waitKey(0);

		Mat outerBox = Mat(image.size(), CV_8UC1);
		GaussianBlur(image, image, Size(11,11), 0);
		adaptiveThreshold(image, outerBox, 255, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 5, 2);
		bitwise_not(outerBox, outerBox);

		Mat kernel = (Mat_<uchar>(3,3) << 0,1,0,1,1,1,0,1,0);
		dilate(outerBox, outerBox, kernel);

		//finding the biggest blob with floodfill function
		int max=-1; Point maxPt;

		for(int y=0;y<outerBox.size().height;y++) {
			uchar *row = outerBox.ptr(y);
			for(int x=0;x<outerBox.size().width;x++) {
				if(row[x]>=128) {
					int area = floodFill(outerBox, Point(x,y), CV_RGB(0,0,64));
					if(area>max) {
						maxPt = Point(x,y);
						max = area;
					}
				}
			}
		}

		imshow("floodfill", outerBox);
		waitKey(0);
			//we have several blobs filled with a dark gray colour(64) and we also know the
		//point what produces a blob with maximum area. so we floodfill that point with white
		floodFill(outerBox, maxPt, CV_RGB(255,255,255));

		imshow("floodfill again", outerBox);
		waitKey(0);

		//now, the biggest blob is white. we need to turn the other blobs black.
		for(int y=0;y<outerBox.size().height;y++) {
			uchar *row = outerBox.ptr(y);
			for(int x=0;x<outerBox.size().width;x++) {
				if(row[x]==64 && x!=maxPt.x && y!=maxPt.y) {
					int area = floodFill(outerBox, Point(x,y), CV_RGB(0,0,0));
				}
			}
		}

		imshow("biggest one remained white, others turn to black", outerBox);
		waitKey(0);

		//restore image from dilated one.
		erode(outerBox, outerBox, kernel);
		imshow("thresholded", outerBox);
		waitKey(0);

		//detecting lines with hough transfrom
		std::vector<Vec2f> lines;
		HoughLines(outerBox, lines, 1, CV_PI/180, 200);
		mergeLines(&lines, image);

		for(int i=0;i<lines.size();i++)  drawLine(lines[i], outerBox, CV_RGB(0,0,128));

		imshow("detecting lines", outerBox);
		waitKey(0);

		// Now detect the lines on extremes
		Vec2f topEdge = Vec2f(1000,1000);    double topYIntercept=100000, topXIntercept=0;
		Vec2f bottomEdge = Vec2f(-1000,-1000);        double bottomYIntercept=0, bottomXIntercept=0;
		Vec2f leftEdge = Vec2f(1000,1000);    double leftXIntercept=100000, leftYIntercept=0;
		Vec2f rightEdge = Vec2f(-1000,-1000);        double rightXIntercept=0, rightYIntercept=0;

		for(int i=0;i<lines.size();i++) {
			Vec2f current = lines[i];
			float rho=current[0], theta=current[1];
			if(rho==0 && theta==-100) continue;
			double xIntercept, yIntercept;
			xIntercept = rho/cos(theta); yIntercept = rho/(cos(theta)*sin(theta));
			if(theta>CV_PI*80/180 && theta<CV_PI*100/180) {
				if(rho<topEdge[0]) topEdge = current;
				if(rho>bottomEdge[0]) bottomEdge = current;
			}
			else if(theta<CV_PI*10/180 || theta>CV_PI*170/180) {
				if(xIntercept>rightXIntercept) {
					rightEdge = current; rightXIntercept = xIntercept;
				}
				else if(xIntercept<=leftXIntercept) {
					leftEdge = current; leftXIntercept = xIntercept;
				}
			}
		}

		drawLine(topEdge, image, CV_RGB(0,0,0)); drawLine(bottomEdge, image, CV_RGB(0,0,0));
		drawLine(leftEdge, image, CV_RGB(0,0,0)); drawLine(rightEdge, image, CV_RGB(0,0,0));
		imshow("detecting lines2", image);
		waitKey(0);

		Point left1, left2, right1, right2, bottom1, bottom2, top1, top2;
		int height=outerBox.size().height, width=outerBox.size().width;

		if(leftEdge[1]!=0) {
			left1.x=0;        left1.y=leftEdge[0]/sin(leftEdge[1]);
			left2.x=width;    left2.y=-left2.x/tan(leftEdge[1]) + left1.y;
		}
		else {
			left1.y=0;        left1.x=leftEdge[0]/cos(leftEdge[1]);
			left2.y=height;    left2.x=left1.x - height*tan(leftEdge[1]);
		}

		if(rightEdge[1]!=0) {
			right1.x=0;        right1.y=rightEdge[0]/sin(rightEdge[1]);
			right2.x=width;    right2.y=-right2.x/tan(rightEdge[1]) + right1.y;
		}
		else {
			right1.y=0;        right1.x=rightEdge[0]/cos(rightEdge[1]);
			right2.y=height;    right2.x=right1.x - height*tan(rightEdge[1]);
		}

		bottom1.x=0;    bottom1.y=bottomEdge[0]/sin(bottomEdge[1]);
		bottom2.x=width;bottom2.y=-bottom2.x/tan(bottomEdge[1]) + bottom1.y;

		top1.x=0;        top1.y=topEdge[0]/sin(topEdge[1]);
		top2.x=width;    top2.y=-top2.x/tan(topEdge[1]) + top1.y;

		// Next, we find the intersection of  these four lines
		double leftA = left2.y-left1.y, leftB = left1.x-left2.x;
		double leftC = leftA*left1.x + leftB*left1.y;

		double rightA = right2.y-right1.y, rightB = right1.x-right2.x;
		double rightC = rightA*right1.x + rightB*right1.y;

		double topA = top2.y-top1.y, topB = top1.x-top2.x;
		double topC = topA*top1.x + topB*top1.y;

		double bottomA = bottom2.y-bottom1.y, bottomB = bottom1.x-bottom2.x;
		double bottomC = bottomA*bottom1.x + bottomB*bottom1.y;

		// Intersection of left and top
		double detTopLeft = leftA*topB - leftB*topA;
		CvPoint ptTopLeft = cvPoint((topB*leftC - leftB*topC)/detTopLeft, (leftA*topC - topA*leftC)/detTopLeft);

		// Intersection of top and right
		double detTopRight = rightA*topB - rightB*topA;

		CvPoint ptTopRight = cvPoint((topB*rightC-rightB*topC)/detTopRight, (rightA*topC-topA*rightC)/detTopRight);

		// Intersection of right and bottom
		double detBottomRight = rightA*bottomB - rightB*bottomA;
		CvPoint ptBottomRight = cvPoint((bottomB*rightC-rightB*bottomC)/detBottomRight, (rightA*bottomC-bottomA*rightC)/detBottomRight);
		// Intersection of bottom and left
		double detBottomLeft = leftA*bottomB-leftB*bottomA;
		CvPoint ptBottomLeft = cvPoint((bottomB*leftC-leftB*bottomC)/detBottomLeft, (leftA*bottomC-bottomA*leftC)/detBottomLeft);

		int maxLength = (ptBottomLeft.x-ptBottomRight.x)*(ptBottomLeft.x-ptBottomRight.x) + (ptBottomLeft.y-ptBottomRight.y)*(ptBottomLeft.y-ptBottomRight.y);
		int temp = (ptTopRight.x-ptBottomRight.x)*(ptTopRight.x-ptBottomRight.x) + (ptTopRight.y-ptBottomRight.y)*(ptTopRight.y-ptBottomRight.y);
		if(temp>maxLength) maxLength = temp;
		temp = (ptTopRight.x-ptTopLeft.x)*(ptTopRight.x-ptTopLeft.x) + (ptTopRight.y-ptTopLeft.y)*(ptTopRight.y-ptTopLeft.y);
		if(temp>maxLength) maxLength = temp;
		temp = (ptBottomLeft.x-ptTopLeft.x)*(ptBottomLeft.x-ptTopLeft.x) + (ptBottomLeft.y-ptTopLeft.y)*(ptBottomLeft.y-ptTopLeft.y);
		if(temp>maxLength) maxLength = temp;
		maxLength = sqrt((double)maxLength);

		Point2f src[4], dst[4];
		src[0] = ptTopLeft;            dst[0] = Point2f(0,0);
		src[1] = ptTopRight;        dst[1] = Point2f(maxLength-1, 0);
		src[2] = ptBottomRight;        dst[2] = Point2f(maxLength-1, maxLength-1);
		src[3] = ptBottomLeft;        dst[3] = Point2f(0, maxLength-1);
		Mat undistorted = Mat(Size(maxLength, maxLength), CV_8UC1);
		cv::warpPerspective(image, undistorted, cv::getPerspectiveTransform(src, dst), Size(maxLength, maxLength));

		imshow("undistorted", undistorted);
		waitKey(0);
		
	 /* 
		Mat undistortedThreshed = undistorted.clone();
		adaptiveThreshold(undistorted, undistortedThreshed, 255, CV_ADAPTIVE_THRESH_GAUSSIAN_C, CV_THRESH_BINARY_INV, 101, 1);

		DigitRecognizer *dr = new DigitRecognizer();
		bool b = dr->train("D:/Test/Character Recognition/train-images.idx3-ubyte", "D:/Test/Character Recognition/train-labels.idx1-ubyte");

		int dist = ceil((double)maxLength/9);
		Mat currentCell = Mat(dist, dist, CV_8UC1);

		for(int j=0;j<9;j++) {
			for(int i=0;i<9;i++) {
				for(int y=0;y<dist && j*dist+y<undistortedThreshed.cols;y++) {
					uchar* ptr = currentCell.ptr(y);
					for(int x=0;x<dist && i*dist+x<undistortedThreshed.rows;x++) {
						ptr[x] = undistortedThreshed.at<uchar>(j*dist+y, i*dist+x);
					}
				}
				Moments m = moments(currentCell, true);
				int area = m.m00;
				if(area > currentCell.rows*currentCell.cols/5) {
					int number = dr->classify(currentCell);
					printf("%d ", number);
				}
				else printf("  ");
			}
			printf(" ");
		}
		*/
		destroyAllWindows();
	}
}

