#include <iostream>
#include <fstream>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>
#include <jank/nrepl/bencode.hpp>
#include <jank/runtime/context.hpp>
#include <jank/error/report.hpp>
#include <fmt/format.h>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <jank/nrepl/logic.hpp>
#include <vector>

namespace jank
{
  std::vector<std::string> session_ids;

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
          auto id_it = map.find("id");
          auto code_it = map.find("code");
          if(code_it != map.end() && std::holds_alternative<std::string>(*code_it->second))
          {
            std::string id = "unknown";
            if(id_it != map.end() && std::holds_alternative<std::string>(*id_it->second))
            {
              id = std::get<std::string>(*id_it->second);
            }
            std::string code = std::get<std::string>(*code_it->second);
            boost::trim(code);
            std::string result = evaluate_code(code, path);

            // Generate UUID for session key if session_ids is empty
            if(session_ids.empty())
            {
              boost::uuids::uuid uuid = boost::uuids::random_generator()();
              session_ids.push_back(boost::uuids::to_string(uuid));
            }
            std::string session = session_ids.back();

            std::map<std::string, BencodeValuePtr> response;
            response["id"] = std::make_shared<BencodeValue>(id);
            response["value"] = std::make_shared<BencodeValue>(result);
            response["session"] = std::make_shared<BencodeValue>(session);
            response["ns"] = std::make_shared<BencodeValue>("user");

            BencodeValuePtr encoded = std::make_shared<BencodeValue>(response);

            std::cout << "Sending back: ";
            printReadableBencode(encoded, std::cout);
            std::cout << "\n";
            std::cout << "\n";
            std::cout << "\n";

            writeBencode(encoded, output);

            std::map<std::string, BencodeValuePtr> follow_up_response;
            follow_up_response["id"] = std::make_shared<BencodeValue>(id);
            follow_up_response["session"] = std::make_shared<BencodeValue>(session);
            follow_up_response["status"] = std::make_shared<BencodeValue>(
              std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("done") });

            BencodeValuePtr follow_up_encoded = std::make_shared<BencodeValue>(follow_up_response);

            writeBencode(follow_up_encoded, output);
          }
          else
          {
            BencodeValuePtr encoded = std::make_shared<BencodeValue>("incorrect format");
            writeBencode(encoded, output);
          }
        }
        else if(op == "clone")
        {
          auto id_it = map.find("id");
          std::string id = "unknown";
          if(id_it != map.end() && std::holds_alternative<std::string>(*id_it->second))
          {
            id = std::get<std::string>(*id_it->second);
          }

          // Generate new UUID for new session
          boost::uuids::uuid new_uuid = boost::uuids::random_generator()();
          std::string new_session = boost::uuids::to_string(new_uuid);
          session_ids.push_back(new_session);

          std::string current_session
            = session_ids.size() > 1 ? session_ids[session_ids.size() - 2] : new_session;

          std::map<std::string, BencodeValuePtr> response;
          response["id"] = std::make_shared<BencodeValue>(id);
          response["new-session"] = std::make_shared<BencodeValue>(new_session);
          response["session"] = std::make_shared<BencodeValue>(current_session);
          response["status"] = std::make_shared<BencodeValue>(
            std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("done") });

          BencodeValuePtr encoded = std::make_shared<BencodeValue>(response);

          std::cout << "Sending back: ";
          printReadableBencode(encoded, std::cout);
          std::cout << "\n";
          std::cout << "\n";
          std::cout << "\n";

          writeBencode(encoded, output);
        }
        else
        {
          BencodeValuePtr encoded
            = std::make_shared<BencodeValue>("only eval and clone keys are supported for now");
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
