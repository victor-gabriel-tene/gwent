#ifndef LAB_M1_TEMA1_RULES_REFERENCE_H_
#define LAB_M1_TEMA1_RULES_REFERENCE_H_

#include <string>
#include <vector>

namespace tema1 {
namespace text {

struct RuleSection {
  const char* heading;
  std::vector<std::string> lines;
};

const std::vector<RuleSection>& RuleSections();

}  // namespace text
}  // namespace tema1

#endif  // LAB_M1_TEMA1_RULES_REFERENCE_H_
