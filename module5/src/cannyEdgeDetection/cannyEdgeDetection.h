/* 
  Example use of openCV to use the Canny edge detector
  ----------------------------------------------------
 
  (This is the interface file: it contains the declarations of dedicated functions to implement the application.
  These function are called by client code in the application file. The functions are defined in the implementation file.)

  David Vernon
  24 November 2017
*/
 
#define ROS_PACKAGE_NAME "lectures"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <ctype.h>
#include <iostream>
#include <string>
#include <sys/select.h>
#include <termios.h>
#include <stropts.h>
#include <sys/ioctl.h>

//opencv
#include <cv.h>
#include <highgui.h>
#include <opencv2/opencv.hpp>



// Must be included after opencv2/opencv.hpp to avoid incompatiability
#include <ncurses.h>
#include <ros/ros.h>
#include <ros/package.h>


#define TRUE  1
#define FALSE 0
#define MAX_STRING_LENGTH   80
#define MAX_FILENAME_LENGTH 200

using namespace std;
using namespace cv;

/* function prototypes go here */

void CannyThreshold(int, void*);
void prompt_and_exit(int status);
void prompt_and_continue();
int _kbhit();