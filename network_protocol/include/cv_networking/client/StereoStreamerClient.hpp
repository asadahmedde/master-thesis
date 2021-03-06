//
// StereoStreamerClient.hpp
// The client for streaming stereo image data and pose of the robot to the reconstruction server
//

#ifndef NETWORK_PROTOCOL_STEREOSTREAMERCLIENT_HPP
#define NETWORK_PROTOCOL_STEREOSTREAMERCLIENT_HPP

#include <queue>
#include <thread>
#include <string>
#include <mutex>

#include "cv_networking/core/StereoStream.hpp"
#include "cv_networking/message/StereoStreamMessages.hpp"

namespace CVNetwork
{
    namespace Clients
    {
        class StereoStreamerClient
        {
        public:
            /// Construct a default instance of the client with the given calibration data
            /// \param calib The calibration data
            StereoStreamerClient(Message::StereoCalibMessage calib);

            ~StereoStreamerClient();

            /// Establish a connection to the reconstruction server
            /// \param ip The IPv4 address of the server
            /// \param port The port number of the server
            /// \return Returns true if connection has been opened successfully
            bool ConnectToReconstructServer(const std::string& ip, int port);

            /// Run the client on a separate thread
            void Run();

            /// Add a stereo data message to the queue
            /// \param message The stereo data message that will be sent through the stream
            void AddStereoDataToQueue(const Message::StereoMessage& message);

        private:
            void RunThread();
            void RunStereoStreamLoop();
            Message::StereoMessage GetNextStereoMessageInQueue();

        private:
            std::queue<Message::StereoMessage> m_DataQueue;

            Message::StereoCalibMessage m_CalibMessage;
            StereoStream m_StereoStream;

            bool m_IsRunning { false };

            std::thread m_Thread;
            std::mutex m_QueueMutex;
        };
    }
}

#endif //NETWORK_PROTOCOL_STEREOSTREAMERCLIENT_HPP
