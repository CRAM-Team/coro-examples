/* 
  Example use of openCV to perform colour segmentation
   
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
#include <sys/types.h> 
#include <sys/timeb.h>

//opencv
#include <cv.h>
#include <highgui.h>
#include <opencv2/opencv.hpp>


#define TRUE  1
#define FALSE 0
#define MAX_STRING_LENGTH 80
#define MAX_FILENAME_LENGTH 80

using namespace std;
using namespace cv;

/* function prototypes go here */

void colourSegmentation(int, void*);
void getSamplePoint( int event, int x, int y, int, void*);
void prompt_and_exit(int status);
void prompt_and_continue();
void pause(int milliseconds);