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

#include "StereoStream.hpp"
#include "message/StereoStreamMessages.hpp"

namespace CVNetwork
{
    namespace Servers
    {
        class ReconstructionServer
        {
        public:
            /// Create an instance of the server with the given address
            /// \param ip The IPv4 address. Default is localhost.
            /// \param port The port that the server will listen on. Default is 7000.
            ReconstructionServer(const std::string& ip = "localhost", int port = 7000);

            ~ReconstructionServer();

            /// Start listening for a connection
            void StartServer();

        private:
            const std::string m_IP;
            int m_Port;
            bool m_IsRunning { false };

            StereoStream m_StereoStream;

            std::queue<Message::StereoMessage> m_DataQueue;
            std::thread m_Thread;
        };
    }
}

#endif //NETWORK_PROTOCOL_RECONSTRUCTIONSERVER_HPP