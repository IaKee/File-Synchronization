#include "command_reader.hpp"
#include "utils.hpp"
#include "constants.hpp"

using namespace command_reader;

void CommandReader::start()
{
    boost::asio::async_read_until(
        input_descriptor_,
        input_buffer_,
        '\n',
        [this](const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
        {
            handle_user_input(error);
        });
}

void CommandReader::stop()
{
    boost::asio::post(io_context_, [this]()
                        {
                            input_buffer_.consume(input_buffer_.size());
                        });
}

void CommandReader::handle_user_input(const boost::system::error_code& error)
{
    if (!error)
    {
        std::istream input(&input_buffer_);
        std::string user_input;
        std::getline(input, user_input);

        // Encerra o programa se o usuário digitar "exit"
        if (user_input == "exit")
        {
            insert_prefix();
            std::cout << EXIT_MESSAGE << std::endl;
            return;
        }

        // Processa o comando com base na entrada do usuário
        bool command_found = false;
        for (const auto& command : commands_)
        {
            if (user_input == command.first)
            {
                command_found = true;
                command.second(user_input); // Chama a função associada
                break;
            }
        }

        if (!command_found)
        {
            insert_prefix();
            std::cout << INVALID_COMMAND << std::endl;
            insert_prefix();
        }

        // Continua lendo a próxima linha de entrada
        start();
    }
}
