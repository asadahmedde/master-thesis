//
// Tracker.cpp
// Tracks frames and estimates transforms between frames
//

#include <opencv2/core/core.hpp>
#include <eigen3/Eigen/Eigen>
#include <pcl/common/geometry.h>

#include "system/TrackingFrame.hpp"
#include "system/Tracker.hpp"

#define MIN_CORRESPONDENCES_NEEDED 20
#define MIN_DISTANCE_FOR_NEW_KEYFRAME 5.0

namespace System
{
    // Constructor
    Tracker::Tracker(std::shared_ptr<Pipeline::FrameFeatureExtractor> featureExtractor,
                     std::shared_ptr<Reconstruct::Reconstruct3D> reconstructor,
                     std::shared_ptr<MappingSystem> mappingSystem,
                     std::shared_ptr<KeyFrameDatabase> keyFrameDB) : m_FeatureExtractor(std::move(featureExtractor)), m_3DReconstructor(reconstructor), m_MappingSystem(mappingSystem), m_KeyFrameDatabase(keyFrameDB)
    {
        // setup optimsation graph with camera params
        float fx, fy, cx, cy;
        m_3DReconstructor->GetCameraParameters(fx, fy, cx, cy);
        m_OptimisationGraph = std::make_unique<OptimisationGraph>(fx, fy, cx, cy);
    }

    // Track this frame and update estimated position and rotation of the camera
    void Tracker::TrackFrame(std::shared_ptr<TrackingFrame> frame)
    {
        // if no keyframes add this as the first keyframe
        if (m_KeyFrames.empty())
        {
            m_KeyFrames.push_back(frame);
            int poseVertexID = m_OptimisationGraph->AddDefaultCameraPoseVertex(true);
            m_KeyFramePoseVertexIDs[m_KeyFrames.size() - 1] = poseVertexID;
        }
        else {
            // track against last keyframe
            std::shared_ptr<TrackingFrame> recentKeyFrame = m_KeyFrames.back();
            return TrackFrame(frame, recentKeyFrame);
        }
    }
    
    // Track between previous frame and this new frame
    void Tracker::TrackFrame(std::shared_ptr<TrackingFrame> currentFrame, std::shared_ptr<TrackingFrame> recentKeyFrame)
    {
        // find correspondences between frames
        std::vector<cv::KeyPoint> keyFrameKeyPoints;
        std::vector<cv::KeyPoint> currentFrameKeyPoints;
        m_FeatureExtractor->ComputeCorrespondences(recentKeyFrame->GetCameraImage(), currentFrame->GetCameraImage(), keyFrameKeyPoints, currentFrameKeyPoints, recentKeyFrame->GetCameraImageMask(), currentFrame->GetCameraImageMask());
        
        // not enough matches - will not get a robust solution for alignment
        if (keyFrameKeyPoints.size() <= MIN_CORRESPONDENCES_NEEDED) {
            std::cerr << "\nTracking lost (" << keyFrameKeyPoints.size() << " matches found)";
            cv::Mat output;
        }
        
        // keyframe pose is already in graph - add (temporarily) the current frame pose to graph
        int currentCameraID = m_OptimisationGraph->AddDefaultCameraPoseVertex(false);
        
        // add common 3D keypoints to graph (points seen by keyframe camera and this frame's camera)
        std::vector<pcl::PointXYZRGB> triangulatedPoints;
        m_3DReconstructor->TriangulatePoints(recentKeyFrame->GetDisparity(), recentKeyFrame->GetCameraImage(), keyFrameKeyPoints, triangulatedPoints);
        
        // get last recent keyframe camera ID
        int keyFrameCameraID = m_KeyFramePoseVertexIDs[m_KeyFrames.size() - 1];
        
        // add 3D points for these 2 cameras looking at these common 3D points
        std::vector<int> cameras { keyFrameCameraID, currentCameraID };
        std::vector<std::vector<cv::KeyPoint>> projectedPoints;
        projectedPoints.emplace_back(std::move(keyFrameKeyPoints));
        projectedPoints.emplace_back(std::move(currentFrameKeyPoints));
        m_OptimisationGraph->AddCamerasLookingAtPoints(cameras, triangulatedPoints, projectedPoints, true);
        
        // solve for poses
        m_OptimisationGraph->Optimise();
        
        // get pose of current frame vs recent keyframe
        Eigen::Isometry3d currentFramePose = m_OptimisationGraph->GetCameraPose(currentCameraID);
        Eigen::Isometry3d keyFramePose = m_OptimisationGraph->GetCameraPose(keyFrameCameraID);
        
        // update current tracked pose
        m_CurrentPose = currentFramePose.matrix();
        
        // get position difference from last keyframe
        Eigen::Vector3d t1 = currentFramePose.translationExt();
        Eigen::Vector3d t0 = keyFramePose.translationExt();
        float distanceFromKeyFrame = (t1 - t0).norm();
        
        std::cout << "\nDistance from last keyframe: " << distanceFromKeyFrame << std::endl;
        
        // if distance or rotation is above threshold - insert as new keyframe
        // TODO: add rotation check here too
        bool isNowKeyFrame = (distanceFromKeyFrame >= MIN_DISTANCE_FOR_NEW_KEYFRAME);
        
        // exceeded distance threshold - insert new keyframe
        if (isNowKeyFrame)
        {
            // add as keyframe and store optimisation graph id
            m_KeyFrames.push_back(currentFrame);
            m_KeyFramePoseVertexIDs[m_KeyFrames.size() - 1] = currentCameraID;
            
            // this camera pose stays - make it fixed if only 2 keyframes exist
            m_OptimisationGraph->SetCameraPoseFixed(currentCameraID, m_KeyFrames.size() <= 2);
            
            std::cout << "\nAdded keyframe" << std::endl;
        }
        else {
             m_OptimisationGraph->RemoveCameraPoseVertex(currentCameraID);
        }
        
        // delete all temp vertices
        m_OptimisationGraph->RemoveTempEdges();
    }

    // Get current tracked pose
    Eigen::Matrix4d Tracker::GetPose() const {
        return m_CurrentPose;
    }
}
