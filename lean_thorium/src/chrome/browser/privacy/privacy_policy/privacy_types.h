#ifndef CHROME_BROWSER_PRIVACY_PRIVACY_POLICY_PRIVACY_TYPES_H_
#define CHROME_BROWSER_PRIVACY_PRIVACY_POLICY_PRIVACY_TYPES_H_

#include <string>

namespace lean_thorium {
namespace privacy {

// Deterministic decision outcome for a network request
enum class PrivacyDecision {
  kAllow = 0,
  kBlock = 1,
};

// Classification of the relationship between target request and document initiator
enum class PartyContext {
  kFirstParty = 0,
  kSameSite = 1,
  kThirdParty = 2,
};

// Classification of the target endpoint from privacy rule analysis
enum class TrackerClassification {
  kNotTracker = 0,
  kKnownTracker = 1,
  kUnclassified = 2,
};

// Resource type of the evaluated network request
enum class ResourceType {
  kMainFrame = 0,
  kSubFrame = 1,
  kScript = 2,
  kImage = 3,
  kStylesheet = 4,
  kFont = 5,
  kXhrFetch = 6,
  kPingBeacon = 7,
  kWebSocket = 8,
  kMedia = 9,
  kOther = 10,
};

// Rule match metadata (independent from tracker classification)
struct RuleMatch {
  bool matched = false;
  std::string rule_id;
  std::string pattern;
  bool is_allow_rule = false;
};

// Result of evaluating a request against privacy policies and rule sets
struct PrivacyEvaluationResult {
  PrivacyDecision decision = PrivacyDecision::kAllow;
  PartyContext party_context = PartyContext::kFirstParty;
  TrackerClassification classification = TrackerClassification::kNotTracker;
  RuleMatch matched_rule;
  std::string reason;
  bool exception_applied = false;
};

// Helper string converters for logging and telemetry
inline const char* PrivacyDecisionToString(PrivacyDecision decision) {
  switch (decision) {
    case PrivacyDecision::kAllow:
      return "ALLOW";
    case PrivacyDecision::kBlock:
      return "BLOCK";
  }
  return "UNKNOWN";
}

inline const char* PartyContextToString(PartyContext context) {
  switch (context) {
    case PartyContext::kFirstParty:
      return "FIRST_PARTY";
    case PartyContext::kSameSite:
      return "SAME_SITE";
    case PartyContext::kThirdParty:
      return "THIRD_PARTY";
  }
  return "UNKNOWN";
}

inline const char* TrackerClassificationToString(TrackerClassification classification) {
  switch (classification) {
    case TrackerClassification::kNotTracker:
      return "NOT_TRACKER";
    case TrackerClassification::kKnownTracker:
      return "KNOWN_TRACKER";
    case TrackerClassification::kUnclassified:
      return "UNCLASSIFIED";
  }
  return "UNKNOWN";
}

inline const char* ResourceTypeToString(ResourceType type) {
  switch (type) {
    case ResourceType::kMainFrame:
      return "main_frame";
    case ResourceType::kSubFrame:
      return "sub_frame";
    case ResourceType::kScript:
      return "script";
    case ResourceType::kImage:
      return "image";
    case ResourceType::kStylesheet:
      return "stylesheet";
    case ResourceType::kFont:
      return "font";
    case ResourceType::kXhrFetch:
      return "xhr_fetch";
    case ResourceType::kPingBeacon:
      return "ping_beacon";
    case ResourceType::kWebSocket:
      return "websocket";
    case ResourceType::kMedia:
      return "media";
    case ResourceType::kOther:
      return "other";
  }
  return "other";
}

}  // namespace privacy
}  // namespace lean_thorium

#endif  // CHROME_BROWSER_PRIVACY_PRIVACY_POLICY_PRIVACY_TYPES_H_