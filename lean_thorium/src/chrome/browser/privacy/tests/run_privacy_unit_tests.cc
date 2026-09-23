#include <iostream>

namespace lean_thorium {
namespace privacy {
  void TestRuleParser();
  void TestRuleDatabase();
  void TestThirdPartyClassifier();
  void TestTrackingParameterFilter();
  void TestPrivacyPolicyManager();
}
}

int main() {
  std::cout << "===============================================================" << std::endl;
  std::cout << "   LEAN THORIUM PRIVACY P1.1 - C++ NATIVE TEST RUNNER          " << std::endl;
  std::cout << "===============================================================" << std::endl;

  try {
    lean_thorium::privacy::TestRuleParser();
    lean_thorium::privacy::TestRuleDatabase();
    lean_thorium::privacy::TestThirdPartyClassifier();
    lean_thorium::privacy::TestTrackingParameterFilter();
    lean_thorium::privacy::TestPrivacyPolicyManager();

    std::cout << std::endl;
    std::cout << "===============================================================" << std::endl;
    std::cout << "   ALL C++ UNIT TESTS COMPLETED SUCCESSFULLY (100% PASS)       " << std::endl;
    std::cout << "===============================================================" << std::endl;
    return 0;
  } catch (const std::exception& e) {
    std::cerr << "Test failed with exception: " << e.what() << std::endl;
    return 1;
  }
}