#include "chrome/browser/privacy/third_party_classifier/third_party_classifier.h"

#include <algorithm>
#include <vector>
#include <unordered_set>

namespace lean_thorium {
namespace privacy {

namespace {

// Known two-part public suffixes for lightweight deterministic extraction
const std::unordered_set<std::string>& GetTwoPartTlds() {
  static const std::unordered_set<std::string> kTwoPartTlds = {
      "co.uk", "org.uk", "gov.uk", "ac.uk", "me.uk", "net.uk",
      "com.au", "net.au", "org.au", "edu.au", "gov.au",
      "co.jp", "ne.jp", "ac.jp", "go.jp", "or.jp",
      "co.nz", "net.nz", "org.nz", "govt.nz",
      "co.za", "org.za", "net.za", "gov.za",
      "com.br", "org.br", "net.br", "gov.br",
      "com.cn", "net.cn", "org.cn", "gov.cn",
      "co.in", "net.in", "org.in", "gen.in", "firm.in", "ind.in",
      "co.kr", "ne.kr", "or.kr", "re.kr",
      "gc.ca", "on.ca", "qc.ca",
  };
  return kTwoPartTlds;
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

std::string ThirdPartyClassifier::ExtractHostname(const std::string& url_str) {
  if (url_str.empty()) return "";

  size_t scheme_pos = url_str.find("://");
  size_t start = (scheme_pos != std::string::npos) ? scheme_pos + 3 : 0;

  size_t end = url_str.find_first_of("/?#:", start);
  if (end == std::string::npos) {
    end = url_str.length();
  }

  std::string host = url_str.substr(start, end - start);
  return ToLower(host);
}

std::string ThirdPartyClassifier::GetDomain(const std::string& hostname) {
  std::string lower_host = ToLower(hostname);
  if (lower_host.empty()) return "";

  // Strip trailing dot if present
  if (lower_host.back() == '.') {
    lower_host.pop_back();
  }

  // Handle IP addresses or localhost
  if (lower_host == "localhost" || lower_host == "127.0.0.1" || lower_host == "::1") {
    return lower_host;
  }

  std::vector<std::string> parts = Split(lower_host, '.');
  if (parts.size() <= 2) {
    return lower_host;
  }

  // Check for two-part TLD (e.g., example.co.uk)
  std::string potential_two_part = parts[parts.size() - 2] + "." + parts[parts.size() - 1];
  if (GetTwoPartTlds().count(potential_two_part) > 0) {
    if (parts.size() >= 3) {
      return parts[parts.size() - 3] + "." + potential_two_part;
    }
    return lower_host;
  }

  // Standard 1-part TLD (e.g., sub.example.com -> example.com)
  return parts[parts.size() - 2] + "." + parts[parts.size() - 1];
}

PartyContext ThirdPartyClassifier::Classify(const std::string& target_url_str,
                                            const std::string& initiator_origin_str) {
  if (initiator_origin_str.empty()) {
    // Top-level navigation without initiator is treated as first-party
    return PartyContext::kFirstParty;
  }

  std::string target_host = ExtractHostname(target_url_str);
  std::string init_host = ExtractHostname(initiator_origin_str);

  if (target_host.empty() || init_host.empty()) {
    return PartyContext::kFirstParty;
  }

  // Exact host match is definitely first-party
  if (target_host == init_host) {
    return PartyContext::kFirstParty;
  }

  std::string target_domain = GetDomain(target_host);
  std::string init_domain = GetDomain(init_host);

  if (target_domain == init_domain) {
    return PartyContext::kSameSite;
  }

  return PartyContext::kThirdParty;
}

}  // namespace privacy
}  // namespace lean_thorium