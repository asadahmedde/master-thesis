//
// test_camera_calib_parser.cpp
// Tests for parsing the camera calib parser
//

#define CATCH_CONFIG_MAIN

#include "catch2/catch.hpp"
#include "camera/CameraCalibParser.hpp"
#include "camera/CameraCalib.hpp"

#include <eigen3/Eigen/Eigen>

// constants for camera calibration from dataset
const Eigen::Matrix3f K1 = (Eigen::Matrix3f() << 837.619011, 0.0, 522.434637,
        0.0, 839.808333, 402.367400,
        0.0, 0.0, 1.0).finished();

const Eigen::Matrix3f K2 = (Eigen::Matrix3f() << 835.542079, 0.0, 511.127987,
        0.0, 837.180798, 388.337888,
        0.0, 0.0, 1.0).finished();

const Eigen::Vector2i IMAGE_RESOLUTION {1024, 768 };

const Eigen::Matrix3f R = (Eigen::Matrix3f() << 9.9997625494747e-001, -6.3729476131001e-003, -2.6220373684323e-003,
        6.3750339453031e-003, 9.9997936870410e-001, 7.8810427338438e-004,
        2.6169607251553e-003, -8.0480113703670e-004, 9.9999625189882e-001).finished();

const Eigen::Vector3f T = (Eigen::Vector3f() << 1.194711e-001, 3.144088e-004, 1.423872e-004).finished();

const Eigen::Vector2i IMAGE_RES { 1024, 768 };


TEST_CASE("JSON calib file parsed correctly", "[camera_calib_parse]")
{
    Camera::CameraCalibParser parser;
    Camera::Calib::StereoCalib calib;

    parser.ParseStereoCalibJSONFile("../resources/calib/test_calib.json", calib);

    // distortion co-effs
    Eigen::VectorXf D1(4);
    D1 << -3.636834e-001, 1.766205e-001, 0.000000e+000, 0.000000e+000;

    Eigen::VectorXf D2(4);
    D2 << -3.508059e-001, 1.538358e-001, 0.000000e+000, 0.000000e+000;

    // check all matrices were parse correctly
    REQUIRE(calib.LeftCameraCalib.K == K1);
    REQUIRE(calib.LeftCameraCalib.D == D1);
    REQUIRE(calib.RightCameraCalib.K == K2);
    REQUIRE(calib.RightCameraCalib.D == D2);
    REQUIRE(calib.LeftCameraCalib.ImageResolutionInPixels == IMAGE_RES);
    REQUIRE(calib.RightCameraCalib.ImageResolutionInPixels == IMAGE_RES);

    // extrinsics
    REQUIRE(calib.R == R);
    REQUIRE(calib.T == T);
}

