import "util/check.h";
import <charconv>;  // bug
import <iostream>;
import <numeric>;
import <optional>;  // bug
import <set>;
import <map>;
import <span>;  // bug
import <string_view>;
import <type_traits>;  // bug
import <vector>;
import util.io;

#include <cassert>

struct reaction {
  std::string_view output_type;
  int output_quantity;
  std::map<std::string_view, int> requirements;
  std::optional<int> stage;
};

auto element(std::string_view& out) {
  return sequence<is_alpha>(out, "element name");
}

scanner& operator>>(scanner& s, reaction& r) {
  r = {};
  std::string_view type;
  int quantity;
  if (!(s >> quantity >> element(type))) return s;
  r.requirements.emplace(type, quantity);
  while (s.remaining().starts_with(',')) {
    if (!(s >> exact(",") >> quantity >> element(type))) return s;
    r.requirements.emplace(type, quantity);
  }
  return s >> exact("=>") >> r.output_quantity >> element(r.output_type);
}

constexpr std::int64_t ceil_div(std::int64_t a, std::int64_t b) {
  return (a + b - 1) / b;
}

std::int64_t ore(const std::map<std::string_view, reaction>& reactions,
                 std::int64_t fuel) {
  std::map<std::string_view, std::int64_t> required = {{"FUEL", fuel}};
  for (int i = *reactions.at("FUEL").stage; i > 1; i--) {
    std::map<std::string_view, std::int64_t> new_required;
    for (const auto& [type, amount] : required) {
      if (type == "ORE") {
        new_required["ORE"] += amount;
        continue;
      }
      const auto& reaction = reactions.at(type);
      std::int64_t repetitions = ceil_div(amount, reaction.output_quantity);
      if (*reaction.stage == i) {
        for (const auto& [required_type, required_amount] :
             reaction.requirements) {
          assert(required_type == "ORE" ||
                 *reactions.at(required_type).stage < i);
          new_required[required_type] += repetitions * required_amount;
        }
      } else {
        assert(*reaction.stage < i);
        new_required[type] += amount;
      }
    }
    std::swap(required, new_required);
  }
  return required.at("ORE");
}

std::int64_t max_fuel(const std::map<std::string_view, reaction>& reactions,
                      std::int64_t supplied_ore) {
  std::int64_t a = 0, b = 10'000'000'000'000;
  while (b - a > 1) {
    auto fuel = a + (b - a) / 2;
    auto required_ore = ore(reactions, fuel);
    if (required_ore <= supplied_ore) {
      a = fuel;
    } else {
      b = fuel;
    }
  }
  return a;
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));
  std::map<std::string_view, reaction> reactions;
  while (!scanner.done()) {
    reaction reaction;
    (scanner >> reaction).check_ok();
    reactions.emplace(reaction.output_type, std::move(reaction));
  }

  bool missing_stage;
  do {
    missing_stage = false;
    for (auto& [type, reaction] : reactions) {
      if (reaction.stage) continue;
      int stage = 0;
      bool missing_requirement = false;
      for (const auto& [input, amount] : reaction.requirements) {
        if (input == "ORE") {
          stage = std::max(stage, 2);
        } else if (reactions.at(input).stage) {
          stage = std::max(stage, 1 + *reactions.at(input).stage);
        } else {
          missing_requirement = true;
          break;
        }
      }
      if (!missing_requirement) {
        missing_stage = true;
        reaction.stage = stage;
      }
    }
  } while (missing_stage);

  std::cout << "part1 " << ore(reactions, 1) << '\n';
  std::cout << "part2 " << max_fuel(reactions, 1'000'000'000'000) << '\n';
}
