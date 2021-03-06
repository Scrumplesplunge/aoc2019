import "util/check.h";
import <charconv>;  // bug
import <iostream>;
import <map>;
import <numeric>;
import <string_view>;
import util.io;

struct reaction {
  std::string_view output_type;
  int output_quantity;
  std::map<std::string_view, int> requirements;
  // The stage is the longest dependency sequence between this element and ORE.
  // It is 1 for ORE itself, 2 for something immediately made from ORE, etc.
  int stage = 0;
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

// Populate reactions[name].stage and return the result.
int calculate_stage(std::map<std::string_view, reaction>& reactions,
                    std::string_view name) {
  auto& reaction = reactions.at(name);
  if (!reaction.stage) {
    reaction.stage =
        1 + std::transform_reduce(
                std::begin(reaction.requirements),
                std::end(reaction.requirements), 0,
                [](int a, int b) { return std::max(a, b); },
                [&](auto& x) { return calculate_stage(reactions, x.first); });
  }
  return reaction.stage;
}

constexpr std::int64_t ceil_div(std::int64_t a, std::int64_t b) {
  return (a + b - 1) / b;
}

// Calculate the minimum amount of ore needed to make the given amount of fuel.
std::int64_t ore(const std::map<std::string_view, reaction>& reactions,
                 std::int64_t fuel) {
  std::map<std::string_view, std::int64_t> required = {{"FUEL", fuel}};
  // Start at FUEL and work backwards in stage order. By doing it according to
  // stage we cover the case where one dependent of a given reaction can be
  // consumed as part of the construction of another dependent.
  for (int i = reactions.at("FUEL").stage; i > 1; i--) {
    std::map<std::string_view, std::int64_t> new_required;
    for (const auto& [output_type, output_quantity] : required) {
      const auto& reaction = reactions.at(output_type);
      if (reaction.stage == i) {
        auto repetitions = ceil_div(output_quantity, reaction.output_quantity);
        for (const auto& [type, amount] : reaction.requirements) {
          new_required[type] += repetitions * amount;
        }
      } else {
        new_required[output_type] += output_quantity;
      }
    }
    std::swap(required, new_required);
  }
  return required.at("ORE");
}

// Binary search for the maximum amount of fuel that can be made from the
// supplied amount of ore.
std::int64_t max_fuel(const std::map<std::string_view, reaction>& reactions,
                      std::int64_t supplied_ore) {
  std::int64_t a = 0, b = 10'000'000'000'000;
  check(ore(reactions, a) < supplied_ore);
  check(ore(reactions, b) > supplied_ore);
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
  reactions["ORE"].stage = 1;
  while (!scanner.done()) {
    reaction reaction;
    (scanner >> reaction).check_ok();
    reactions.emplace(reaction.output_type, std::move(reaction));
  }

  // Compute the stage for each reaction.
  for (auto& [name, reaction] : reactions) calculate_stage(reactions, name);

  std::cout << "part1 " << ore(reactions, 1) << '\n';
  std::cout << "part2 " << max_fuel(reactions, 1'000'000'000'000) << '\n';
}
