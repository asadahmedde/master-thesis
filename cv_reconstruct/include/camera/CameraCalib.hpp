//
// CameraCalib.hpp
// Structs containing calibration information for camera and stereo rig
//

#ifndef CV_RECONSTRUCT_CAMERACALIB_HPP
#define CV_RECONSTRUCT_CAMERACALIB_HPP

#include <opencv2/core/core.hpp>
#include <eigen3/Eigen/Eigen>

namespace Camera
{
    namespace Calib
    {
        /// The calib for a single camera (intrinsics (K), distortion coeffs (D), and image resolution)
        struct CameraCalib
        {
            Eigen::Matrix3f K = Eigen::Matrix3f::Zero();
            Eigen::VectorXf D = Eigen::VectorXf(8);
            Eigen::Vector2i ImageResolutionInPixels = Eigen::Vector2i::Zero();
        };

        /// Rectification for the stereo camera system
        struct StereoRectification
        {
            // Rectified rotation transform for left (RL) and right (RR) cameras
            cv::Mat RL;
            cv::Mat RR;

            // Projection matrices for the rectified coordinate system for the left (PL), and right (PR) cameras
            cv::Mat PL;
            cv::Mat PR;

            // 4x4 disparity to depth mapping matrix
            cv::Mat Q;

            // Valid image spaces after rectification in rectified images
            cv::Rect ValidRectLeft;
            cv::Rect ValidRectRight;
        };

        /// The calib for a stereo camera rig
        struct StereoCalib
        {
            // Left and right camera settings
            CameraCalib LeftCameraCalib{};
            CameraCalib RightCameraCalib{};

            // Epipolar geometry with essential and fundamental matrices
            // (currently not used)
            Eigen::Matrix3f E = Eigen::Matrix3f::Zero();
            Eigen::Matrix3f F = Eigen::Matrix3f::Zero();
            
            // Relative rotation of 2nd camera to 1st
            Eigen::Matrix3f R = Eigen::Matrix3f::Zero();

            // Transform from left camera origin to right camera origin
            Eigen::Vector3f T = Eigen::Vector3f::Zero();

            // Stereo rectified transform
            StereoRectification Rectification;
        };
    }
}

#endif //CV_RECONSTRUCT_CAMERACALIB_HPP
