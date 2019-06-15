#include <stdio.h>
#include "ros/ros.h"
#include <ros/callback_queue.h>
#include "edg_fingers_control/SetPosition.h"

using namespace std;

//The node handle needs to be global so its accessible in the callback
//Needs to be instantiated in the main once ros::init is done
ros::NodeHandle *thisNode;

//Pointer to the opened file in which we write in order to transmit serial data.
FILE* file_pointer;

//This sends the requested positions through the serial communication to
//the Arduino. It is impossible at the moment to make sure it is correctly
//received.
bool transmitCommand(uint16_t requested_motor1_position, uint16_t requested_motor2_position){
    if(requested_motor1_position > 9999 || requested_motor1_position < 0){
      ROS_ERROR("The requested motor 1 position is not within bounds.");
      return false;
    }

    if(requested_motor2_position > 9999 || requested_motor2_position < 0){
      ROS_ERROR("The requested motor 2 position is not within bounds.");
      return false;
    }

    //The Arduino expect each integer to be made of 4 characters, this format
    //ensures that the width of the transmitted data respects this.
    int result = 0;
    if(file_pointer)
      result = fprintf(file_pointer, "1:%.4d 2:%.4d\n", requested_motor1_position, requested_motor2_position);

    //Result should contain the number of bytes successfully written (14).
    if( result > 0){
        ROS_INFO("Successfully transmitted command.");
        return true;
    }else{
        ROS_ERROR("Unable to transmit command.");
        return false;
    }
}

//This function gets called whenever a node request our service
bool setPosition(   edg_fingers_control::SetPosition::Request  &req,
                    edg_fingers_control::SetPosition::Response &res)
{
    //Retrieve the values submitted by the user
    uint16_t requested_motor1_position = req.Motor1Position;
    uint16_t requested_motor2_position = req.Motor2Position;

    ROS_INFO("Setting Motor 1 goal position to: %d",requested_motor1_position);
    ROS_INFO("Setting Motor 2 goal position to: %d",requested_motor2_position);

    //Try to transmit these values to the serial connection
    bool result = transmitCommand(requested_motor1_position, requested_motor2_position);

    //This is the result that the user will see.
    res.Success = result;
    return result;
}

int main(int argc, char **argv){
    //The first argument of the service must represent the fil descriptor of the
    // TTY being used. Most of the time, it is /dev/ttyACM0 but it can be mapped
    // to /dev/ttyACM1, for example, if the RObotiq 2f-140 gripper is plugged in
    // and is already using that file path.
    if(argc != 2){
      ROS_ERROR("The TTY path must be given as the sole argument.");
      return 1;
    }
    char* serialTtyPath = argv[1];
    file_pointer = fopen(serialTtyPath,"w");

    //Initialize the ROS node
    ros::init(argc, argv, "edg_fingers_control_server");

    //Instantiate the handle, needs to be done after ros::init
    thisNode = new ros::NodeHandle();

    //Tell everyone that this service exists. Upon using this service, the callback function
    //named setPosition will be called.
    ros::ServiceServer service = thisNode->advertiseService("fingers/set_position", setPosition);
    ROS_INFO("Ready to control the EDG Fingers.");

    //Default callback queue
    ros::CallbackQueue* queue = ros::getGlobalCallbackQueue();

    //If ROS is being closed by the user, we want to stop this process.
    while(ros::ok()){

        //callAvailable() can take in an optional timeout, which is the amount
        //of time it will wait for a callback to become available before
        //returning. If this is zero and there are no callbacks in the queue
        //the method will return immediately.
        queue->callAvailable(ros::WallDuration(0));
    }

    //Close the file before exiting the program.
    if(file_pointer)
      fclose(file_pointer);

    return 0;
}
