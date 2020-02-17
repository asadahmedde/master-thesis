//
// localization_test_kitti.cpp
// localization test on kitti dataset
//

#include <iostream>
#include <vector>
#include <algorithm>
#include <thread>

#include <eigen3/Eigen/Eigen>

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/io/pcd_io.h>
#include <pcl/visualization/pcl_visualizer.h>

#include "reconstruct/Reconstruct3D.hpp"
#include "camera/CameraCalibParser.hpp"
#include "config/ConfigParser.hpp"
#include "pipeline/ReconstructionPipeline.hpp"

#include "tests_common.hpp"

// test file names
#define LOCALIZATION_DATA_FILE "localization_data.txt"

void AddXYZPattern(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud);

int main(int argc, char** argv)
{
    // read in localization data and convert to frames
    std::vector<Pipeline::StereoFrame> frames;
    std::vector<LocalizationData> localizationData;

    ReadLocalizationData(LOCALIZATION_DATA_FILE, localizationData);
    ConvertLocalizationsToStereoFrames(localizationData, frames);

    // get calib and config
    Camera::Calib::StereoCalib stereoCalib;
    Config::Config config;
    GetCalibAndConfig(stereoCalib, config);

    // prepare localization module
    Reconstruct::Localizer localizer;

    // main process loop
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr pointCloud { new pcl::PointCloud<pcl::PointXYZRGB>() };
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr temp { new pcl::PointCloud<pcl::PointXYZRGB>() };
    pcl::PointCloud<pcl::PointXYZRGB>::Ptr input { new pcl::PointCloud<pcl::PointXYZRGB>() };

    AddXYZPattern(input);

    if (frames.empty()) {
        std::cerr << "\nError: Frames list is empty. Aborting" << std::endl;
        return 1;
    }

    int i = 0;
    for (const auto& frame : frames)
    {
        input->clear();

        pcl::PointXYZRGB p{};
        p.z = 10;

        if (i == 0) p.r = 255;
        if (i == 1) p.g = 255;
        if (i == 2) p.b = 255;

        input->push_back(p);

        localizer.TransformPointCloud(frame, *input, *temp);
        *pointCloud += *temp;
        temp->clear();

        i++;
    }

    // visualise using PCL
    pcl::visualization::PCLVisualizer::Ptr viewer = rgbVis(pointCloud);
    while (!viewer->wasStopped ()) {
        viewer->spinOnce (100);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // save to disk
    std::cout << "\nSaving PCD to disk" << std::endl;
    pcl::io::savePCDFileBinary("localized.pcd", *pointCloud);

    std::cout << std::endl;
    return 0;
}

// Add points to point cloud in X,Y,Z pattern
void AddXYZPattern(pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud)
{
    const float scale = 1.0;

    pcl::PointXYZRGB x{};
    x.x = 1.0f * scale;
    x.r = 255;

    pcl::PointXYZRGB y{};
    y.y = 1.0f * scale;
    y.g = 255;

    pcl::PointXYZRGB z{};
    z.z = 1.0f * scale;
    z.b = 255;

    cloud->push_back(x);
    cloud->push_back(y);
    cloud->push_back(z);
}
