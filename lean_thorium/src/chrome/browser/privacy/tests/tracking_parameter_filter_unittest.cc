#include "chrome/browser/privacy/tracking_parameter_filter/tracking_parameter_filter.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace lean_thorium {
namespace privacy {

TEST(TrackingParameterFilterTest, StripsUtmParameters) {
  TrackingParameterFilter filter;
  std::string input = "https://example.com/product?id=42&utm_source=newsletter&utm_medium=email";
  auto result = filter.StripTrackingParameters(input);
  EXPECT_TRUE(result.parameters_removed);
  EXPECT_EQ(result.cleaned_url, "https://example.com/product?id=42");
  EXPECT_EQ(result.removed_parameters.size(), 2u);
}

TEST(TrackingParameterFilterTest, StripsClickIds) {
  TrackingParameterFilter filter;
  std::string input = "https://shop.example.com/items?q=shoes&gclid=12345&fbclid=abcde&p=1";
  auto result = filter.StripTrackingParameters(input);
  EXPECT_TRUE(result.parameters_removed);
  EXPECT_EQ(result.cleaned_url, "https://shop.example.com/items?q=shoes&p=1");
}

TEST(TrackingParameterFilterTest, PreservesFunctionalParametersAndFragments) {
  TrackingParameterFilter filter;
  std::string input = "https://example.org/search?q=quantum+epistemics&page=2#section-results";
  auto result = filter.StripTrackingParameters(input);
  EXPECT_FALSE(result.parameters_removed);
  EXPECT_EQ(result.cleaned_url, input);
}

TEST(TrackingParameterFilterTest, PreservesOAuthAndAuthParameters) {
  TrackingParameterFilter filter;
  std::string input = "https://auth.example.com/login?redirect_uri=https://app.example.com&state=xyz&code=secret123";
  auto result = filter.StripTrackingParameters(input);
  EXPECT_FALSE(result.parameters_removed);
  EXPECT_EQ(result.cleaned_url, input);
}

TEST(TrackingParameterFilterTest, IdempotentExecution) {
  TrackingParameterFilter filter;
  std::string input = "https://example.com/?utm_source=twitter&id=100";
  auto result1 = filter.StripTrackingParameters(input);
  auto result2 = filter.StripTrackingParameters(result1.cleaned_url);
  EXPECT_EQ(result1.cleaned_url, "https://example.com/?id=100");
  EXPECT_EQ(result2.cleaned_url, "https://example.com/?id=100");
  EXPECT_FALSE(result2.parameters_removed);
}

}  // namespace privacy
}  // namespace lean_thorium