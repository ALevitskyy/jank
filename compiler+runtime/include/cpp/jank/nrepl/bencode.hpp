#ifndef BENCODE_H
#define BENCODE_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <stdexcept>
#include <cassert>

struct BencodeValue;
using BencodeValuePtr = std::shared_ptr<BencodeValue>;

struct BencodeValue
  : std::variant<std::string,
                 int,
                 std::vector<BencodeValuePtr>,
                 std::map<std::string, BencodeValuePtr>>
{
  using variant::variant;
};

void writeBencode(BencodeValuePtr const &value, std::ostream &output);
BencodeValuePtr readBencode(std::istream &input);
void printBencode(BencodeValuePtr const &value);
void testBencode();

template <typename T>
BencodeValuePtr convertBencodeType(T const &value)
{
  return std::make_shared<BencodeValue>(value);
}


#endif // BENCODE_H
