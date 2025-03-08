#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <jank/nrepl/bencode.hpp>
#include <jank/runtime/context.hpp>
#include <jank/error/report.hpp>
#include <fmt/format.h>

#include <jank/nrepl/logic.hpp>

namespace jank
{

  void handler(std::istream &input, std::ostream &output, boost::filesystem::path const &path)
  {
    BencodeValuePtr decoded = readBencode(input);

    std::cout << "Recieved payload: ";
    printReadableBencode(decoded, std::cout);
    std::cout << "\n";
    std::cout << "\n";
    std::cout << "\n";

    if(std::holds_alternative<std::map<std::string, BencodeValuePtr>>(*decoded))
    {
      auto const &map = std::get<std::map<std::string, BencodeValuePtr>>(*decoded);
      auto it = map.find("op");
      if(it != map.end() && std::holds_alternative<std::string>(*it->second))
      {
        std::string op = std::get<std::string>(*it->second);
        if(op == "eval")
        {
          auto code_it = map.find("code");
          if(code_it != map.end() && std::holds_alternative<std::string>(*code_it->second))
          {
            std::string code = std::get<std::string>(*code_it->second);
            boost::trim(code);
            std::string result = evaluate_code(code, path);
            BencodeValuePtr encoded = std::make_shared<BencodeValue>(result);
            writeBencode(encoded, output);
          }
          else
          {
            BencodeValuePtr encoded = std::make_shared<BencodeValue>("incorrect format");
            writeBencode(encoded, output);
          }
        }
        else
        {
          BencodeValuePtr encoded
            = std::make_shared<BencodeValue>("only eval key is supported for now");
          writeBencode(encoded, output);
        }
      }
      else
      {
        BencodeValuePtr encoded = std::make_shared<BencodeValue>("incorrect format");
        writeBencode(encoded, output);
      }
    }
    else
    {
      BencodeValuePtr encoded = std::make_shared<BencodeValue>("incorrect format");
      writeBencode(encoded, output);
    }
  }
}
