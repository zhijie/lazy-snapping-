/* author: zhijie Lee
 * home page: lzhj.me
 * 2012-02-06
 */
#include <cv.h>
#include <highgui.h>
#include "maxflow-v3.01/graph.h"
#include <vector>
#include <iostream>
#include <cmath>
using namespace std;

typedef Graph<float,float,float> GraphType;
class LasySnapping{
    
public :
    LasySnapping():graph(NULL){
        avgForeColor ={0,0,0};
        avgBackColor ={0,0,0};
    }
    ~LasySnapping(){ 
        if(graph){
            delete graph;
        }
    };
private :
    vector<CvPoint> forePts;
    vector<CvPoint> backPts;
    IplImage* image;
    // average color of foreground points
    unsigned char avgForeColor[3];
    // average color of background points
    unsigned char avgBackColor[3];
public :
    void setImage(IplImage* image){
        this->image = image;
        graph = new GraphType(image->width*image->height,image->width*image->height*2);
    }
    // include-pen locus
    void setForegroundPoints(vector<CvPoint> pts){
        forePts.clear();
        for(int i =0; i< pts.size(); i++){
            if(!isPtInVector(pts[i],forePts)){
                forePts.push_back(pts[i]);
            }
        }
        if(forePts.size() == 0){
            return;
        }
        int sum[3] = {0};
        for(int i =0; i < forePts.size(); i++){
            unsigned char* p = (unsigned char*)image->imageData + forePts[i].x * 3 
                + forePts[i].y*image->widthStep;
            sum[0] += p[0];
            sum[1] += p[1];
            sum[2] += p[2];            
        }
        cout<<sum[0]<<" " <<forePts.size()<<endl;
        avgForeColor[0] = sum[0]/forePts.size();
        avgForeColor[1] = sum[1]/forePts.size();
        avgForeColor[2] = sum[2]/forePts.size();
    }
    // exclude-pen locus
    void setBackgroundPoints(vector<CvPoint> pts){
        backPts.clear();
        for(int i =0; i< pts.size(); i++){
            if(!isPtInVector(pts[i],backPts)){
                backPts.push_back(pts[i]);
            }
        }
        if(backPts.size() == 0){
            return;
        }
        int sum[3] = {0};
        for(int i =0; i < backPts.size(); i++){
            unsigned char* p = (unsigned char*)image->imageData + backPts[i].x * 3 + 
                backPts[i].y*image->widthStep;
            sum[0] += p[0];
            sum[1] += p[1];
            sum[2] += p[2];            
        }
        avgBackColor[0] = sum[0]/backPts.size();
        avgBackColor[1] = sum[1]/backPts.size();
        avgBackColor[2] = sum[2]/backPts.size();
    }
    // return maxflow of graph
    int runMaxflow();
    // get result, a grayscale mast image indicating forground by 255 and background by 0
    IplImage* getImageMask();
private :
    float colorDistance(unsigned char* color1, unsigned char* color2);
    float minDistance(unsigned char* color, vector<CvPoint> points);
    bool isPtInVector(CvPoint pt, vector<CvPoint> points);
    void getE1(unsigned char* color,float* energy);
    float getE2(unsigned char* color1,unsigned char* color2);
    
    GraphType *graph;    
};

float LasySnapping::colorDistance(unsigned char* color1, unsigned char* color2)
{
    return sqrt((color1[0]-color2[0])*(color1[0]-color2[0])+
        (color1[1]-color2[1])*(color1[1]-color2[1])+
        (color1[2]-color2[2])*(color1[2]-color2[2]));    
}

float LasySnapping::minDistance(unsigned char* color, vector<CvPoint> points)
{
    float distance = -1;
    for(int i =0 ; i < points.size(); i++){
        unsigned char* p = (unsigned char*)image->imageData + points[i].y * image->widthStep + 
            points[i].x * image->nChannels;
        float d = colorDistance(p,color);
        if(distance < 0 ){
            distance = d;
        }else{
            if(distance > d){
                distance = d;
            }
        }
    }
}
bool LasySnapping::isPtInVector(CvPoint pt, vector<CvPoint> points)
{
    for(int i =0 ; i < points.size(); i++){
        if(pt.x == points[i].x && pt.y == points[i].y){
            return true;
        }
    }
    return false;
}
void LasySnapping::getE1(unsigned char* color,float* energy)
{
    // average distance
    float df = colorDistance(color,avgForeColor);
    float db = colorDistance(color,avgBackColor);
    // min distance from background points and forground points
    // float df = minDistance(color,forePts);
    // float db = minDistance(color,backPts);
    energy[0] = df/(db+df);
    energy[1] = db/(db+df);
}
float LasySnapping::getE2(unsigned char* color1,unsigned char* color2)
{
    const float EPSILON = 0.01;
    float lambda = 100;
    return lambda/(EPSILON+
        (color1[0]-color2[0])*(color1[0]-color2[0])+
        (color1[1]-color2[1])*(color1[1]-color2[1])+
        (color1[2]-color2[2])*(color1[2]-color2[2]));
}

int LasySnapping::runMaxflow()
{   
    const float INFINNITE_MAX = 1e10;
    int indexPt = 0;
    for(int h = 0; h < image->height; h ++){
        unsigned char* p = (unsigned char*)image->imageData + h *image->widthStep;
        for(int w = 0; w < image->width; w ++){
            // calculate energe E1
            float e1[2]={0};
            if(isPtInVector(cvPoint(w,h),forePts)){
                e1[0] =0;
                e1[1] = INFINNITE_MAX;
            }else if(isPtInVector(cvPoint(w,h),backPts)){
                e1[0] = INFINNITE_MAX;
                e1[1] = 0;
            }else {
                getE1(p,e1);
            }
            // add node
            graph->add_node();
            graph->add_tweights(indexPt, e1[0],e1[1]);
            // add edge, 4-connect
            if(h > 0 && w > 0){
                float e2 = getE2(p,p-3);
                graph->add_edge(indexPt,indexPt-1,e2,e2);
                e2 = getE2(p,p-image->widthStep);
                graph->add_edge(indexPt,indexPt-image->width,e2,e2);
            }
            
            p+= 3;
            indexPt ++;            
        }
    }
    
    return graph->maxflow();
}
IplImage* LasySnapping::getImageMask()
{
    IplImage* gray = cvCreateImage(cvGetSize(image),8,1); 
    int indexPt =0;
    for(int h =0; h < image->height; h++){
        unsigned char* p = (unsigned char*)gray->imageData + h*gray->widthStep;
        for(int w =0 ;w <image->width; w++){
            if (graph->what_segment(indexPt) == GraphType::SOURCE){
                *p = 0;
            }else{
                *p = 255;
            }
            p++;
            indexPt ++;
        }
    }
    return gray;
}

// global
vector<CvPoint> forePts;
vector<CvPoint> backPts;
int currentMode = 0;// indicate foreground or background, foreground as default
CvScalar paintColor[2] = {CV_RGB(0,0,255),CV_RGB(255,0,0)};

IplImage* image = NULL;
char* winName = "lazySnapping";
IplImage* imageDraw = NULL;
const int SCALE = 4;

void on_mouse( int event, int x, int y, int flags, void* )
{    
    if( event == CV_EVENT_LBUTTONUP ){
        if(backPts.size() == 0 && forePts.size() == 0){
            return;
        }
        LasySnapping ls;
        IplImage* imageLS = cvCreateImage(cvSize(image->width/SCALE,image->height/SCALE),
            8,3);
        cvResize(image,imageLS);
        ls.setImage(imageLS);
        ls.setBackgroundPoints(backPts);
        ls.setForegroundPoints(forePts);
        ls.runMaxflow();
        IplImage* mask = ls.getImageMask();
        IplImage* gray = cvCreateImage(cvGetSize(image),8,1);
        cvResize(mask,gray);
        // edge
        cvCanny(gray,gray,50,150,3);
        
        IplImage* showImg = cvCloneImage(imageDraw);
        for(int h =0; h < image->height; h ++){
            unsigned char* pgray = (unsigned char*)gray->imageData + gray->widthStep*h;
            unsigned char* pimage = (unsigned char*)showImg->imageData + showImg->widthStep*h;
            for(int width  =0; width < image->width; width++){
                if(*pgray++ != 0 ){
                    pimage[0] = 0;
                    pimage[1] = 255;
                    pimage[2] = 0;
                }
                pimage+=3;                
            }
        }
        cvSaveImage("t.bmp",showImg);
        cvShowImage(winName,showImg);
        cvReleaseImage(&imageLS);
        cvReleaseImage(&mask);
        cvReleaseImage(&showImg);
        cvReleaseImage(&gray);
    }else if( event == CV_EVENT_LBUTTONDOWN ){
    }else if( event == CV_EVENT_MOUSEMOVE && (flags & CV_EVENT_FLAG_LBUTTON)){
        CvPoint pt = cvPoint(x,y);
        if(currentMode == 0){//foreground
            forePts.push_back(cvPoint(x/SCALE,y/SCALE));
        }else{//background
            backPts.push_back(cvPoint(x/SCALE,y/SCALE));
        }
        cvCircle(imageDraw,pt,2,paintColor[currentMode]);
        cvShowImage(winName,imageDraw);
    }
}
int main(int argc, char** argv)
{	
    if(argc != 2){
        cout<<"command : lazysnapping inputImage"<<endl;
        return 0;
    }
    cvNamedWindow(winName,1);
    cvSetMouseCallback( winName, on_mouse, 0);
    
    image = cvLoadImage(argv[1],CV_LOAD_IMAGE_COLOR);
    imageDraw = cvCloneImage(image);
    cvShowImage(winName, image);
    for(;;){
        int c = cvWaitKey(0);
        c = (char)c;
        if(c == 27){//exit
            break;
        }else if(c == 'r'){//reset
            image = cvLoadImage(argv[1],CV_LOAD_IMAGE_COLOR);
            imageDraw = cvCloneImage(image);
            forePts.clear();
            backPts.clear();
            currentMode = 0;
            cvShowImage(winName, image);
        }else if(c == 'b'){//change to background selection
            currentMode = 1;
        }else if(c == 'f'){//change to foreground selection
            currentMode = 0;
        }
    }
    cvReleaseImage(&image);
    cvReleaseImage(&imageDraw);
	return 0;
}
