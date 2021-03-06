//
// StereoStreamerClient.cpp
// The client for streaming stereo image data and pose of the robot to the reconstruction server
//

#include <iostream>
#include <chrono>

#include "cv_networking/client/StereoStreamerClient.hpp"

#define THREAD_WAIT_SLEEP_TIME_SECONDS 1

namespace CVNetwork
{
    namespace Clients
    {
        // Constructor
        StereoStreamerClient::StereoStreamerClient(Message::StereoCalibMessage calib) : m_CalibMessage(calib)
        {

        }

        // Destructor
        StereoStreamerClient::~StereoStreamerClient()
        {
            // close and wait for thread to join
            if (m_Thread.joinable()) {
                m_IsRunning = false;
                m_Thread.join();
            }

            // close connection in the stereo stream
            m_StereoStream.CloseConnection();
        }

        // ConnectToServer to reconstruct server
        bool StereoStreamerClient::ConnectToReconstructServer(const std::string& ip, int port)
        {
            try {
                return m_StereoStream.ConnectToServer(ip, port);
            }
            catch (boost::system::system_error& error) {
                m_StereoStream.CloseConnection();
                return false;
            }
        }

        // Run the client indefinitely until requested to close or connection closed
        void StereoStreamerClient::Run() {
            m_Thread = std::thread(&StereoStreamerClient::RunThread, this);
            m_IsRunning = true;
        }

        // Add a stereo data message to the queue
        void StereoStreamerClient::AddStereoDataToQueue(const Message::StereoMessage &message)
        {
            m_QueueMutex.lock();
            m_DataQueue.push(message);
            m_QueueMutex.unlock();
        }

        // Main thread loop
        void StereoStreamerClient::RunThread()
        {
#ifndef NDEBUG
            std::cout << "\nMain thread running..." << std::endl;
#endif

            // initiate the flow by sending server message that flow will begin
            bool isCalibRequested = m_StereoStream.InitiateStereoAndCheckIfCalibNeeded();
            if (isCalibRequested) {
                std::cout << "\nServer requested calibration data. Sending calib data..." << std::endl;
                m_StereoStream.WriteCalibData(m_CalibMessage);
            }

            // run main stereo stream loop
#ifndef NDEBUG
            std::cout << "\nRunning main stereo loop" << std::endl;
#endif

            RunStereoStreamLoop();
        }

        // Stereo stream loop for sending stereo image data to server
        void StereoStreamerClient::RunStereoStreamLoop()
        {
            while (m_IsRunning)
            {
                if (!m_DataQueue.empty())
                {
#ifndef NDEBUG
                    std::cout << "\nFound stereo data in queue. Sending to server..." << std::endl;
#endif

                    // read from queue and send any pending messages
                    Message::StereoMessage message = GetNextStereoMessageInQueue();
                    m_StereoStream.WriteStereoImageData(message);

#ifndef NDEBUG
                    std::cout << "\nStereo data sent to server." << std::endl;
#endif
                }

                // sleep so that the user of the client can add messages to queue
                std::this_thread::sleep_for(std::chrono::seconds(THREAD_WAIT_SLEEP_TIME_SECONDS));
            }
        }

        // Thread-safe get next message from queue
        Message::StereoMessage StereoStreamerClient::GetNextStereoMessageInQueue()
        {
            m_QueueMutex.lock();

            Message::StereoMessage message = m_DataQueue.front();
            m_DataQueue.pop();

            m_QueueMutex.unlock();

            return message;
        }
    }
}
