#include <sstream>
#include <stdlib.h>
#include "ros/ros.h"
#include "std_msgs/Header.h"
#include "geometry_msgs/Point.h"
#include "geometry_msgs/Pose.h"
#include "geometry_msgs/Twist.h"
#include "geometry_msgs/Vector3.h"
#include "geometry_msgs/TransformStamped.h"
#include "geometry_msgs/Transform.h"
#include "nav_msgs/Path.h"
#include <string>
#include <array>
#include <cmath>
using namespace std;


struct Quaternion {
        float w, x, y, z;
};

struct EulerAngles {
        float roll, pitch, yaw;
};

EulerAngles ToEulerAngles(Quaternion q);


//Constants and general stuff
//These current positions and orientations could be vectors!!!!!!
//I don't really know the contents, or what rtabmap pushes out, but we'll have to discuss about it
//still need to fix my current position, i just gotta find it!

void path_callback(const nav_msgs::Path::ConstPtr& received_path);
bool proximity_check(geometry_msgs::Point goal, geometry_msgs::Point current);
void tf_callback(const tf::tfMessage::ConstPtr& robot_position);

geometry_msgs::Point current_position;
geometry_msgs::Quaternion current_orientation; //maybe don't need

geometry_msgs::Point goal_position;
geometry_msgs::Quaternion goal_orientation; //maybe don't need

geometry_msgs::Twist velocity_to_publish;

string desired_child = "/odom";
string desired_frame = "/map";

int main(int argc, char** argv){

        ros::init(argc, argv, "path_follower");

        //initializing my node
        ros::NodeHandle node;

        ros::Rate loop_rate(1000);

        ros::Publisher twist_pub = node.advertise<geometry_msgs::Twist>("path/cmd_vel", 1000); //publisher for the veolocity forwarder/the robot
        ros::Subscriber nav_sub = node.subscribe("local_path", 1000, &path_callback); //subscriber for the velocity from the path planner
        ros::Subscriber tf_sub = node.subscribe("tf", 1000, &tf_callback);

        //initializing speeds at 0
        velocity_to_publish.linear.x = 0.0;
        velocity_to_publish.linear.y = 0.0;
        velocity_to_publish.linear.z = 0.0;
        velocity_to_publish.angular.x = 0.0;
        velocity_to_publish.angular.y = 0.0;
        velocity_to_publish.angular.z = 0.0;

        float inc_x, inc_y, current_roll, current_pitch, current_yaw, angle_to_goal;
        struct EulerAngles robot_angle;

        //Need a function to obtain my yaw from my current orientation! Maybe dont need current Quaternion! Depends on how I can compute the orientation of my robot.

        while(ros::ok()) {

                robot_angle = ToEulerAngles(current_orientation);
                current_roll = robot_angle.roll;
                current_pitch = robot_angle.pitch;
                current_yaw =  robot_angle.yaw;
                //determine proper orientation of my goal = received path

                inc_x = goal_position.x - current_position.x;
                inc_y = goal_position.y - current_position.y;
                angle_to_goal = atan2 (inc_y, inc_x);

                if( !proximity_check(goal_position, current_position)) {
                        if((angle_to_goal - current_yaw) > 0.01 ) {
                                // RIGHT ROTATION
                                velocity_to_publish.linear.x = 0.0;
                                velocity_to_publish.angular.z = 0.3;

                                //actually publish velocity
                                twist_pub.publish(velocity_to_publish);
                        } else if ((angle_to_goal - current_yaw) < 0.1 ) {
                                //LFET ROTATION
                                velocity_to_publish.linear.x = 0.0;
                                velocity_to_publish.angular.z = -0.3;

                                //actually publish velocity
                                twist_pub.publish(velocity_to_publish);
                        } else
                        { //GO STRAIGHT
                                velocity_to_publish.linear.x = 1;
                                velocity_to_publish.angular.z = 0.0;

                                //actually publish velocity
                                twist_pub.publish(velocity_to_publish);
                        }
                }


                ros::spinOnce();
                loop_rate.sleep();
        }

        return 0;
};

void path_callback(const nav_msgs::Path::ConstPtr& received_path){

        goal_position = received_path[0]->Pose.Point;
        goal_orientation = received_path[0]->Pose.Quaternion;

}

void tf_callback(const tf::tfMessage::ConstPtr& robot_position){

        for(std::vector<int>::const_iterator it = robot_position->TransformStamped.begin(); it != robot_position->TransformStamped.end(); ++it)
        {
                if(strcmp(desired_frame, it->Header.frame_id)   && strcmp(desired_child, it->child_frame_id) ) {

                        current_position = *it->Transform.Vector3;
                        current_orientation = *it->Transform.Quaternion;

                }

        }

}

bool proximity_check(geometry_msgs::Point goal, geometry_msgs::Point current){
        //checking if the robot is close/has almost reached to my goal
        if( ( (goal.x - current.x) <0.1 ) &&  ( (goal.x - current.x) <0.1 ) ) {
                return true;
        } else {
                return false;
        }
}


EulerAngles ToEulerAngles(Quaternion q){
        EulerAngles angles;

        // roll (x-axis rotation)
        float sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
        float cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
        angles.roll = std::atan2(sinr_cosp, cosr_cosp);

        // pitch (y-axis rotation)
        float sinp = 2 * (q.w * q.y - q.z * q.x);
        if (std::abs(sinp) >= 1)
                angles.pitch = std::copysign(M_PI / 2, sinp);  // use 90 degrees if out of range
        else
                angles.pitch = std::asin(sinp);

        // yaw (z-axis rotation)
        float siny_cosp = 2 * (q.w * q.z + q.x * q.y);
        float cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
        angles.yaw = std::atan2(siny_cosp, cosy_cosp);

        return angles;
}
