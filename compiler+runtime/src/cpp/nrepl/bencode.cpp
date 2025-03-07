#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <stdexcept>
#include <cassert>
#include "bencode.h"

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

void writeBencode(BencodeValuePtr const &value, std::ostream &output)
{
  if(std::holds_alternative<std::string>(*value))
  {
    std::string const &str = std::get<std::string>(*value);
    output << str.size() << ':' << str;
  }
  else if(std::holds_alternative<int>(*value))
  {
    output << 'i' << std::get<int>(*value) << 'e';
  }
  else if(std::holds_alternative<std::vector<BencodeValuePtr>>(*value))
  {
    output << 'l';
    for(auto const &item : std::get<std::vector<BencodeValuePtr>>(*value))
    {
      writeBencode(item, output);
    }
    output << 'e';
  }
  else if(std::holds_alternative<std::map<std::string, BencodeValuePtr>>(*value))
  {
    output << 'd';
    for(auto const &[key, val] : std::get<std::map<std::string, BencodeValuePtr>>(*value))
    {
      writeBencode(std::make_shared<BencodeValue>(key), output);
      writeBencode(val, output);
    }
    output << 'e';
  }
}

BencodeValuePtr readBencode(std::istream &input)
{
  char c = input.peek();
  if(isdigit(c))
  {
    int length;
    input >> length;
    input.get(); // consume ':'
    std::string str(length, ' ');
    input.read(&str[0], length);
    return std::make_shared<BencodeValue>(str);
  }
  else if(c == 'i')
  {
    input.get(); // consume 'i'
    int number;
    input >> number;
    input.get(); // consume 'e'
    return std::make_shared<BencodeValue>(number);
  }
  else if(c == 'l')
  {
    input.get(); // consume 'l'
    std::vector<BencodeValuePtr> list;
    while(input.peek() != 'e')
    {
      list.push_back(readBencode(input));
    }
    input.get(); // consume 'e'
    return std::make_shared<BencodeValue>(list);
  }
  else if(c == 'd')
  {
    input.get(); // consume 'd'
    std::map<std::string, BencodeValuePtr> dict;
    while(input.peek() != 'e')
    {
      std::string key = std::get<std::string>(*readBencode(input));
      BencodeValuePtr value = readBencode(input);
      dict[key] = value;
    }
    input.get(); // consume 'e'
    return std::make_shared<BencodeValue>(dict);
  }
  else
  {
    throw std::runtime_error("Invalid Bencode format");
  }
}

void printBencode(BencodeValuePtr const &value)
{
  std::ostringstream outputStream;
  writeBencode(value, outputStream);
  std::cout << outputStream.str() << std::endl;
}

void testBencode()
{
  std::vector<std::pair<BencodeValuePtr, std::string>> testCases = {
    { std::make_shared<BencodeValue>(""), "0:" },
    { std::make_shared<BencodeValue>("Hello, World!"), "13:Hello, World!" },
    { std::make_shared<BencodeValue>("Hällö, Würld!"), "16:Hällö, Würld!" },
    { std::make_shared<BencodeValue>("Здравей, Свят!"), "25:Здравей, Свят!" },
    { std::make_shared<BencodeValue>(0), "i0e" },
    { std::make_shared<BencodeValue>(42), "i42e" },
    { std::make_shared<BencodeValue>(-42), "i-42e" },
    { std::make_shared<BencodeValue>(std::vector<BencodeValuePtr>{}), "le" },
    { std::make_shared<BencodeValue>(
        std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("cheese") }),
     "l6:cheesee" },
    { std::make_shared<BencodeValue>(
        std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("cheese"),
                                      std::make_shared<BencodeValue>("ham"),
                                      std::make_shared<BencodeValue>("eggs") }),
     "l6:cheese3:ham4:eggse" },
    { std::make_shared<BencodeValue>(std::map<std::string, BencodeValuePtr>{}), "de" },
    { std::make_shared<BencodeValue>(std::map<std::string, BencodeValuePtr>{
        { "ham", std::make_shared<BencodeValue>("eggs") } }),
     "d3:ham4:eggse" },
    { std::make_shared<BencodeValue>(std::vector<BencodeValuePtr>{
        std::make_shared<BencodeValue>("cheese"),
        std::make_shared<BencodeValue>(42),
        std::make_shared<BencodeValue>(std::map<std::string, BencodeValuePtr>{
          { "ham", std::make_shared<BencodeValue>("eggs") } }) }),
     "l6:cheesei42ed3:ham4:eggsee" },
    { std::make_shared<BencodeValue>(std::map<std::string, BencodeValuePtr>{
        { "cheese", std::make_shared<BencodeValue>(42) },
        { "ham",
          std::make_shared<BencodeValue>(
            std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("eggs") }) } }),
     "d6:cheesei42e3:haml4:eggsee" }
  };

  for(auto const &[value, bencode] : testCases)
  {
    std::ostringstream encodedStream;
    writeBencode(value, encodedStream);
    std::string encoded = encodedStream.str();

    std::istringstream input(bencode);
    BencodeValuePtr decoded = readBencode(input);

    std::ostringstream decodedStream;
    writeBencode(decoded, decodedStream);
    std::string decodedStr = decodedStream.str();

    std::cout << "Original: " << bencode << "\nEncoded: " << encoded << "\nDecoded: " << decodedStr
              << "\n\n";

    assert(encoded == bencode);
    assert(decodedStr == bencode);
  }
  std::cout << "All tests passed!" << std::endl;
}
