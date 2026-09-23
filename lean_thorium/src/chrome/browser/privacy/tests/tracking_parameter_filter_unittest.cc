#include "chrome/browser/privacy/tracking_parameter_filter/tracking_parameter_filter.h"
#include <cassert>
#include <iostream>

namespace lean_thorium {
namespace privacy {

void TestTrackingParameterFilter() {
  std::cout << "[TEST] TrackingParameterFilter: Standard UTM and Click IDs..." << std::endl;
  TrackingParameterFilter filter;
  auto r1 = filter.StripTrackingParameters("https://example.com/product?id=42&utm_source=newsletter&fbclid=abc123");
  assert(r1.parameters_removed == true);
  assert(r1.cleaned_url == "https://example.com/product?id=42");

  std::cout << "[TEST] TrackingParameterFilter: Functional parameters & fragments..." << std::endl;
  std::string input2 = "https://example.org/search?q=quantum+epistemics&page=2#section-results";
  auto r2 = filter.StripTrackingParameters(input2);
  assert(r2.parameters_removed == false);
  assert(r2.cleaned_url == input2);

  std::cout << "[TEST] TrackingParameterFilter: OAuth and Tokens..." << std::endl;
  std::string input3 = "https://auth.example.com/oauth?redirect_uri=https://app.com&state=xyz&code=abc";
  auto r3 = filter.StripTrackingParameters(input3);
  assert(r3.parameters_removed == false);
  assert(r3.cleaned_url == input3);

  std::cout << "[TEST] TrackingParameterFilter: Idempotent execution..." << std::endl;
  auto r4 = filter.StripTrackingParameters("https://example.com/?utm_source=x&id=10");
  auto r5 = filter.StripTrackingParameters(r4.cleaned_url);
  assert(r4.cleaned_url == "https://example.com/?id=10");
  assert(r5.cleaned_url == "https://example.com/?id=10");
  assert(r5.parameters_removed == false);

  std::cout << "[PASS] TrackingParameterFilter tests passed!" << std::endl;
}

}  // namespace privacy
}  // namespace lean_thorium