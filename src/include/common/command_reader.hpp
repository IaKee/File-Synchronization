#pragma once

#include <vector>
#include <string>
#include <functional>
#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include "constants.hpp"
#include "utils.hpp"

namespace command_reader
{
    class CommandReader 
    {
        public:
            CommandReader(
                boost::asio::io_context& io_context, 
                const std::vector<std::pair<std::string, std::function<void(const std::string&)>>>& commands)
                    : io_context_(io_context),
                    input_descriptor_(io_context, ::dup(STDIN_FILENO)),
                    input_buffer_(),
                    commands_(commands) {}

            void start();

            void stop();

        private:
            void handle_user_input(const boost::system::error_code& error);

            boost::asio::io_context& io_context_;
            boost::asio::posix::stream_descriptor input_descriptor_;
            boost::asio::streambuf input_buffer_;
            std::vector<std::pair<std::string, std::function<void(const std::string&)>>> commands_;
    };
}  // namespace command_reader
