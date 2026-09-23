#include "testing/gtest/include/gtest/gtest.h"
#include "chrome/browser/cognitia_adapter/protocol/observation_envelope.h"
#include "chrome/browser/cognitia_adapter/protocol/url_sanitizer.h"
#include "chrome/browser/cognitia_adapter/transport/bounded_event_queue.h"
#include "chrome/browser/cognitia_adapter/observation/observation_types.h"

namespace cognitia {

TEST(CognitiaUrlSanitizerTest, StripsCredentialsAndSensitiveTokens) {
  // Test 1: Strip basic embedded user:pass credentials
  GURL raw_url("https://alice:secretpassword123@example.com/dashboard");
  std::string sanitized = UrlSanitizer::SanitizeUrl(raw_url, UrlExposurePolicy::kFullUrlSanitized);
  EXPECT_EQ(sanitized, "https://example.com/dashboard");

  // Test 2: Redact sensitive query parameters
  GURL query_url("https://example.com/api?token=secret123&user=bob&session_id=sess998");
  std::string sanitized_query = UrlSanitizer::SanitizeUrl(query_url, UrlExposurePolicy::kFullUrlSanitized);
  EXPECT_EQ(sanitized_query, "https://example.com/api?token=[REDACTED]&user=bob&session_id=[REDACTED]");

  // Test 3: Origin Only policy
  std::string origin_only = UrlSanitizer::SanitizeUrl(query_url, UrlExposurePolicy::kOriginOnly);
  EXPECT_EQ(origin_only, "https://example.com/");

  // Test 4: Omitted policy
  std::string omitted = UrlSanitizer::SanitizeUrl(query_url, UrlExposurePolicy::kOmitted);
  EXPECT_TRUE(omitted.empty());
}

TEST(CognitiaObservationEnvelopeTest, GeneratesValidAbiCompliantJson) {
  ObservationEnvelope envelope(
      BrowserEventType::kNavigationCommitted,
      1, 402, true,
      "https://example.com/research",
      "https://example.com",
      u"Research Page",
      false);

  EXPECT_EQ(envelope.schema_version(), "1.0.0");
  EXPECT_FALSE(envelope.id().empty());

  std::string json = envelope.ToJson();
  EXPECT_NE(json.find("\"schema_version\":\"1.0.0\""), std::string::npos);
  EXPECT_NE(json.find("\"source_id\":\"thorium_browser_adapter\""), std::string::npos);
  EXPECT_NE(json.find("\"type\":\"navigation_committed\""), std::string::npos);
  EXPECT_NE(json.find("\"url\":\"https://example.com/research\""), std::string::npos);
}

TEST(CognitiaBoundedEventQueueTest, EnforcesCapacityAndDropsOldest) {
  BoundedEventQueue queue(3);

  queue.Enqueue("event_1");
  queue.Enqueue("event_2");
  queue.Enqueue("event_3");
  EXPECT_EQ(queue.Size(), 3u);

  // Overflow: enqueue 4th item; event_1 should be dropped
  queue.Enqueue("event_4");
  EXPECT_EQ(queue.Size(), 3u);
  EXPECT_EQ(queue.dropped_count(), 1u);

  std::string item;
  EXPECT_TRUE(queue.Dequeue(&item));
  EXPECT_EQ(item, "event_2");

  EXPECT_TRUE(queue.Dequeue(&item));
  EXPECT_EQ(item, "event_3");

  EXPECT_TRUE(queue.Dequeue(&item));
  EXPECT_EQ(item, "event_4");

  EXPECT_FALSE(queue.Dequeue(&item));
  EXPECT_TRUE(queue.IsEmpty());
}

}  // namespace cognitia
