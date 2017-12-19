#include <opencv2/opencv.hpp>
using namespace cv;
extern void mergeLines(std::vector<Vec2f> *lines, Mat &img){
	std::vector<Vec2f>::iterator cur;
  for (cur=lines->begin();cur!=lines->end();cur++){
    if((*cur)[0]==0 && (*cur)[1]==-100) continue;
    float p1 = (*cur)[0], theta1 = (*cur)[1];
    Point pt1cur, pt2cur;
    if(theta1>CV_PI*45/180 && theta1<CV_PI*135/180) {
      pt1cur.x=0;
      pt1cur.y = p1/sin(theta1);
      pt2cur.x=img.size().width;
      pt2cur.y=-pt2cur.x/tan(theta1) + p1/sin(theta1);
    }   
    else {
      pt1cur.y=0;
      pt1cur.x=p1/cos(theta1);
      pt2cur.y=img.size().height;
      pt2cur.x=-pt2cur.y/tan(theta1) + p1/cos(theta1);
    }   

		std::vector<Vec2f>::iterator pos;
    for(pos=lines->begin();pos!=lines->end();pos++) { if(*cur==*pos) continue; }
    if(fabs((*pos)[0]-(*cur)[0])<20 && fabs((*pos)[1]-(*cur)[1])<CV_PI*10/180) {
      float p = (*pos)[0], theta = (*pos)[1];
      Point pt1, pt2;
      if((*pos)[1]>CV_PI*45/180 && (*pos)[1]<CV_PI*135/180) {
        pt1.x=0; pt1.y = p/sin(theta);
        pt2.x=img.size().width;
        pt2.y=-pt2.x/tan(theta) + p/sin(theta);
      }
      else {
        pt1.y=0;
        pt1.x=p/cos(theta);
        pt2.y=img.size().height;
        pt2.x=-pt2.y/tan(theta) + p/cos(theta);
      }
      if(((double)(pt1.x-pt1cur.x)*(pt1.x-pt1cur.x) + (pt1.y-pt1cur.y)*(pt1.y-pt1cur.y)<64*64) &&
          ((double)(pt2.x-pt2cur.x)*(pt2.x-pt2cur.x) + (pt2.y-pt2cur.y)*(pt2.y-pt2cur.y)<64*64)) {
          // Merge the two
          (*cur)[0] = ((*cur)[0]+(*pos)[0])/2;
          (*cur)[1] = ((*cur)[1]+(*pos)[1])/2;
          (*pos)[0]=0; (*pos)[1]=-100;
      }
    }
  }
}
void drawLine(Vec2f line, Mat &img, Scalar rgb = CV_RGB(0,0,255)) {
  if(line[1]!=0) {
      float m = -1/tan(line[1]), c = line[0]/sin(line[1]);
      cv::line(img, Point(0, c), Point(img.size().width, m*img.size().width+c), rgb);
  }
  else cv::line(img, Point(line[0], 0), Point(line[0], img.size().height), rgb);
}
