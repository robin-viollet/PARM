#include "exception.hpp"

#include <utility>

exception::exception(std::string _message){

    message = std::move(_message);

}

const char* exception::what() const noexcept {

    return message.c_str();

}
