#include "chrome/browser/privacy/tracker_blocker/rule_parser.h"

#include <algorithm>
#include <sstream>

namespace lean_thorium {
namespace privacy {

namespace {

std::string Trim(const std::string& str) {
  size_t first = str.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  size_t last = str.find_last_not_of(" \t\r\n");
  return str.substr(first, (last - first + 1));
}

std::string ToLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
    return std::tolower(c);
  });
  return str;
}

std::vector<std::string> Split(const std::string& s, char delim) {
  std::vector<std::string> tokens;
  size_t start = 0;
  size_t end = s.find(delim);
  while (end != std::string::npos) {
    tokens.push_back(s.substr(start, end - start));
    start = end + 1;
    end = s.find(delim, start);
  }
  tokens.push_back(s.substr(start));
  return tokens;
}

}  // namespace

unsigned int RuleParser::ParseResourceTypeMask(const std::string& options_str) {
  unsigned int mask = 0;
  std::vector<std::string> opts = Split(options_str, ',');
  for (const auto& opt : opts) {
    std::string o = ToLower(Trim(opt));
    if (o == "script") mask |= (1 << static_cast<int>(ResourceType::kScript));
    else if (o == "image" || o == "img") mask |= (1 << static_cast<int>(ResourceType::kImage));
    else if (o == "stylesheet" || o == "css") mask |= (1 << static_cast<int>(ResourceType::kStylesheet));
    else if (o == "font") mask |= (1 << static_cast<int>(ResourceType::kFont));
    else if (o == "xmlhttprequest" || o == "xhr" || o == "fetch") mask |= (1 << static_cast<int>(ResourceType::kXhrFetch));
    else if (o == "ping" || o == "beacon") mask |= (1 << static_cast<int>(ResourceType::kPingBeacon));
    else if (o == "websocket") mask |= (1 << static_cast<int>(ResourceType::kWebSocket));
    else if (o == "subdocument" || o == "frame") mask |= (1 << static_cast<int>(ResourceType::kSubFrame));
    else if (o == "media") mask |= (1 << static_cast<int>(ResourceType::kMedia));
  }
  return mask == 0 ? 0xFFFFFFFF : mask;
}

bool RuleParser::MatchesResourceType(ResourceType type, unsigned int mask) {
  int shift = static_cast<int>(type);
  if (shift >= 0 && shift < 32) {
    return (mask & (1 << shift)) != 0;
  }
  return true;
}

ParsedRule RuleParser::ParseLine(const std::string& raw_line, const std::string& rule_id_prefix) {
  ParsedRule rule;
  std::string line = Trim(raw_line);
  rule.original_rule_text = line;

  // 1. Skip comments and empty lines safely
  if (line.empty() || line[0] == '!' || line[0] == '#' || line.length() > 1024) {
    rule.is_valid = false;
    return rule;
  }

  // 2. Check for Allow rule (@@)
  if (line.rfind("@@", 0) == 0) {
    rule.rule_type = RuleType::kAllow;
    line = line.substr(2);
  } else {
    rule.rule_type = RuleType::kBlock;
  }

  // 3. Extract options ($)
  std::string options_str = "";
  size_t opt_pos = line.find('$');
  if (opt_pos != std::string::npos) {
    options_str = line.substr(opt_pos + 1);
    line = line.substr(0, opt_pos);
  }

  // 4. Parse options
  if (!options_str.empty()) {
    std::vector<std::string> opts = Split(options_str, ',');
    for (const auto& opt : opts) {
      std::string o = ToLower(Trim(opt));
      if (o == "third-party" || o == "3p") {
        rule.require_third_party = true;
      } else if (o == "first-party" || o == "1p") {
        rule.require_first_party = true;
      }
    }
    rule.resource_type_mask = ParseResourceTypeMask(options_str);
  }

  // 5. Parse domain pattern (||pattern^)
  if (line.rfind("||", 0) == 0) {
    line = line.substr(2);
    rule.match_subdomains = true;
  }

  // Strip trailing boundary separator '^' or '/'
  if (!line.empty() && line.back() == '^') {
    line.pop_back();
  }

  // Check if path pattern exists
  size_t slash_pos = line.find('/');
  if (slash_pos != std::string::npos) {
    rule.domain_pattern = ToLower(line.substr(0, slash_pos));
    rule.path_pattern = line.substr(slash_pos);
  } else {
    rule.domain_pattern = ToLower(line);
    rule.path_pattern = "";
  }

  if (!rule.domain_pattern.empty()) {
    rule.is_valid = true;
    rule.rule_id = rule_id_prefix + rule.domain_pattern;
  }

  return rule;
}

std::vector<ParsedRule> RuleParser::ParseText(const std::string& text_content) {
  std::vector<ParsedRule> rules;
  std::stringstream ss(text_content);
  std::string line;
  int count = 0;

  while (std::getline(ss, line)) {
    ParsedRule r = ParseLine(line, "rule_" + std::to_string(++count) + "_");
    if (r.is_valid) {
      rules.push_back(r);
    }
  }

  return rules;
}

}  // namespace privacy
}  // namespace lean_thorium