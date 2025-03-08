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

    if(std::holds_alternative<std::string>(*decoded))
    {
      std::string line = std::get<std::string>(*decoded);
      boost::trim(line);
      std::string result = evaluate_code(line, path);
      BencodeValuePtr encoded = std::make_shared<BencodeValue>(result);
      writeBencode(encoded, output);
    }
    else
    {
      std::cerr << "Received non-string bencoded value" << std::endl;
    }
  }
}
