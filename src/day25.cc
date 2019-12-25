import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import <unordered_map>;
import <map>;
import <set>;
import <vector>;
import <queue>;
import intcode;
import util.circular_buffer;
import util.io;
import util.vec2;

enum direction : char { north, east, south, west };
constexpr std::string_view direction_names[] =
    {"north", "east", "south", "west"};

constexpr auto is_direction(std::string_view name) {
  return name == "north" || name == "east" || name == "south" || name == "west";
}

scanner& operator>>(scanner& s, direction& d) {
  std::string_view temp;
  if (s >> matches<is_direction>(temp, "direction name")) {
    for (int i = north; i <= west; i++) {
      if (temp == direction_names[i]) d = (direction)i;
    }
  }
  return s;
}

struct location {
  std::string name;
  std::string description;
  std::array<bool, 4> doors = {};
  std::vector<std::string> items;
};

std::optional<location> parse_location(std::string_view input) {
  location output;
  scanner scanner(input);
  constexpr auto not_equals = +[](char c) { return c != '='; };
  std::string_view name;
  if (!(scanner >> exact("==") >> sequence<not_equals>(name, "location name")
                >> exact("==") >> exact("\n", "newline"))) {
    return std::nullopt;
  }
  while (!name.empty() && name.back() == ' ') name.remove_suffix(1);
  output.name = std::string{name};
  constexpr auto is_line = +[](char c) { return c != '\n'; };
  std::string_view description;
  if (!(scanner >> sequence<is_line>(description, "location description")
                >> exact("\n", "newline")
                >> exact("Doors here lead:\n", "list of doors")
                >> whitespace)) {
    return std::nullopt;
  }
  output.description = std::string{description};
  while (scanner.remaining().starts_with("-")) {
    direction d;
    if (!(scanner >> exact("-") >> d >> exact("\n", "newline"))) {
      return std::nullopt;
    }
    output.doors[d] = true;
  }
  scanner >> whitespace;
  if (scanner.remaining().starts_with("Items here:")) {
    if (!(scanner >> exact("Items here:\n", "list of items") >> whitespace)) {
      return std::nullopt;
    }
    while (scanner.remaining().starts_with("-")) {
      //check(output.items.size() < output.items.capacity());
      std::string_view item;
      if (!(scanner >> exact("-") >> sequence<is_line>(item, "item name")
                    >> exact("\n", "newline"))) {
        return std::nullopt;
      }
      check(item.size() < 20);
      output.items.push_back(std::string{item});
    }
  }
  if (!(scanner >> exact("Command?\n", "input prompt"))) {
    return std::nullopt;
  }
  return output;
}

struct item {
  std::string name;
  std::vector<direction> path;
};

struct world {
  std::vector<direction> path;
  location location;
  program robot;

  world(program::const_span source) : robot(source) {
    std::string state;
    while (robot.resume() == program::output) {
      state.push_back((unsigned char)robot.get_output());
    }
    check(robot.current_state() == program::waiting_for_input);
    auto location = parse_location(state);
    check(location);
    this->location = *location;
  }

  std::string execute(std::string_view command) {
    check(robot.current_state() == program::waiting_for_input);
    for (char c : command) {
      robot.provide_input(c);
      check(robot.resume() == program::waiting_for_input);
    }
    robot.provide_input('\n');
    std::string state;
    while (robot.resume() == program::output) {
      state.push_back((unsigned char)robot.get_output());
    }
    return state;
  }

  bool take(std::string_view item) {
    if (item == "infinite loop") return false;  // Halting problem? Easy.
    auto temp = *this;
    auto state = temp.execute("take " + std::string{item});
    if (temp.robot.current_state() != program::waiting_for_input) return false;
    *this = std::move(temp);
    scanner scanner(state);
    (scanner >> exact("You take the " + std::string{item} + ".")
             >> exact("\n", "newline") >> exact("Command?")
             >> scanner::end).check_ok();
    return true;
  }

  bool drop(std::string_view item) {
    if (item == "infinite loop") return false;  // Halting problem? Easy.
    auto temp = *this;
    auto state = temp.execute("drop " + std::string{item});
    if (temp.robot.current_state() != program::waiting_for_input) return false;
    *this = std::move(temp);
    scanner scanner(state);
    (scanner >> exact("You drop the " + std::string{item} + ".")
             >> exact("\n", "newline") >> exact("Command?")
             >> scanner::end).check_ok();
    return true;
  }

  bool move(direction d) {
    program temp = robot;
    check(temp.current_state() == program::waiting_for_input);
    const auto& command = direction_names[d];
    for (int i = 0, n = command.size(); i < n; i++) {
      temp.provide_input(command[i]);
      check(temp.resume() == program::waiting_for_input);
    }
    temp.provide_input('\n');
    std::string state;
    while (temp.resume() == program::output) {
      state.push_back((unsigned char)temp.get_output());
    }
    auto next = parse_location(state);
    if (!next) return false;
    robot = std::move(temp);
    location = *next;
    if (!path.empty() && path.back() == (d + 2) % 4) {
      path.pop_back();
    } else {
      path.push_back(d);
    }
    return true;
  }

  bool move(std::span<const direction> steps) {
    auto temp = *this;
    for (auto d : steps) {
      if (!temp.move(d)) return false;
    }
    *this = std::move(temp);
    return true;
  }

  bool unmove(std::span<const direction> steps) {
    auto temp = *this;
    for (int i = steps.size() - 1; i >= 0; i--) {
      if (!temp.move((direction)((steps[i] + 2) % 4))) return false;
    }
    *this = std::move(temp);
    return true;
  }

  bool collect(const item& item) {
    auto temp = *this;
    if (!temp.move(item.path)) return false;
    if (!temp.take(item.name)) return false;
    if (!temp.unmove(item.path)) return false;
    *this = std::move(temp);
    return true;
  }

  direction target_direction() {
    for (int i = north; i <= west; i++) {
      auto output = execute(direction_names[i]);
      if (scanner(output) >> exact("== Pressure-Sensitive Floor ==")) {
        return (direction)i;
      }
    }
    std::cerr << "Failed to identify target direction from security "
                 "checkpoint.\n";
    std::abort();
  }
};

struct search {
  std::map<std::string, std::vector<direction>> rooms;
  std::vector<item> items;

  void explore(world current) {
    if (!rooms.emplace(current.location.name, current.path).second) return;
    for (const auto& item : current.location.items) {
      items.push_back({item, current.path});
    }
    for (int d = north; d <= west; d++) {
      if (!current.location.doors[d]) continue;
      world next = current;
      if (!next.move((direction)d)) continue;
      explore(std::move(next));
    }
  }
};

int part1(program::const_span source) {
  search s;
  s.explore(world(source));
  std::vector<std::string_view> collectable;
  world state(source);
  // Collect all items and carry them to the security checkpoint.
  for (const auto& item : s.items) {
    if (state.collect(item)) collectable.push_back(item.name);
  }
  std::sort(collectable.begin(), collectable.end());
  const auto security_checkpoint = s.rooms.find("Security Checkpoint");
  check(security_checkpoint != s.rooms.end());
  check(state.move(security_checkpoint->second));
  const direction target = state.target_direction();
  for (const auto& item : collectable) check(state.drop(item));
  // Try all combinations of items.
  check(collectable.size() < 10);
  for (int i = 0, n = collectable.size(); i < (1 << n); i++) {
    world temp = state;
    for (int j = 0; j < n; j++) {
      if ((i >> j) & 1) check(temp.take(collectable[j]));
    }
    auto output = temp.execute(direction_names[target]);
    const auto first = output.data(), last = first + output.size();
    const auto b = std::find(std::make_reverse_iterator(last),
                             std::make_reverse_iterator(first), '\n').base();
    const auto a = std::find(std::make_reverse_iterator(b - 1),
                             std::make_reverse_iterator(first), '\n').base();
    scanner scanner(std::string_view(a, b - a));
    int password;
    if (scanner >> exact("\"Oh, hello! You should be able to get in by typing ")
                >> password >> exact(" on the keypad at the main airlock.\"")) {
      return password;
    }
  }
  std::cerr << "Failed to find password.\n";
  std::abort();
}

int main(int argc, char* argv[]) {
  program::buffer program_buffer;
  const auto source = program::load(init(argc, argv), program_buffer);

  std::cout << "part1 " << part1(source) << '\n';
  std::cout << "part2 n/a\n";
}
