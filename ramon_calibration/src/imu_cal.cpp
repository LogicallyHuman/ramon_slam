#include <iostream>
#include <fstream>
#include <ros/ros.h>
#include <sensor_msgs/Imu.h>
#include <rosbag/bag.h>
#include <rosbag/view.h>

#include <Eigen/Eigen>
#include <unsupported/Eigen/NonLinearOptimization>

#include "gyro_cal.h"
#include "accel_cal.h"

#include <boost/foreach.hpp>
#define foreach BOOST_FOREACH

int main (int argc, char** argv)
{
    ros::init (argc, argv, "imu_calibration");    
    rosbag::Bag bag;
	Eigen::MatrixXf accel_values, accel_values_corr, gyro_values, gyro_values_corr;
    Eigen::VectorXf output_accel_values(9), output_gyro_values(12);
    Eigen::Matrix3f T_a, K_a, T_g, K_g;
    Eigen::Vector3f b_a, b_g;
    std::ofstream file("test.txt");

    bag.open("/home/fdominguez/Documents/bagfiles/imu_data3.bag", rosbag::bagmode::Read);

    std::vector<std::string> topics;
    topics.push_back(std::string("/imu_data"));
    rosbag::View view(bag, rosbag::TopicQuery(topics));

    ros::Time::init();
    int j = 0;
    
    accel_values.resize(bag.getSize(),3);
    gyro_values.resize(bag.getSize(),3);
    foreach(rosbag::MessageInstance const mm, view)
    {
        sensor_msgs::Imu::ConstPtr i = mm.instantiate<sensor_msgs::Imu>();
        if (i != NULL)
        {
            accel_values(j, 0) = i->linear_acceleration.x;
            accel_values(j, 1) = i->linear_acceleration.y;
            accel_values(j, 2) = i->linear_acceleration.z;
            gyro_values(j,0) = i->angular_velocity.x;
            gyro_values(j,1) = i->angular_velocity.y;
            gyro_values(j,2) = i->angular_velocity.z;
            j++;
        }
    }
    accel_values.conservativeResize(j, 3);
    accel_values_corr.resize(j, 3);
    gyro_values.conservativeResize(j, 3);
    gyro_values_corr.resize(j, 3);

    accel_values = accel_values / -9.80665;    
    accel_calibration(accel_values, output_accel_values);

    T_a << 1, -output_accel_values(0),  output_accel_values(1),
           0,           1            , -output_accel_values(2),
           0,           0            ,            1           ;  

    K_a << output_accel_values(3),          0            ,          0             ,
                    0            , output_accel_values(4),          0             ,
                    0            ,          0            ,  output_accel_values(5);
    
    b_a << output_accel_values(6),
           output_accel_values(7),
           output_accel_values(8);         

    for(int i=0; i<j; i++)
    {
        accel_values_corr.row(i) = (T_a * K_a * (accel_values.row(i).transpose() + b_a)).transpose();
    }

    gyro_calibration(accel_values_corr, gyro_values, output_gyro_values);

    T_g <<   1  , -output_gyro_values(0),  output_gyro_values(1),
            output_gyro_values(3),   1  , -output_gyro_values(2),
            -output_gyro_values(4),  output_gyro_values(5),   1  ;  

    K_g << output_gyro_values(6),  0  ,   0  ,
            0  , output_gyro_values(7),   0  ,
            0  ,  0  ,  output_gyro_values(8);
    
    b_g << output_gyro_values(9),
            output_gyro_values(10),
            output_gyro_values(11);

    for(int i=0; i<j; i++)
    {
        gyro_values_corr.row(i) = (T_g * K_g * (gyro_values.row(i).transpose() + b_g)).transpose();
    }

    // std::cout << "Accel data:" << std::endl;
    // std::cout <<  accel_values << std::endl;
    // std::cout << "Accel data corr:" << std::endl;
    // std::cout <<  accel_values_corr << std::endl;

    // std::cout << "Gyro data:" << std::endl;
    // std::cout <<  gyro_values << std::endl;
    // std::cout << "Gyro data corr:" << std::endl;
    // std::cout <<  gyro_values_corr << std::endl;

    std::cout << "Accel data values" << std::endl;
    std::cout <<  output_accel_values << std::endl;
    std::cout << "Gyro data values" << std::endl;
    std::cout <<  output_gyro_values << std::endl;

    return 0;
}