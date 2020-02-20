//
// PointCloudPostProcessor.cpp
// Post processes generated point clouds (registration, outlier removal, etc...)
//

#include <pcl/common/transforms.h>
#include <pcl/features/multiscale_feature_persistence.h>
#include <pcl/registration/correspondence_rejection_sample_consensus.h>
#include <pcl/registration/correspondence_estimation.h>
#include <pcl/registration/transformation_estimation_svd.h>

#include "point_cloud/PointCloudPostProcessor.hpp"
#include "point_cloud/FeatureExtractor.hpp"

typedef pcl::FPFHEstimation<pcl::PointXYZRGB, pcl::Normal, pcl::FPFHSignature33> FPFH;

namespace PointCloud
{
    PointCloudPostProcessor::PointCloudPostProcessor(const Config::Config& config) : m_Config(config)
    {
        // setup outlier remover with default values
        m_OutlierRemover.setMeanK(m_Config.PointCloudPostProcess.OutlierMinK);
        m_OutlierRemover.setStddevMulThresh(m_Config.PointCloudPostProcess.OutlierStdDevThreshold);

        // setup ICP alignment
        m_ICP.setMaximumIterations(25);
        m_ICP.setRANSACIterations(25);
        m_ICP.setMaxCorrespondenceDistance(500);

        // setup feature extractor
        m_FeatureExtractor = std::make_unique<FeatureExtractor>(m_Config.PointCloudPostProcess.KeypointDetector, m_Config.PointCloudPostProcess.FeatureDetector, m_Config);

        // setup feature descriptor
        m_FeatureDescriptor = FPFH::Ptr(new FPFH());
    }

    // Outlier removal
    void PointCloudPostProcessor::RemoveOutliers(pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr input, pcl::PointCloud<pcl::PointXYZRGB>::Ptr output) {
        m_OutlierRemover.setInputCloud(input);
        m_OutlierRemover.filter(*output);
    }

    // ICP alignment
    bool PointCloudPostProcessor::AlignPointCloud(PointCloudConstPtr source, PointCloudConstPtr target, PointCloudPtr result)
    {
        // extract features from both point clouds and get correspondences
        FeatureDetectionResult sourceFeatures;
        FeatureDetectionResult targetFeatures;
        pcl::CorrespondencesPtr correspondences(new pcl::Correspondences());

        ExtractFeatures(source, sourceFeatures);
        ExtractFeatures(target, targetFeatures);

        switch (m_Config.PointCloudPostProcess.FeatureDetector)
        {
            case FEATURE_DETECTOR_FPFH:
            {
                pcl::registration::CorrespondenceEstimation<pcl::FPFHSignature33, pcl::FPFHSignature33> corr;
                corr.setInputSource(sourceFeatures.FPFHFeatures);
                corr.setInputTarget(targetFeatures.FPFHFeatures);
                corr.determineCorrespondences(*correspondences);
                break;
            }
            case FEATURE_DETECTOR_SHOT_COLOR:
            {
                // TODO: Implement SHOT COLOR
                break;
            }
        }

        // reject invalid correspondences using RANSAC
        pcl::CorrespondencesPtr validCorrespondences(new pcl::Correspondences());
        pcl::registration::CorrespondenceRejectorSampleConsensus<pcl::PointXYZRGB> rejector;

        rejector.setInputSource(source);
        rejector.setInputTarget(target);
        rejector.setInlierThreshold(2.5);
        rejector.setMaximumIterations(10);
        rejector.setRefineModel(false);
        rejector.setInputCorrespondences(correspondences);
        rejector.getCorrespondences(*validCorrespondences);

        // compute estimated rigid body transform
        Eigen::Matrix4f T = Eigen::Matrix4f::Identity();
        pcl::registration::TransformationEstimationSVD<pcl::PointXYZRGB, pcl::PointXYZRGB> transformationEstimator;
        transformationEstimator.estimateRigidTransformation(*source, *target, *validCorrespondences, T);

        //std::cout << "\nEstimated transform: \n" << T << std::endl;

        // apply transform to source
        pcl::transformPointCloud(*source, *result, T);

        return true;
    }

    // Extract features from point cloud
    void PointCloudPostProcessor::ExtractFeatures(PointCloudConstPtr cloud, FeatureDetectionResult& result) const
    {
        // TODO: uncomment when using keypoint detector that uses normals
        NormalsPtr normals = NormalsPtr(new pcl::PointCloud<pcl::Normal>());
        //m_FeatureExtractor->ComputeNormals(cloud, normals);

        // compute keypoints
        PointCloudPtr keypoints = PointCloudPtr(new pcl::PointCloud<PointType>());
        m_FeatureExtractor->ComputeKeypoints(cloud, normals, keypoints);

        // compute keypoint normals
        NormalsPtr keypointNormals = NormalsPtr(new pcl::PointCloud<pcl::Normal>());
        m_FeatureExtractor->ComputeNormals(keypoints, keypointNormals);

        // compute features using keypoints
        m_FeatureExtractor->ComputeFeatures(keypoints, keypointNormals, result);
    }

    // Setters
    void PointCloudPostProcessor::SetMinimumNeighboursOutlierRemoval(int k) {
        m_OutlierRemover.setMeanK(k);
    }

    void PointCloudPostProcessor::SetStdDevOutlierRemoval(double std) {
        m_OutlierRemover.setStddevMulThresh(std);
    }
}