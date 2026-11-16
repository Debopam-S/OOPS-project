#pragma once
#include <stdexcept>
#include <string>

// Base class for every error raised by the blood bank.
class BankException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class InvalidInputException : public BankException {
public:
    explicit InvalidInputException(const std::string& m) : BankException("Invalid input: " + m) {}
};

class RecordNotFoundException : public BankException {
public:
    explicit RecordNotFoundException(const std::string& m) : BankException("Not found: " + m) {}
};

// Not enough compatible blood in stock for a request.
class ShortageException : public BankException {
    std::string group_;
public:
    ShortageException(const std::string& group, int needed, int available)
        : BankException("Shortage: need " + std::to_string(needed) + " unit(s) for group " + group +
                        ", only " + std::to_string(available) + " compatible unit(s) in stock"),
          group_(group) {}
    const std::string& group() const { return group_; }
};
