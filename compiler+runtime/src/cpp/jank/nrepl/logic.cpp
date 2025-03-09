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

  std::string get_id(std::map<std::string, BencodeValuePtr> const &map)
  {
    auto id_it = map.find("id");
    if(id_it != map.end() && std::holds_alternative<std::string>(*id_it->second))
    {
      return std::get<std::string>(*id_it->second);
    }
    return "unknown";
  }

  std::string get_session()
  {
    if(session_ids.empty())
    {
      boost::uuids::uuid uuid = boost::uuids::random_generator()();
      session_ids.push_back(boost::uuids::to_string(uuid));
    }
    return session_ids.back();
  }

  std::map<std::string, BencodeValuePtr>
  create_echo_response(std::map<std::string, BencodeValuePtr> const &map)
  {
    std::string id = get_id(map);
    std::string session = get_session();

    std::map<std::string, BencodeValuePtr> response;
    response["id"] = std::make_shared<BencodeValue>(id);
    response["session"] = std::make_shared<BencodeValue>(session);

    return response;
  }

  void send_response(std::ostream &output, std::map<std::string, BencodeValuePtr> const &response)
  {
    BencodeValuePtr encoded = std::make_shared<BencodeValue>(response);

    std::cout << "Sending back: ";
    printReadableBencode(encoded, std::cout);
    std::cout << "\n\n\n";

    writeBencode(encoded, output);
  }

  void handle_eval(std::map<std::string, BencodeValuePtr> const &map,
                   std::ostream &output,
                   boost::filesystem::path const &path)
  {
    auto code_it = map.find("code");
    if(code_it != map.end() && std::holds_alternative<std::string>(*code_it->second))
    {
      std::map<std::string, BencodeValuePtr> response = create_echo_response(map);
      std::string code = std::get<std::string>(*code_it->second);
      boost::trim(code);
      std::string result = evaluate_code(code, path);

      response["value"] = std::make_shared<BencodeValue>(result);
      response["ns"] = std::make_shared<BencodeValue>("user");

      send_response(output, response);

      std::map<std::string, BencodeValuePtr> follow_up_response = create_echo_response(map);
      follow_up_response["status"] = std::make_shared<BencodeValue>(
        std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("done") });

      send_response(output, follow_up_response);
    }
    else
    {
      BencodeValuePtr encoded = std::make_shared<BencodeValue>("incorrect format");
      writeBencode(encoded, output);
    }
  }

  void handle_clone(std::map<std::string, BencodeValuePtr> const &map, std::ostream &output)
  {
    std::map<std::string, BencodeValuePtr> response = create_echo_response(map);

    // Generate new UUID for new session
    boost::uuids::uuid new_uuid = boost::uuids::random_generator()();
    std::string new_session = boost::uuids::to_string(new_uuid);
    session_ids.push_back(new_session);

    std::string current_session
      = session_ids.size() > 1 ? session_ids[session_ids.size() - 2] : new_session;

    response["new-session"] = std::make_shared<BencodeValue>(new_session);
    response["session"] = std::make_shared<BencodeValue>(current_session);
    response["status"] = std::make_shared<BencodeValue>(
      std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("done") });

    send_response(output, response);
  }

  void handle_clojuredocs_refresh_cache(std::map<std::string, BencodeValuePtr> const &map,
                                        std::ostream &output)
  {
    std::map<std::string, BencodeValuePtr> response = create_echo_response(map);

    response["status"] = std::make_shared<BencodeValue>(
      std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("done"),
                                    std::make_shared<BencodeValue>("ok") });

    send_response(output, response);
  }

  void handle_info(std::map<std::string, BencodeValuePtr> const &map, std::ostream &output)
  {
    std::map<std::string, BencodeValuePtr> response = create_echo_response(map);

    response["status"] = std::make_shared<BencodeValue>(
      std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("done"),
                                    std::make_shared<BencodeValue>("no-info") });

    send_response(output, response);
  }

  void handle_init_debugger(std::map<std::string, BencodeValuePtr> const &map, std::ostream &output)
  {
    std::map<std::string, BencodeValuePtr> response = create_echo_response(map);

    response["status"] = std::make_shared<BencodeValue>(
      std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("done"),
                                    std::make_shared<BencodeValue>("ok") });

    send_response(output, response);
  }

  void handle_describe(std::map<std::string, BencodeValuePtr> const &map, std::ostream &output)
  {
    std::map<std::string, BencodeValuePtr> response = create_echo_response(map);

    response["ops"] = std::make_shared<BencodeValue>(std::map<std::string, BencodeValuePtr>{
      { "eval", std::make_shared<BencodeValue>(std::map<std::string, BencodeValuePtr>{}) }
    });
    response["status"] = std::make_shared<BencodeValue>(
      std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("done") });

    send_response(output, response);
  }

  void handler(std::istream &input, std::ostream &output, boost::filesystem::path const &path)
  {
    BencodeValuePtr decoded = readBencode(input);

    std::cout << "Received payload: ";
    printReadableBencode(decoded, std::cout);
    std::cout << "\n\n\n";

    if(std::holds_alternative<std::map<std::string, BencodeValuePtr>>(*decoded))
    {
      auto const &map = std::get<std::map<std::string, BencodeValuePtr>>(*decoded);
      auto it = map.find("op");
      if(it != map.end() && std::holds_alternative<std::string>(*it->second))
      {
        std::string op = std::get<std::string>(*it->second);
        if(op == "eval")
        {
          handle_eval(map, output, path);
        }
        else if(op == "clone")
        {
          handle_clone(map, output);
        }
        else if(op == "describe")
        {
          handle_describe(map, output);
        }
        else
        {
          std::map<std::string, BencodeValuePtr> response = create_echo_response(map);
          response["status"] = std::make_shared<BencodeValue>(
            std::vector<BencodeValuePtr>{ std::make_shared<BencodeValue>("error"),
                                          std::make_shared<BencodeValue>("unknown-op"),
                                          std::make_shared<BencodeValue>("done") });

          send_response(output, response);
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
