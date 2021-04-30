#include "../external/json.hpp"
#include <fstream>
#include <iostream>
#include <streambuf>

using json = nlohmann::json;

namespace xn {
json load_json_file(const std::string &filepath) {
  std::ifstream t(filepath);
  std::string buf((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());
  return json::parse(buf);
}
} // namespace xn