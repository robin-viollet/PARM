#ifndef AVENGERS_EXCEPTION_HPP
#define AVENGERS_EXCEPTION_HPP

#include <exception>
#include <string>

class exception : public std::exception {
public:
    explicit exception(std::string message);

    [[nodiscard]] const char* what() const noexcept override;

protected:
    std::string message;
};

#endif //AVENGERS_EXCEPTION_HPP