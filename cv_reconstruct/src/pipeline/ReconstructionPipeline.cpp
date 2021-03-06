//
// ReconstructionPipeline.cpp
// Pipeline responsible for complete processing of a frame to a point cloud
// Responsible for configuring required components according to the config
//

#include <opencv2/imgproc/imgproc.hpp>

#include "pipeline/ReconstructionPipeline.hpp"

namespace Pipeline
{
    // Constructor
    ReconstructionPipeline::ReconstructionPipeline(const Config::Config& config, const Camera::Calib::StereoCalib& calib, bool isProcessingRectifiedImages) : m_ShouldRectifyImages(!isProcessingRectifiedImages), m_Config(config)
    {
        // setup pipeline components

        // 3D reconstructor
        m_Reconstructor = std::make_unique<Reconstruct::Reconstruct3D>(calib, config);

        // point cloud post processor
        m_PointCloudPostProcessor = std::make_unique<PointCloud::PointCloudPostProcessor>(m_Config);
        m_PointCloudPostProcessor->SetMinimumNeighboursOutlierRemoval(m_Config.PointCloudPostProcess.OutlierMinK);
        m_PointCloudPostProcessor->SetStdDevOutlierRemoval(m_Config.PointCloudPostProcess.OutlierStdDevThreshold);

        // localization
        m_Localizer = std::make_unique<Reconstruct::Localizer>();

        // point cloud registration
        m_PointCloudRegistration = std::make_unique<PointCloud::PointCloudRegistration>(config);
    }

    // Calculate disparity map
    void ReconstructionPipeline::CalculateDisparity(const Pipeline::StereoFrame& frame, cv::Mat& disparity) const
    {
        // rectify if image rectification required
        if (m_ShouldRectifyImages)
        {
            cv::Mat leftImageRectified, rightImageRectified;
            m_Reconstructor->RectifyImages(frame.LeftImage, frame.RightImage, leftImageRectified, rightImageRectified);
            disparity = m_Reconstructor->GenerateDisparityMap(leftImageRectified, rightImageRectified);
        }
        else {
            disparity = m_Reconstructor->GenerateDisparityMap(frame.LeftImage, frame.RightImage);
        }

        // compute std dev of disparity and threshold it
        cv::Mat disp; cv::Mat mean; std::vector<double> std;
        cv::normalize(disparity, disp, 0, 255, cv::NORM_MINMAX, CV_8U);
        cv::Mat mask(disp.rows, disp.cols, CV_8U);
        cv::meanStdDev(disp, mean, std);
        cv::threshold(disp, mask, std[0] * 1.3, 255, cv::THRESH_BINARY);

        // apply mask to disparity
        for (int row = 0; row < mask.rows; row++)
        {
            for (int col = 0; col < mask.cols; col++)
            {
                int value = static_cast<int>(mask.at<unsigned char>(row, col));
                if (value == 0) {
                    disparity.at<short>(row, col) = 0;
                }
            }
        }
    }

    // Process frame and generate processed, localized, point cloud
    void ReconstructionPipeline::ProcessFrame(const StereoFrame& frame, pcl::PointCloud<pcl::PointXYZRGB>::Ptr& result)
    {
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud{ new pcl::PointCloud<pcl::PointXYZRGB>() };
        PipelineResult pipelineResult{};

        if (frame.ID == 0) {
            ProcessFirstFrame(frame, pipelineResult);
        }
        else {
            ProcessSubsequentFrame(frame, pipelineResult);
        }

        result = pipelineResult.PointCloudLocalized;
    }

    // Process this as the first frame
    void ReconstructionPipeline::ProcessFirstFrame(const Pipeline::StereoFrame &frame, PipelineResult& result)
    {
        // disparity image
        CalculateDisparity(frame, result.DisparityImage);

        // triangulation
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr localPointCloud(new pcl::PointCloud<pcl::PointXYZRGB>(m_Reconstructor->GeneratePointCloud(result.DisparityImage, frame.LeftImage)));

        // remove outliers
        m_PointCloudPostProcessor->RemoveOutliers(localPointCloud, localPointCloud);

        // transform point cloud
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr transformedPointCloud { new pcl::PointCloud<pcl::PointXYZRGB>() };
        Eigen::Matrix4f T = m_Localizer->TransformPointCloud(frame, *localPointCloud, *transformedPointCloud);

        // set first frame for registration pipeline
        cv::Mat projected3D;
        m_Reconstructor->Project3D(result.DisparityImage, projected3D);
        m_PointCloudRegistration->SaveFirstFrame(frame.LeftImage, projected3D, T);

        result.PointCloudLocalized = transformedPointCloud;
    }

    // Process a frame that has had a frame processed before it. Assumes m_LastPipelineResult has valid data set.
    void ReconstructionPipeline::ProcessSubsequentFrame(const Pipeline::StereoFrame &frame, PipelineResult& result)
    {
        // disparity image
        CalculateDisparity(frame, result.DisparityImage);

        // triangulation
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr localPointCloud(new pcl::PointCloud<pcl::PointXYZRGB>(m_Reconstructor->GeneratePointCloud(result.DisparityImage, frame.LeftImage)));

        // remove outliers
        m_PointCloudPostProcessor->RemoveOutliers(localPointCloud, localPointCloud);

        // transform point cloud
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr transformedPointCloud { new pcl::PointCloud<pcl::PointXYZRGB>() };
        Eigen::Matrix4f T = m_Localizer->TransformPointCloud<pcl::PointXYZRGB>(frame, *localPointCloud, *transformedPointCloud);

        // align with last transformed point cloud
        pcl::PointCloud<pcl::PointXYZRGB>::Ptr alignedPointCloud { new pcl::PointCloud<pcl::PointXYZRGB>() };
        cv::Mat projected3D;
        m_Reconstructor->Project3D(result.DisparityImage, projected3D);
        m_PointCloudRegistration->RegisterFrameWithPreviousFrame(frame.LeftImage, projected3D, T, transformedPointCloud, alignedPointCloud);

        result.PointCloudLocalized = alignedPointCloud;
    }
}