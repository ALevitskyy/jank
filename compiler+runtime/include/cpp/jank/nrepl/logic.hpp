#ifndef JANK_NREPL_LOGIC_HPP
#define JANK_NREPL_LOGIC_HPP

#include <iostream>
#include <boost/filesystem.hpp>

namespace jank
{
  void handler(std::istream &input, std::ostream &output, boost::filesystem::path const &path);
  std::string evaluate_code(std::string const &line, boost::filesystem::path const &path);
}

#endif // JANK_NREPL_LOGIC_HPP
