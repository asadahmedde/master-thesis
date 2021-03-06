//
// FrameFeatureExtractor.cpp
// Extracts 2D features from stereo frames
//

#include <opencv2/flann/miniflann.hpp>

#include "pipeline/FrameFeatureExtractor.hpp"

// Lowe ratio test
#define RATIO_THRESHOLD 0.7f

namespace Pipeline
{
    // Constructor
    FrameFeatureExtractor::FrameFeatureExtractor()
    {
        // setup feature detector as BRISK
        m_FeatureExtractor = cv::BRISK::create();

        // setup BF matcher
        m_BFMatcher = cv::BFMatcher::create(cv::NORM_HAMMING, false);
    }

    // Find correspondences
    void FrameFeatureExtractor::ComputeCorrespondences(const cv::Mat& image1, const cv::Mat& image2, std::vector<cv::KeyPoint>& keypoints1, std::vector<cv::KeyPoint>& keypoints2, std::vector<cv::DMatch>& matches) const
    {
        // image 1 features
        cv::Mat descriptors1;
        m_FeatureExtractor->detectAndCompute(image1, cv::noArray(), keypoints1, descriptors1, false);

        // image 2 features
        cv::Mat descriptors2;
        m_FeatureExtractor->detectAndCompute(image2, cv::noArray(), keypoints2, descriptors2, false);

        // feature matching
        std::vector<std::vector<cv::DMatch>> allMatches;
        m_BFMatcher->knnMatch(descriptors1, descriptors2, allMatches, 2);

        // filter using Lowe ratio test
        FilterForGoodMatches(allMatches, matches);
    }

    void FrameFeatureExtractor::ComputeCorrespondences(const cv::Mat& d1, const cv::Mat& d2, std::vector<cv::DMatch>& matches) const
    {
        // feature matching
        std::vector<std::vector<cv::DMatch>> allMatches;
        m_BFMatcher->knnMatch(d1, d2, allMatches, 2);

        // filter using Lowe ratio test
        FilterForGoodMatches(allMatches, matches);
    }

    void FrameFeatureExtractor::ComputeCorrespondences(const cv::Mat& image1, const cv::Mat& image2, std::vector<cv::KeyPoint>& kp1, std::vector<cv::KeyPoint>& kp2, cv::InputArray mask1, cv::InputArray mask2) const
    {
        // feature matching
        std::vector<cv::KeyPoint> points1; std::vector<cv::KeyPoint> points2;
        cv::Mat descriptors1; cv::Mat descriptors2;
        
        m_FeatureExtractor->detectAndCompute(image1, mask1, points1, descriptors1);
        m_FeatureExtractor->detectAndCompute(image2, mask2, points2, descriptors2);
        
        std::vector<std::vector<cv::DMatch>> allMatches;
        m_BFMatcher->knnMatch(descriptors1, descriptors2, allMatches, 2);

        // filter using Lowe ratio test
        std::vector<cv::DMatch> matches;
        FilterForGoodMatches(allMatches, matches);
        
        // add keypoints of matches
        for (const cv::DMatch& match : matches) {
            kp1.push_back(points1[match.queryIdx]);
            kp2.push_back(points2[match.trainIdx]);
        }
    }

    // Find matches with pre-computed descriptors and image
    void FrameFeatureExtractor::ComputeMatchesWithImage(const cv::Mat& descriptors, const cv::Mat& image, std::vector<cv::KeyPoint>& computedKeypoints, cv::Mat& computedDescriptors, std::vector<cv::DMatch>& matches) const
    {
        // detect features in image
        m_FeatureExtractor->detectAndCompute(image, cv::noArray(), computedKeypoints, computedDescriptors, false);

        std::vector<std::vector<cv::DMatch>> allMatches;
        m_BFMatcher->knnMatch(descriptors, computedDescriptors, allMatches, 2);

        // Lowe ratio test
        FilterForGoodMatches(allMatches, matches);
    }

    // Features from image
    void FrameFeatureExtractor::ComputeFeaturesFromImage(const cv::Mat& image, std::vector<cv::KeyPoint>& computedKeypoints, cv::Mat& computedDescriptors) const {
        m_FeatureExtractor->detectAndCompute(image, cv::noArray(), computedKeypoints, computedDescriptors, false);
    }

    // Lowe match ratio test
    void FrameFeatureExtractor::FilterForGoodMatches(const std::vector<std::vector<cv::DMatch>>& matches, std::vector<cv::DMatch>& goodMatches) const
    {
        for (size_t i = 0; i < matches.size(); i++)
        {
            if (!matches[i].empty() && matches[i][0].distance < RATIO_THRESHOLD * matches[i][1].distance) {
                goodMatches.push_back(matches[i][0]);
            }
        }
    }
}
