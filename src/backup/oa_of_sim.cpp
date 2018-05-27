#include "ros/ros.h"
#include "oa_of/MsgOAOF.h"

#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <geometry_msgs/PoseStamped.h>
#include <sensor_msgs/Image.h>
#include <opencv2/video/tracking.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/videoio/videoio.hpp>

#include <iostream>
#include <ctype.h>
#include <math.h>

#define S_TIME      0.1

#define ALPHA       0.00003

#define O_WIDTH     160
#define O_HEIGHT    120

#define	WIDTH	    80
#define HEIGHT	    60

#define WIDTH_H     80
#define HEIGHT_H    12

#define WIDTH_V     16
#define HEIGHT_V    60

#define HEIGHT_H_O  24
#define WIDTH_V_O   32

#define D_SET       0.1

#define P_RL_GAIN   -0.001
#define P_UD_GAIN   -0.003

using namespace cv;
using namespace std;

// ---------------------- //
// -- Grobal Variables -- //
// ---------------------- //

unsigned char Img_data[O_WIDTH*O_HEIGHT];

double pose_p_x_c = 0;
double pose_p_y_c = 0;
double pose_p_z_c = 0;
double pose_o_qx_c = 0;
double pose_o_qy_c = 0;
double pose_o_qz_c = 0;
double pose_o_qw_c = 1;
double pose_o_ex_c = 0;
double pose_o_ey_c = 0;
double pose_o_ez_c = 0;

double pose_p_x_t = 0;
double pose_p_y_t = 4.5;
double pose_p_z_t = 1.5;
double pose_o_qx_t = 0;
double pose_o_qy_t = 0;
double pose_o_qz_t = 0;
double pose_o_qw_t = 1;
double pose_o_ex_t = 0;
double pose_o_ey_t = 0;
double pose_o_ez_t = 0;

// ------------------------ //
// -- Callback Functions -- //
// ------------------------ //

void msgCallback(const geometry_msgs::Pose::ConstPtr& Pose){
	pose_p_x_c = Pose->position.x;
	pose_p_y_c = Pose->position.y;
	pose_p_z_c = Pose->position.z;
	pose_o_qx_c = Pose->orientation.x;
	pose_o_qy_c = Pose->orientation.y;
	pose_o_qz_c = Pose->orientation.z;
	pose_o_qw_c = Pose->orientation.w;

	double siny = +2.0 * (pose_o_qw_c * pose_o_qz_c + pose_o_qx_c * pose_o_qy_c);
	double cosy = +1.0 - 2.0 * (pose_o_qy_c * pose_o_qy_c + pose_o_qz_c * pose_o_qz_c);
	pose_o_ez_c = atan2(siny, cosy);
}

void msgCallback_img(const sensor_msgs::Image::ConstPtr& Img){
	for(int i = 0; i<O_WIDTH*O_HEIGHT; i++){
        Img_data[i] = Img->data[i];
	}
}

// ------------------- //
// -- Main Function -- //
// ------------------- //

int main (int argc, char **argv){
	ros::init(argc, argv, "oa_of_sim");
	ros::NodeHandle nh, nh_mavros, nh_image;

	ros::Publisher oa_of_pub = nh.advertise<oa_of::MsgOAOF>("oa_of_msg",100);

    ros::Publisher Set_Position_pub = nh_mavros.advertise<geometry_msgs::PoseStamped>("firefly/command/pose",100);
    ros::Subscriber oa_of_sub_pos = nh_mavros.subscribe("firefly/odometry_sensor1/pose", 10, msgCallback);
    ros::Subscriber oa_of_sub_image = nh_image.subscribe("firefly/camera_nadir/image_raw", 10, msgCallback_img);

    ros::Rate loop_rate(1/S_TIME);

    Mat mat_arrow_h(Size(WIDTH_H*100, HEIGHT_H*100),CV_8UC1,255);
    Mat mat_arrow_v(Size(WIDTH_V*100, HEIGHT_V*100),CV_8UC1,255);
    Mat mat_array;
    Mat mat_total;

	char keypressed;

    uchar arr_gray_prev[WIDTH][HEIGHT];
	uchar arr_gray_curr[WIDTH][HEIGHT];

	uchar arr_gray_prev_h[WIDTH_H][HEIGHT_H];
	uchar arr_gray_curr_h[WIDTH_H][HEIGHT_H];

	uchar arr_gray_prev_v[WIDTH_V][HEIGHT_V];
	uchar arr_gray_curr_v[WIDTH_V][HEIGHT_V];

	double Ix_h[WIDTH_H-1][HEIGHT_H-1];
	double Iy_h[WIDTH_H-1][HEIGHT_H-1];
	double It_h[WIDTH_H-1][HEIGHT_H-1];

	double u_h[WIDTH_H][HEIGHT_H];
	double v_h[WIDTH_H][HEIGHT_H];

	double mu_u_h[WIDTH_H-2][HEIGHT_H-2];
	double mu_v_h[WIDTH_H-2][HEIGHT_H-2];

    CvPoint p1_h[WIDTH_H][HEIGHT_H];
    CvPoint p2_h[WIDTH_H][HEIGHT_H];

    double r_x_h[WIDTH_H][HEIGHT_H];
    double r_y_h[WIDTH_H][HEIGHT_H];
    double r_h[WIDTH_H][HEIGHT_H];

    double eta_h[WIDTH_H][HEIGHT_H];
    double eta_h_sum = 0;

    double Ix_v[WIDTH_V-1][HEIGHT_V-1];
	double Iy_v[WIDTH_H-1][HEIGHT_V-1];
	double It_v[WIDTH_V-1][HEIGHT_V-1];

	double u_v[WIDTH_V][HEIGHT_V];
	double v_v[WIDTH_V][HEIGHT_V];

	double mu_u_v[WIDTH_V-1][HEIGHT_V-1];
	double mu_v_v[WIDTH_V-1][HEIGHT_V-1];

    CvPoint p1_v[WIDTH_V][HEIGHT_V];
    CvPoint p2_v[WIDTH_V][HEIGHT_V];

	double OFright = 0;
	double OFleft = 0;
	double OFup = 0;
	double OFdown = 0;

	double D_OF_RL = 0;
	double D_OF_UD = 0;

	double cy = 0;
    double sy = 0;
    double cr = 0;
    double sr = 0;
    double cp = 0;
    double sp = 0;

    for(int i=0; i<(WIDTH_H-1); i++){
		for(int j=0; j<(HEIGHT_H-1); j++){
            p1_h[i][j] = cvPoint(i,j);
		}
    }

    for(int i=0; i<(WIDTH_V-1); i++){
		for(int j=0; j<(HEIGHT_V-1); j++){
            p1_v[i][j] = cvPoint(i,j);
		}
    }

    for(int i=0; i<(WIDTH_H-1); i++){
	    for(int j=0; j<(HEIGHT_H-1); j++){
            p1_h[i][j].x = (p1_h[i][j].x*100)+100;
            p1_h[i][j].y = (p1_h[i][j].y*100)+100;
        }
    }

    for(int i=0; i<(WIDTH_V-1); i++){
	    for(int j=0; j<(HEIGHT_V-1); j++){
            p1_v[i][j].x = (p1_v[i][j].x*100)+100;
            p1_v[i][j].y = (p1_v[i][j].y*100)+100;
        }
    }

    for(int i=0; i<(WIDTH_H); i++){
	    for(int j=0; j<(HEIGHT_H); j++){
            r_x_h[i][j] = i-WIDTH_H/2+0.5;
            r_y_h[i][j] = j-HEIGHT_H/2+0.5;
            r_h[i][j] = sqrt(r_x_h[i][j]*r_x_h[i][j] + r_y_h[i][j]*r_y_h[i][j]);
        }
    }

    double count = 0;
	while (ros::ok()){
        mat_arrow_h.setTo(255);
        mat_arrow_v.setTo(255);
        mat_array = Mat(O_HEIGHT, O_WIDTH, CV_8UC1, &Img_data);

		resize(mat_array, mat_array, Size(WIDTH, HEIGHT));

        // ------------------------- //
		// -- Save Previous Image -- //
		// ------------------------- //

		for(int i=0; i < (WIDTH_H); i++){
			for(int j=0; j < (HEIGHT_H); j++){
				arr_gray_prev_h[i][j] = arr_gray_curr_h[i][j];
			}
		}

		for(int i=0; i < (WIDTH_V); i++){
			for(int j=0; j < (HEIGHT_V); j++){
				arr_gray_prev_v[i][j] = arr_gray_curr_v[i][j];
			}
		}

		// -------------------- //
		// -- Save New Image -- //
		// -------------------- //

		for(int i=0; i < (WIDTH); i++){
			for(int j=0; j < (HEIGHT); j++){
        			arr_gray_curr[i][j] = mat_array.at<uchar>(j,i);
			}
		}

        // ------------------- //
		// -- Image Cutting -- //
		// ------------------- //

		for(int i=0; i<WIDTH_H; i++){
            for(int j=0; j<HEIGHT_H; j++){
                arr_gray_curr_h[i][j] = arr_gray_curr[i][HEIGHT_H_O+j];
            }
        }

        for(int i=0; i<WIDTH_V; i++){
            for(int j=0; j<HEIGHT_V; j++){
                arr_gray_curr_v[i][j] = arr_gray_curr[WIDTH_V_O+i][j];
            }
        }

		// ----------------------------------------- //
		// -- Horizental Optical Flow Calculation -- //
        // ----------------------------------------- //

		for(int i=0; i<(WIDTH_H-1); i++){
			for(int j=0; j<(HEIGHT_H-1); j++){
				Ix_h[i][j] = (arr_gray_prev_h[i+1][j] - arr_gray_prev_h[i][j] + arr_gray_prev_h[i+1][j+1] - arr_gray_prev_h[i][j+1] + arr_gray_curr_h[i+1][j] - arr_gray_curr_h[i][j] + arr_gray_curr_h[i+1][j+1] - arr_gray_curr_h[i][j+1])/4;
				Iy_h[i][j] = (arr_gray_prev_h[i][j+1] - arr_gray_prev_h[i][j] + arr_gray_prev_h[i+1][j+1] - arr_gray_prev_h[i+1][j] + arr_gray_curr_h[i][j+1] - arr_gray_curr_h[i][j] + arr_gray_curr_h[i+1][j+1] - arr_gray_curr_h[i+1][j])/4;
				It_h[i][j] = (arr_gray_curr_h[i][j] - arr_gray_prev_h[i][j] + arr_gray_curr_h[i+1][j] - arr_gray_prev_h[i+1][j] + arr_gray_curr_h[i][j+1] - arr_gray_prev_h[i][j+1] + arr_gray_curr_h[i+1][j+1] - arr_gray_prev_h[i+1][j+1])/4;
			}
		}

		for(int i=0; i<(WIDTH_H-2); i++){
		    for(int j=0; j<(HEIGHT_H-2); j++){
                mu_u_h[i][j] = (u_h[i][j+1] + u_h[i+1][j] + u_h[i+2][j+1] + u_h[i+1][j+2])/6 + (u_h[i][j] + u_h[i][j+2] + u_h[i+2][j] + u_h[i+2][j+2])/12;
                mu_v_h[i][j] = (v_h[i][j+1] + v_h[i+1][j] + v_h[i+2][j+1] + v_h[i+1][j+2])/6 + (v_h[i][j] + v_h[i][j+2] + v_h[i+2][j] + v_h[i+2][j+2])/12;
		    }
		}

		for(int i=0; i<(WIDTH_H-2); i++){
		    for(int j=0; j<(HEIGHT_H-2); j++){
                u_h[i+1][j+1] = mu_u_h[i][j] - Ix_h[i][j]*((Ix_h[i][j]*mu_u_h[i][j] + Iy_h[i][j]*mu_v_h[i][j] + It_h[i][j])/(ALPHA*ALPHA + Ix_h[i][j]*Ix_h[i][j] + Iy_h[i][j]*Iy_h[i][j]));
                v_h[i+1][j+1] = mu_v_h[i][j] - Iy_h[i][j]*((Ix_h[i][j]*mu_u_h[i][j] + Iy_h[i][j]*mu_v_h[i][j] + It_h[i][j])/(ALPHA*ALPHA + Ix_h[i][j]*Ix_h[i][j] + Iy_h[i][j]*Iy_h[i][j]));
		    }
		}

		// -------------------------------- //
		// -- Horizental Eta Calcuration -- //
		// -------------------------------- //

		for(int i=0; i<(WIDTH_H); i++){
		    for(int j=0; j<(HEIGHT_H); j++){
                eta_h[i][j] = (r_x_h[i][j]*u_h[i][j] + r_y_h[i][j]*v_h[i][j])/((r_h[i][j])*(r_h[i][j]));
		    }
		}

		eta_h_sum = 0;
		for(int i=0; i<(WIDTH_H); i++){
            for(int j=0; j<(HEIGHT_H); j++){
                eta_h_sum += eta_h[i][j];
		    }
		}

		// --------------------------------------- //
		// -- Vertical Optical Flow Calculation -- //
        // --------------------------------------- //

		for(int i=0; i<(WIDTH_V-1); i++){
			for(int j=0; j<(HEIGHT_V-1); j++){
				Ix_v[i][j] = (arr_gray_prev_v[i+1][j] - arr_gray_prev_v[i][j] + arr_gray_prev_v[i+1][j+1] - arr_gray_prev_v[i][j+1] + arr_gray_curr_v[i+1][j] - arr_gray_curr_v[i][j] + arr_gray_curr_v[i+1][j+1] - arr_gray_curr_v[i][j+1])/4;
				Iy_v[i][j] = (arr_gray_prev_v[i][j+1] - arr_gray_prev_v[i][j] + arr_gray_prev_v[i+1][j+1] - arr_gray_prev_v[i+1][j] + arr_gray_curr_v[i][j+1] - arr_gray_curr_v[i][j] + arr_gray_curr_v[i+1][j+1] - arr_gray_curr_v[i+1][j])/4;
				It_v[i][j] = (arr_gray_curr_v[i][j] - arr_gray_prev_v[i][j] + arr_gray_curr_v[i+1][j] - arr_gray_prev_v[i+1][j] + arr_gray_curr_v[i][j+1] - arr_gray_prev_v[i][j+1] + arr_gray_curr_v[i+1][j+1] - arr_gray_prev_v[i+1][j+1])/4;
			}
		}

		for(int i=0; i<(WIDTH_V-2); i++){
		    for(int j=0; j<(HEIGHT_V-2); j++){
                mu_u_v[i][j] = (u_v[i][j+1] + u_v[i+1][j] + u_v[i+2][j+1] + u_v[i+1][j+2])/6 + (u_v[i][j] + u_v[i][j+2] + u_v[i+2][j] + u_v[i+2][j+2])/12;
                mu_v_v[i][j] = (v_v[i][j+1] + v_v[i+1][j] + v_v[i+2][j+1] + v_v[i+1][j+2])/6 + (v_v[i][j] + v_v[i][j+2] + v_v[i+2][j] + v_v[i+2][j+2])/12;
		    }
		}

		for(int i=0; i<(WIDTH_V-2); i++){
		    for(int j=0; j<(HEIGHT_V-2); j++){
                u_v[i][j] = mu_u_v[i][j] - Ix_v[i][j]*((Ix_v[i][j]*mu_u_v[i][j] + Iy_v[i][j]*mu_v_v[i][j] + It_v[i][j])/(ALPHA*ALPHA + Ix_v[i][j]*Ix_v[i][j] + Iy_v[i][j]*Iy_v[i][j]));
                v_v[i][j] = mu_v_v[i][j] - Iy_v[i][j]*((Ix_v[i][j]*mu_u_v[i][j] + Iy_v[i][j]*mu_v_v[i][j] + It_v[i][j])/(ALPHA*ALPHA + Ix_v[i][j]*Ix_v[i][j] + Iy_v[i][j]*Iy_v[i][j]));
		    }
		}

        // ------------------------------------ //
		// -- Computation Horizontal Command -- //
        // ------------------------------------ //

        OFright = 0;
        OFleft = 0;

        for (int i=0; i<((WIDTH_H/2)-2); i++){            // Size of u,v are (WIDTH-1) and (HEIGHT-1), respectively.
            for(int j=0; j<(HEIGHT_H-2); j++){
                OFright = OFright + sqrt((u_h[i][j]*u_h[i][j]) + (v_h[i][j]*v_h[i][j]));
            }
        }
        for (int i=((WIDTH_H/2)); i<(WIDTH_H-2); i++){      // Size of u,v are (WIDTH-1) and (HEIGHT-1), respectively.
            for(int j=0; j<(HEIGHT_H-2); j++){
                OFleft = OFleft + sqrt((u_h[i][j]*u_h[i][j]) + (v_h[i][j]*v_h[i][j]));
            }
        }

        D_OF_RL = P_RL_GAIN*(OFright - OFleft);

        // ---------------------------------- //
		// -- Computation Vertical Command -- //
        // ---------------------------------- //

        OFup = 0;
        OFdown = 0;

        for(int i=0; i<(WIDTH_V-2); i++){
            for(int j=0; j<((HEIGHT_V/2)-2); j++){
                OFup = OFup + sqrt((u_v[i][j]*u_v[i][j]) + (v_v[i][j]*v_v[i][j]));
            }
            for(int j=(HEIGHT_V/2); j<(HEIGHT_V-2); j++){
                OFdown = OFdown + sqrt((u_v[i][j]*u_v[i][j]) + (v_v[i][j]*v_v[i][j]));
            }
        }

        D_OF_UD = P_UD_GAIN*(OFup - OFdown);

        // -------------------------------- //
        // -- Target Position Generation -- //
        // -------------------------------- //

        if(count>=5){
            pose_o_ex_t = 0;
            pose_o_ey_t = 0;
            pose_o_ez_t = pose_o_ez_c + D_OF_RL;
            pose_p_x_t = pose_p_x_c + D_SET*cos(pose_o_ez_t);
            pose_p_y_t = pose_p_y_c + D_SET*sin(pose_o_ez_t);
            pose_p_z_t = 1.5+D_OF_UD;

            cy = cos(pose_o_ez_t * 0.5);
            sy = sin(pose_o_ez_t * 0.5);
            cr = cos(pose_o_ey_t * 0.5);
            sr = sin(pose_o_ey_t * 0.5);
            cp = cos(pose_o_ex_t * 0.5);
            sp = sin(pose_o_ex_t * 0.5);

            pose_o_qw_t = cy * cr * cp + sy * sr * sp;
            pose_o_qx_t = cy * sr * cp - sy * cr * sp;
            pose_o_qy_t = cr * cr * sp + sy * sr * cp;
            pose_o_qz_t = sy * cr * cp - cy * sr * sp;
        }

        // ------------- //
        // -- Display -- //
        // ------------- //

        for(int i=0; i<(WIDTH_H-1); i++){
		    for(int j=0; j<(HEIGHT_H-1); j++){
                p2_h[i][j].x = p1_h[i][j].x+(int)(u_h[i][j]*5);
                p2_h[i][j].y = p1_h[i][j].y+(int)(v_h[i][j]*5);
		    }
		}

		for(int i=0; i<(WIDTH_V-1); i++){
		    for(int j=0; j<(HEIGHT_V-1); j++){
                p2_v[i][j].x = p1_v[i][j].x+(int)(u_v[i][j]*5);
                p2_v[i][j].y = p1_v[i][j].y+(int)(v_v[i][j]*5);
		    }
		}

        for(int i=0; i<(WIDTH_H-1); i++){
		    for(int j=0; j<(HEIGHT_H-1); j++){
                arrowedLine(mat_arrow_h,p1_h[i][j],p2_h[i][j],0,3,CV_AA,0,1);
		    }
		}

		for(int i=0; i<(WIDTH_V-1); i++){
		    for(int j=0; j<(HEIGHT_V-1); j++){
                arrowedLine(mat_arrow_v,p1_v[i][j],p2_v[i][j],0,3,CV_AA,0,1);
		    }
		}

		namedWindow("Img_Array",WINDOW_NORMAL);
		imshow("Img_Array",mat_array);
		namedWindow("Optical_flow_h",WINDOW_NORMAL);
		imshow("Optical_flow_h",mat_arrow_h);
		namedWindow("Optical_flow_v",WINDOW_NORMAL);
		imshow("Optical_flow_v",mat_arrow_v);

		keypressed = (char)waitKey(10);
		if(keypressed == 27)
			break;

        oa_of::MsgOAOF msg;
        geometry_msgs::PoseStamped msg_setposition;

		msg.data = count;
		msg_setposition.pose.position.x = pose_p_x_t;
		msg_setposition.pose.position.y = pose_p_y_t;
		msg_setposition.pose.position.z = pose_p_z_t;
		msg_setposition.pose.orientation.x = pose_o_qx_t;
		msg_setposition.pose.orientation.y = pose_o_qy_t;
		msg_setposition.pose.orientation.z = pose_o_qz_t;
		msg_setposition.pose.orientation.w = pose_o_qw_t;

		oa_of_pub.publish(msg);
		Set_Position_pub.publish(msg_setposition);

		ROS_INFO("Send msg = %f", count);
		ROS_INFO("Target X = %f", pose_p_x_t);
		ROS_INFO("Target Y = %f", pose_p_y_t);
		ROS_INFO("Target Z = %f", pose_p_z_t);
		ROS_INFO("Target Yaw = %f", pose_o_ez_t);
		ROS_INFO("Eta_h = %f", eta_h_sum);

        ros::spinOnce();
        loop_rate.sleep();
		count = count + S_TIME;
	}

	return 0;
}
