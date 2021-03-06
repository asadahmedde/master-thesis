//
// ReconstructionServer.hpp
// Server responsible for 3D reconstruction from the stereo stream from the robot
// Currently there is only single-client support
//

#ifndef NETWORK_PROTOCOL_RECONSTRUCTIONSERVER_HPP
#define NETWORK_PROTOCOL_RECONSTRUCTIONSERVER_HPP

#include <queue>
#include <thread>
#include <string>
#include <mutex>

#include "cv_networking/core/StereoStream.hpp"
#include "cv_networking/message/StereoStreamMessages.hpp"

namespace CVNetwork
{
    namespace Servers
    {
        class ReconstructionServer
        {
        public:
            /// Create an instance of the server with the given address
            /// \param port The port that the server will listen on. Default is 7000.
            /// \param isCalibRequired Set to true if client should provide calib data after connecting. Default is true.
            ReconstructionServer(int port = 7000, bool isCalibRequired = true);

            ~ReconstructionServer();

            /// Start listening for a connection
            void StartServer();

            /// Shut down server
            void StopServer();

            /// Get the next stereo data message from the queue that has been received from the client.
            /// \param message Will be set with the message if true is returned
            /// \return Returns true if there was data in the queue. False if no data.
            bool GetNextStereoDataFromQueue(Message::StereoMessage& message);

            /// Get the calib message if it was received from the client. Call IsCalibAvailable() to ensure it is available.
            /// Otherwise, a default calib message will be returned
            /// \return The calib message that was received from the client
            Message::StereoCalibMessage GetCalibMessage() const;

            /// Check if the calib data has arrived
            /// \return Returns true if calib data is available
            bool IsCalibAvailable() const;

        private:
            void ServerMainThread();
            void RunMainServerLoop();
            void ProcessDataMessage(Protocol::DataMessageID dataMessageID);

        private:
            int m_Port;

            bool m_IsRunning { false };
            bool m_IsCalibAvailable { false };
            bool m_IsCalibRequired;

            StereoStream m_StereoStream;
            Message::StereoCalibMessage m_CalibMessage;

            std::queue<Message::StereoMessage> m_DataQueue;
            std::thread m_Thread;
            std::mutex m_Mutex;
        };
    }
}

#endif //NETWORK_PROTOCOL_RECONSTRUCTIONSERVER_HPP
