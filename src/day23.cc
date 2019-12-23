import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import <unordered_map>;
import <map>;
import <set>;
import intcode;
import util.circular_buffer;
import util.io;
import util.vec2;

using vec2v = vec2<program::value_type>;

struct network;

struct computer {
  bool running = false;
  program cpu;
  program::state state = program::ready;
  circular_buffer<program::value_type, 1000> buffered_input;
  circular_buffer<program::value_type, 5> buffered_output;
};

struct network {
  std::array<computer, 50> computers;

  void send(program::value_type destination,
            program::value_type x, program::value_type y) {
    check(0 <= destination && destination < 50);
    auto& target = computers[destination];
    target.buffered_input.push(x);
    target.buffered_input.push(y);
    target.running = true;
  }

  std::optional<vec2v> run(int index) {
    constexpr int steps = 10;
    auto& computer = computers[index];
    computer.running = true;
    for (int i = 0; i < steps; i++) {
      switch (computer.state) {
        case program::ready: break;
        case program::waiting_for_input:
          if (computer.buffered_input.empty()) {
            computer.running = false;
            return std::nullopt;
          }
          computer.cpu.provide_input(computer.buffered_input.pop());
          break;
        case program::output:
          computer.buffered_output.push(computer.cpu.get_output());
          if (computer.buffered_output.size() == 3) {
            auto address = computer.buffered_output.pop();
            auto x = computer.buffered_output.pop();
            auto y = computer.buffered_output.pop();
            if (address == 255) {
              computer.state = computer.cpu.resume();
              return vec2v{x, y};
            } else {
              send(address, x, y);
            }
          }
          break;
        case program::halt:
          computer.running = false;
          return std::nullopt;
      }
      computer.state = computer.cpu.resume();
    }
    return std::nullopt;
  }
};

int part1(network network) {
  while (true) {
    std::array<bool, 50> running_before, running_after;
    do {
      for (int i = 0; i < 50; i++) {
        running_before[i] = network.computers[i].running;
      }
      for (int i = 0; i < 50; i++) {
        if (network.computers[i].running) {
          if (auto result = network.run(i); result) return result->y;
        }
      }
      for (int i = 0; i < 50; i++) {
        running_after[i] = network.computers[i].running;
      }
    } while (running_before != running_after);
    for (int i = 0; i < 50; i++) {
      network.computers[i].buffered_input.push(-1);
      if (auto result = network.run(i); result) return result->y;
    }
  }
}

int part2(network network) {
  std::optional<program::value_type> previous_y;
  vec2v nat;
  while (true) {
    std::array<bool, 50> running_before, running_after;
    do {
      for (int i = 0; i < 50; i++) {
        running_before[i] = network.computers[i].running;
      }
      for (int i = 0; i < 50; i++) {
        if (network.computers[i].running) {
          if (auto result = network.run(i); result) nat = *result;
        }
      }
      for (int i = 0; i < 50; i++) {
        running_after[i] = network.computers[i].running;
      }
    } while (running_before != running_after);
    if (running_after == std::array<bool, 50>{}) {
      std::map<program::state, int> states;
      for (auto& x : network.computers) states[x.state]++;
      if (states[program::waiting_for_input] == 50) {
        network.send(0, nat.x, nat.y);
        if (previous_y && nat.y == *previous_y) return nat.y;
        previous_y = nat.y;
      }
    }
    for (int i = 0; i < 50; i++) {
      network.computers[i].buffered_input.push(-1);
      if (auto result = network.run(i); result) nat = *result;
    }
  }
}

int main(int argc, char* argv[]) {
  program::buffer program_buffer;
  const auto source = program::load(init(argc, argv), program_buffer);

  network network;
  for (int i = 0; i < 50; i++) {
    network.computers[i].buffered_input.push(i);
    network.computers[i].cpu = program(source);
  }

  std::cout << "part1 " << part1(network) << '\n';
  std::cout << "part2 " << part2(network) << '\n';
}
