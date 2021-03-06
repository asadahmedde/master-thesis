//
// OpticalFlowEstimator.cpp
// Estimates pixel movement using Optical Flow
//

#include <iostream>
#include <unordered_set>

#include "pipeline/OpticalFlowEstimator.hpp"

#define NUM_LEVELS 3
#define PYR_SCALE 0.5
#define FAST_PYR false
#define WIN_SIZE 179
#define NUM_ITERS 10
#define POLY_N 5
#define POLY_SIGMA 1.0

namespace Features
{
    // Constructor
    OpticalFlowEstimator::OpticalFlowEstimator() {
        m_FarnebackOF = cv::FarnebackOpticalFlow::create(NUM_LEVELS, PYR_SCALE, FAST_PYR, WIN_SIZE, NUM_ITERS, POLY_N, POLY_SIGMA);
    }

    // Flow from image1 to image2
    void OpticalFlowEstimator::EstimateCorrespondingPixels(const cv::Mat& image1, const cv::Mat& image2, std::vector<cv::KeyPoint>& points1, std::vector<cv::KeyPoint>& points2, cv::InputArray mask1, cv::InputArray mask2)
    {
        // convert to greyscale
        cv::Mat prev; cv::Mat next;
        cv::cvtColor(image1, prev, cv::COLOR_BGR2GRAY);
        cv::cvtColor(image2, next, cv::COLOR_BGR2GRAY);
        
        // setup masks if provided
        bool isMasked1 = (&mask1 != &cv::noArray());
        bool isMasked2 = (&mask2 != &cv::noArray());
        cv::Mat maskImage1; cv::Mat maskImage2;
        
        if (isMasked1) {
            maskImage1 = mask1.getMat();
        }
        if (isMasked2) {
            maskImage2 = mask2.getMat();
        }
        
        // compute flow
        cv::Mat flow;
        m_FarnebackOF->calc(prev, next, flow);
        
        // split into x and y components
        std::vector<cv::Mat> components;
        cv::split(flow, components);
        
        float dx; float dy;
        float x1; float y1;
        for (int y0 = 0; y0 < flow.rows; y0++)
        {
            for (int x0 = 0; x0 < flow.cols; x0++)
            {
                // if not in mask then skip
                if (isMasked1)
                {
                    int value = static_cast<int>(maskImage1.at<unsigned char>(y0, x0));
                    if (value == 0) {
                        continue;
                    }
                }
                
                // amount moved from prev to next
                dx = components[0].at<float>(y0, x0);
                dy = components[1].at<float>(y0, x0);
                
                // new pixel positions in image 2
                x1 = x0 + dx;
                y1 = y0 + dy;
                
                // skip point - has gone out of image
                if (x1 < 0.0f || x1 >= flow.cols) {
                    continue;
                }
                else if (y1 < 0.0f || y1 >= flow.rows) {
                    continue;
                }
                
                // check mask in image 2 after flow
                if (isMasked2)
                {
                    int value = static_cast<int>(maskImage2.at<unsigned char>(y1, x1));
                    if (value == 0) {
                        continue;
                    }
                }
                
                // pixel was tracked in both images
                points1.push_back(cv::KeyPoint(x0, y0, 8.0));
                points2.push_back(cv::KeyPoint(x1, y1, 8.0));
            }
        }
    }

    // Estimate for N images - common pixels through optical flow
    void OpticalFlowEstimator::EstimateCorrespondingPixels(const std::vector<cv::Mat>& images, std::vector<std::vector<cv::KeyPoint>>& trackedPoints, cv::InputArray mask)
    {
        // need at least 2 images
        if (images.size() < 2) {
            std::cerr << "\nWarning: Optical Flow Estimator requires at least 2 images!" << std::endl;
            return;
        }
        
        // convert all images to greyscale
        std::vector<cv::Mat> greyScaleImages;
        for (const cv::Mat& image : images) {
            cv::Mat greyScaleImage;
            cv::cvtColor(image, greyScaleImage, cv::COLOR_BGR2GRAY);
            greyScaleImages.push_back(greyScaleImage);
        }
        
        // store cols for later access and for bounds checking
        int rows = images[0].rows;
        int cols = images[0].cols;
        
        // create the keypoint MxN matrix
        // M: number of images
        // N: number of pixels (must match in each image)
        size_t M = images.size();
        size_t N = rows * cols;
        cv::KeyPoint defaultValue { 0.0, 0.0, 1.0 };
        
        // prepare data: keypoint matrix, and flow image
        std::vector<std::vector<cv::KeyPoint>> keypoints(M, std::vector<cv::KeyPoint>(N, defaultValue));
        cv::Mat flow;
        
        // first image keypoints: (x,y) coordinates of image
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                keypoints[0][row * cols + col].pt.x = col;
                keypoints[0][row * cols + col].pt.y = row;
            }
        }
        
        // invalid pixel indices
        std::unordered_set<size_t> outOfBoundKeyPointIndices;
        
        // determine if mask is being applied
        bool isMasked = (&mask != &cv::noArray());
        if (isMasked)
        {
            cv::Mat maskImage = mask.getMat();
            
            // add indices for all pixels in mask to out of bounds indices
            for (int row = 0; row < maskImage.rows; row++)
            {
                for (int col = 0; col < maskImage.cols; col++)
                {
                    int value = static_cast<int>(maskImage.at<unsigned char>(row, col));
                    if (value == 0) {
                        size_t index = row * cols + col;
                        outOfBoundKeyPointIndices.insert(index);
                    }
                }
            }
        }
        
        // optical flow of first 2 images
        m_FarnebackOF->calc(greyScaleImages[0], greyScaleImages[1], flow);
        size_t index = 0;
        for (int row = 0; row < rows; row++)
        {
            for (int col = 0; col < cols; col++)
            {
                cv::Vec2f delta = flow.at<cv::Vec2f>(row, col);
                index = row * cols + col;
                
                // only access valid pixels
                if (outOfBoundKeyPointIndices.find(index) == outOfBoundKeyPointIndices.end())
                {
                    keypoints[1][index].pt.x = keypoints[0][index].pt.x + delta[0];
                    keypoints[1][index].pt.y = keypoints[0][index].pt.y + delta[1];
                    
                    // make sure not out of bounds
                    if (keypoints[1][index].pt.x < 0 || keypoints[1][index].pt.x >= cols) {
                        outOfBoundKeyPointIndices.insert(index);
                    }
                    else if (keypoints[1][index].pt.y < 0 || keypoints[1][index].pt.y >= rows) {
                        outOfBoundKeyPointIndices.insert(index);
                    }
                }
            }
        }
        
        // if more images, process iteratively while tracking pixel movement across remaining images
        if (greyScaleImages.size() > 2)
        {
            for (size_t m = 2; m < M; m++)
            {
                // optical flow on image (m - 1) and m
                m_FarnebackOF->calc(greyScaleImages[m - 1], greyScaleImages[m], flow);
                for (size_t index = 0; index < N; index++)
                {
                    // only process valid pixel
                    if (outOfBoundKeyPointIndices.find(index) == outOfBoundKeyPointIndices.end())
                    {
                        int row = keypoints[m - 1][index].pt.y;
                        int col = keypoints[m - 1][index].pt.x;
                        
                        cv::Vec2f delta = flow.at<cv::Vec2f>(row, col);
                        
                        keypoints[m][index].pt.x = keypoints[m - 1][index].pt.x + delta[0];
                        keypoints[m][index].pt.y = keypoints[m - 1][index].pt.y + delta[1];
                        
                        // make sure not out of bounds
                        if (keypoints[m][index].pt.x < 0 || keypoints[m][index].pt.x >= cols) {
                            outOfBoundKeyPointIndices.insert(index);
                        }
                        else if (keypoints[m][index].pt.y < 0 || keypoints[m][index].pt.y >= rows) {
                            outOfBoundKeyPointIndices.insert(index);
                        }
                    }
                }
            }
        }
        
        // allocate space for tracked points
        for (size_t m = 0; m < M; m++) {
            trackedPoints.push_back(std::vector<cv::KeyPoint>());
        }
        
        // filter out bad pixels
        for (size_t n = 0; n < N; n++)
        {
            if (outOfBoundKeyPointIndices.find(n) == outOfBoundKeyPointIndices.end())
            {
                for (size_t m = 0; m < M; m++) {
                    trackedPoints[m].push_back(keypoints[m][n]);
                }
            }
        }
        
        std::cout << "\nTracking complete. " << trackedPoints[0].size() << " common points matched." << std::endl;
    }
}
