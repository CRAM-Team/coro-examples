/* 
  Example use of openCV to compute the 3x4 camera model matrix
  ------------------------------------------------------------
 
  (This is the interface file: it contains the declarations of dedicated functions to implement the application.
  These function are called by client code in the application file. The functions are defined in the implementation file.)

  David Vernon
  24 November 2017
*/
 

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <ctype.h>
#include <iostream>
#include <string>
#include <conio.h>

//opencv
#include <cv.h>
#include <highgui.h>
#include <opencv2/opencv.hpp>
 

#define TRUE  1
#define FALSE 0
#define MAX_STRING_LENGTH 80
#define MAX_FILENAME_LENGTH 80
#define MAX_NUMBER_OF_CONTROL_POINTS 500 
#define NUMBER_OF_UNKNOWNS 11 

using namespace std;
using namespace cv;

struct imagePointType {
   int u, v;
};

struct worldPointType {
   float x, y, z;
};

/* function prototypes go here */ 
void computeCameraModel(int numberOfControlPoints, worldPointType worldPoints[], imagePointType imagePoints[], double cameraModel[][4]);
void prompt_and_exit(int status);
void prompt_and_continue();