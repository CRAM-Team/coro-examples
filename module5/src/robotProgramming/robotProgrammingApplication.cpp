/*******************************************************************************************************************
*   Task-level robot programming for a LynxMotion AL5D robot arm
*   ------------------------------------------------------------
*
*   This application implements robot programming with frames (homogeneous transformations)
*   using the Frame and Vector classes and auxilliary friend functions, all defined in the interface file
*  
*   The frames are defined in a Cartesian frame of reference and use an inverse kinematic model 
*   to map to the robot joint space.
*
*   The solution to the inverse kinematic problem is based on original code by 
*   Oleg Mazurov:    www.circuitsathome.com/mcu/robotic-arm-inverse-kinematics-on-arduino
*   and subsuquently adapted by Eric Goldsmith:  www.ericgoldsmith.com
*   
*   See the following for a figure to explain the variables in the inverse kinematic solution:
*   github.com/EricGoldsmith/AL5D-BotBoarduino-PS2/blob/master/Robot_Arm_IK.pdf
*
*   Also, refer to the CR_13.pdf lectures slides on www.cognitiverobotics.net for more details on the kinematics 
*   and the solution to the inverse kinematics.
*
*   This inverse kinematic solution has been modified to make it compatible with the task-level 
*   robot program specification and so that T5 is embedded in the wrist.
*
*   The serial port interface to the AL5D robot was written by Victor Akinwande, Carnegie Mellon University Africa 
*   Refer to lynxmotion_ssc-32u_usb_user_guide.pdf, p. 24, for details of the serial port command protocol.
*
*   Different AL5D robots are accommodated by reading the required calibration data from a robot-specific configuration file.
*   The configuration filename is provided in the robotProgrammingInput.txt file.
*   This allows the application to be easily adapted to the different robots used in class exercises.
*   The configuration file contains the following data, specified using key-value pairs
*
*   COM     <serial port name>
*   BAUD    <rate>
*   SPEED   <value>
*   CHANNEL <servo 1 pin number> <servo 2 pin number> <servo 3 pin number> <servo 4 pin number> <servo 5 pin number> <servo 6 pin number> 
*   HOME    <servo 1 setpoint>   <servo 2 setpoint>   <servo 3 setpoint>   <servo 4 setpoint>   <servo 5 setpoint>   <servo 6 setpoint> 
*   DEGREE  <servo 1 pulses>     <servo 2 pulses>     <servo 3 pulses>     <servo 4 pulses>     <servo 5 pulses>     <servo 6 pulses>
*   WRIST   <wrist type>
*   DEFAULT <joint 1 angle> <joint 2 angle> <joint 3 angle> <joint 4 angle> <joint 5 angle> <gripper distance>
*
*   The serial port name is assigned automatically by the operating system when the robot's USB-to-serial port is connected to the computer
*   e.g., COM0, COM6, ... 
*
*   The baud rate is also assigned automatically by the operating system when the robot's USB-to-serial port is connected to the computer
*   e.g., 9600
*
*   The speed value determines the default servo speed.  This is specified microseconds per second, e.g. 500
*   "For a better understanding of the speed argument, consider that 1000uS of travel will result in around 90� of rotation. 
*   A speed value of 100uS per second means the servo will take 10 seconds (divide 1000 by 100) to move 90�. 
*   Alternately, a speed value of 2000uS per second equates to 500mS (half a second) to move 90� (divide 1000 by 2000)."
*   Taken from lynxmotion_ssc-32u_usb_user_guide.pdf
*
*   The channel data specifies which pins on the SSC-32U servo-controller board are used for each servo motor.
*   Normally, servo 1 is controlled using pin 0, servo 2 using pin 1, ...   
*   However, sometimes different pins are used, e.g. in the case of the servo-controller for robot 1 where pin 2 malfunctions
*   and instead pin 6 is used to control servo 3.
*
*   The home data specifies the servo setpoint value required to position the robot in the pre-defined home position with 
*   joint angles 0, 90, -90, -90, 0, respectively, and gripper fully open.
*   These setpoint values are specified in microseconds: 
*   typically 500 microseconds corresponds to the servo-motor positioned at one extreme and 2500 microseconds to the other extreme.
*
*   The degree data specifies the pulse width per degree for each servo. In theory, these should be identical for all servos 
*   but in practice they are not and the value specified are determined using a calibration exercise. 
*
*   The wrist data specifies whether the wrist is a lightweight wrist or a heavy duty wrist. The key values are "lightweight" or "heavyduty"
*   This information is used to determine which direction the roll servo should turn (the heavy duty wrist reverses the direction)
*   
*   The default data specifies the initial joint angles in radians and the gripper distance in metres
*   This information is updated every time either setJointAngles() or grasp() is called 
*   and is used in constructing the message to be published on the ROS topic when using ROS to control the robot or the simulator
*
*   This code can also be compiled for ROS in which case a simulator for the robot can be controlled by publishing the joint angles on the
*   the /lynxmotion_al5d/joints_positions/command topic.  The ROS code is conditionally compiles by defining flag in the interface file. 
*
*   David Vernon, Carnegie Mellon University Africa
*   25 April 2018
*
*   Audit Trail
*   -----------
*   6  June 2018: added configuration file functionality
*   28 June 2020: re-factored code to separate calculation of the joint anglesusing the inverse kinematics,  
*                 from the calculation of servomotor setpoint values.  
*                 This was done to allow the simulator to be controlled by publishing joint angles on the 
*                 ROS /lynxmotion_al5d/joints_positions/command topic 
*
*******************************************************************************************************************/

#include "robotProgramming.h"

int main(int argc, char ** argv) {

   extern robotConfigurationDataType robotConfigurationData;

   bool debug = true;

   FILE *fp_in;                        // robotProgramming input file
   int end_of_file; 
   char filename[MAX_FILENAME_LENGTH];

   /******************************************************************************

   Robot programming declarations

   *******************************************************************************/

   /* initial position for demo */
   
   float effector_length;                     // initialized from robot configuration file

   float object_x        = 0;                 // object position x
   float object_y        = 187.0;             //                 y
   float object_z        = 0;                 //                 z
   float object_theta    = -90;               //                 theta 
                                              //                 (theta is object orientation in degrees 
                                              //                  w.r.t. horizontal
                                              //                  anticlockwise for a positive angle)

   float example_x       =  object_x;         // example pose x  
   float example_y       =  object_y;         // example pose y  
   float example_z       =  (float) 216.0;    // example pose z

   float value           = 0;                 // variable for demo
   float side_x          = 100;               // x coordinate to the right of the centre

   float tray_x          = 150;               // tray position x 
   float tray_y          = 100;               // tray position y 
   float tray_z          = 100;               // tray position z 

   float initial_approach_distance;
   float approach_distance;
   float delta;

   Frame E;
   Frame Z;
   Frame T5;
   Frame object_grasp;
   Frame object_approach;
   Frame example_pose;
   Frame tray_pose;
 


   /*******************************************************************************

   Get the name of the file containing the robot configuration and calibration data 
   from the input file robotProgrammingInput.txt

   ********************************************************************************/

   if ((fp_in = fopen("../data/robotProgrammingInput.txt","r")) == 0) {
	   printf("Error can't open input robotProgrammingInput.txt\n");
      prompt_and_exit(0);
   }

   end_of_file = fscanf(fp_in, "%s", filename); // read the configuration filename
      
   if (end_of_file != EOF) {  // only proceed if there is a configuration file

      readRobotConfigurationData(filename);

	  	 
      goHome();    // not strictly necessary ... just for demonstration
      wait(2000);  // wait for 5 seconds
      
      effector_length = (float) robotConfigurationData.effector_z;

      delta = 5;                                 // increment in approach distance 

      E = trans(0.0, 0.0, effector_length);      // end-effector (gripper) frame
      Z = trans(0.0 ,0.0, 0.0);                  // robot base frame

	  initial_approach_distance = 100;           // initial distance from the grasp position

      /*****************************************************************************************/
      /* example pose: just a translation so that the gripper frame is aligned with base frame */
	  /* with the y axis (direction of gripper movement) pointing away from the robot          */
	  /*                                                                                       */
      /* Add the effector_length to z position because this is where we want the tip of the    */
	  /* gripper to be                                                                         */
      /*****************************************************************************************/

	  if (debug) printf("\nalign gripper with base frame\n");

      example_pose = trans(example_x, example_y, example_z+effector_length); 

      T5 = inv(Z) * example_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

      wait(5000);  // wait for 5 seconds
                                                                    
	  /*****************************************************************************************/
      /* example pose ... rotate the wrist 90 degrees about z to have the gripper y axis       */
	  /* (i.e. direction of gripper movement) pointing to the left                             */
	  /*                                                                                       */
      /* Add the effector_length to z position because this is where we want the tip of the    */
	  /* gripper to be                                                                         */
      /*****************************************************************************************/

	  if (debug) printf("\nalign gripper with base frame; rotate wrist 90 degrees\n");

      example_pose = trans(example_x, example_y, example_z+effector_length) *  rotz(90.0);

      T5 = inv(Z) * example_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

      wait(5000);  // wait for 5 seconds


	                                                                      
	  /*****************************************************************************************/
      /* example pose ... rotate the wrist 90 degrees about z and then  90 degrees about y     */
	  /* to achieve the same pose as the home configuration, i.e. gripper pointing in the y    */
	  /* direction and the direction of gripper movement aligned with the base x axis          */
	  /*                                                                                       */
      /* Add the effector_length to z position because this is where we want the tip of the    */
	  /* gripper to be                                                                         */
      /*****************************************************************************************/
 
	  if (debug) printf("\nhome pose\n");

      example_pose = trans(example_x, example_y+effector_length, example_z) *  rotz(90.0) * roty(90); 

      T5 = inv(Z) * example_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

      wait(5000);  // wait for 5 seconds
       

	  /*****************************************************************************************/
      /* same pose as above ... but different combination of rotations to achieve it           */
      /*****************************************************************************************/
 
	  if (debug) printf("\nhome pose, version 2\n");

      example_pose = trans(example_x, example_y+effector_length, example_z) *  roty(90.0) * rotx(-90); 

      T5 = inv(Z) * example_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

      wait(5000);  // wait for 5 seconds
       
	  /*****************************************************************************************/
      /* same wrist orientation as above but with the wrist now 20mm above to the work surface */
      /*****************************************************************************************/
 	  
	  if (debug) printf("\nhome pose 20 mm above the worksurface\n");

      example_pose = trans(example_x, example_y+effector_length, 20) *  rotz(90.0) * roty(90); 

      T5 = inv(Z) * example_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

      wait(5000);  // wait for 5 seconds

#ifdef NOCOMPILE

	  /*****************************************************************************************/
	  /*                                                                                       */
      /* now show how an object can be grasped from above using the technique in the notes     */
	  /*                                                                                       */
	  /*****************************************************************************************/

      object_grasp = trans(object_x, object_y, object_z) * roty(180.0) * rotz(object_theta);     // object grasp frame
      //object_grasp.printFrame();  // useful for validation

      /* approach distance */
   
      approach_distance = initial_approach_distance;
      object_approach = trans(0,0,-approach_distance); // frame defined wrt object_grasp frame


      /* move to initial approach pose */

      T5 = inv(Z) * object_grasp * object_approach * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

      wait(5000);  // wait for 5 seconds


	  grasp(GRIPPER_OPEN); // open the gripper fully before attempting to grasp an object
      wait(1000); 


      /* move vertically in the -Z direction (down), maintaining wrist pose */

      do {
         T5 = inv(Z) * object_grasp * object_approach * inv(E);

         if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

         approach_distance = approach_distance - delta;
         object_approach = trans(0,0,-approach_distance);
      } while (approach_distance > 0);

      wait(3000);    

	  
	  grasp(15);  // 15 mm opening in gripper ... this is the width of a Lego block and could be used to grasp the block 
	  wait(1000); // we definitely do not want to use grasp(GRIPPER_CLOSED) because this closes the gripper completely                   
	              // and it can't reach this position if there is a block preventing it from closing fully
	              // this will damage the motors and possible the controller board also
                 


      /* move vertically in the Z direction (up), maintaining wrist pose */

      approach_distance = 0;
      object_approach = trans(0,0,-approach_distance);
      do {
         T5 = inv(Z) * object_grasp * object_approach * inv(E);

         if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

         approach_distance = approach_distance + delta;
         object_approach = trans(0,0,-approach_distance);
      } while (approach_distance <= initial_approach_distance);

      wait(3000); 

	 
	  /* move horizontally in the X direction (right), maintaining wrist pose */

      for (value = example_x; value <= example_x + side_x; value = value + delta) {
         example_pose = trans(value, example_y, example_z-effector_length) * roty(180.0) * rotz(-90.0); 

         T5 = inv(Z) * example_pose * inv(E);

         if (move(T5) == false) display_error_and_exit("move error ... quitting\n");;
      } 

      wait(3000);  


	  /* move above the tray */
	  /* ------------------- */

	  tray_pose = trans(tray_x, tray_y, tray_z) * roty(180.0) * rotz(-90.0); 

      T5 = inv(Z) * tray_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

      wait(3000);  // wait for 5 seconds
	   
      grasp(GRIPPER_OPEN);   // release anything that's been grasped
      wait(1000); 


      /* move horizontally in the -X direction (left), maintaining wrist pose */

      for (value = example_x + side_x; value >= example_x; value = value - delta) {
         example_pose = trans(value, example_y, example_z-effector_length) * roty(180.0) * rotz(-90.0); 

         T5 = inv(Z) * example_pose * inv(E);

         if (move(T5) == false) display_error_and_exit("move error ... quitting\n");;
      }       

      wait(3000);
#endif
 
	  /**************************************************/
      /* Alternative without the point-to-point control */
      /**************************************************/

//#ifdef NOCOMPILE

	  object_grasp = trans(object_x, object_y, object_z) * roty(180.0) * rotz(object_theta);     // object grasp frame
 
      approach_distance = initial_approach_distance;
      object_approach = trans(0,0,-approach_distance); // frame defined wrt object_grasp frame


      /* move to initial approach pose */

	  if (debug) printf("\ninitial approach pose\n");

      T5 = inv(Z) * object_grasp * object_approach * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

      wait(3000);  // wait for 5 seconds
   
	  grasp(GRIPPER_OPEN); // open the gripper fully before attempting to grasp an object
      wait(1000); 

      /* move to the grasp pose */
       
	  if (debug) printf("\ngrasp pose\n");

      T5 = inv(Z) * object_grasp * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");;
 
      wait(3000);    
 
	  grasp(15);  // 15 mm opening in gripper ... this is the width of a Lego block and could be used to grasp the block 
	  wait(1000); // we definitely do not want to use grasp(GRIPPER_CLOSED) because this closes the gripper completely                   
	              // and it can't reach this position if there is a block preventing it from closing fully
	              // this will damage the motors and possible the controller board also
                 

      /* move to initial approach pose */

	  if (debug) printf("\napproach approach pose\n");

      T5 = inv(Z) * object_grasp * object_approach * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");;

      wait(3000);  // wait for 5 seconds
   

      /* move to example pose */

	  if (debug) printf("\nexample pose\n");

      example_pose = trans(example_x, example_y, example_z-effector_length) * roty(180.0) * rotz(-90.0); 

      T5 = inv(Z) * example_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");;

      wait(3000);


      /* move horizontally in the X direction (right) */

	  if (debug) printf("\nhorizontally right pose\n");

      example_pose = trans(example_x + side_x, example_y, example_z-effector_length) * roty(180.0) * rotz(-90.0); 

      T5 = inv(Z) * example_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");;

      wait(3000);  


	  /* move above the tray */
	  /* ------------------- */

	  if (debug) printf("\nabove the tray pose\n");

	  tray_pose = trans(tray_x, tray_y, tray_z) * roty(180.0) * rotz(-90.0); 

      T5 = inv(Z) * tray_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");

      wait(3000);  // wait for 3 seconds
	   
      grasp(GRIPPER_OPEN);   // release anything that's been grasped
      wait(1000); 
    

      /* move back to example pose */

	 if (debug) printf("\nexample pose\n");

      example_pose = trans(example_x, example_y, example_z-effector_length) * roty(180.0) * rotz(-90.0); 

      T5 = inv(Z) * example_pose * inv(E);

      if (move(T5) == false) display_error_and_exit("move error ... quitting\n");;

      wait(3000);

      goHome(); // this returns the robot to the home position so that when it's switched off 
                // it's in a pose that is close to the one that the servo-controller uses as its initial state
                // could also do this with a move() as show above

//#endif
      prompt_and_exit(0);

   }
   return 0;
}