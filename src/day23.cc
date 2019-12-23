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
  program cpu;
  program::state state = program::ready;
  circular_buffer<program::value_type, 1000> buffered_input;
  circular_buffer<program::value_type, 5> buffered_output;

  bool running() {
    return !(state == program::waiting_for_input && buffered_input.empty());
  }
};

struct network {
  std::array<computer, 50> computers;

  void send(program::value_type destination,
            program::value_type x, program::value_type y) {
    check(0 <= destination && destination < 50);
    auto& target = computers[destination];
    target.buffered_input.push(x);
    target.buffered_input.push(y);
  }

  // Run computer[i] until it gets blocked. A computer is blocked if it needs
  // input but there is none, if it sends a message to the NAT via address 255,
  // or if it halts. The question says that input should not block, but this is
  // handled in the part1() and part2() functions.
  std::optional<vec2v> run(int index) {
    auto& computer = computers[index];
    while (true) {
      switch (computer.state) {
        case program::ready: break;
        case program::waiting_for_input:
          if (computer.buffered_input.empty()) {
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
          return std::nullopt;
      }
      computer.state = computer.cpu.resume();
    }
  }

  using state_counts = std::array<int, 4>;
  state_counts state() const {
    state_counts output = {};
    for (auto& c : computers) output[c.state]++;
    return output;
  }
};

int part1(network network) {
  while (true) {
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
    if (std::none_of(network.computers.begin(), network.computers.end(),
                     [](auto& c) { return c.running(); }) &&
        network.state()[program::waiting_for_input] == 50) {
      network.send(0, nat.x, nat.y);
      if (previous_y && nat.y == *previous_y) return nat.y;
      previous_y = nat.y;
    }
    for (int i = 0; i < 50; i++) {
      // Allow at most one non-blocking read. This allows for forward progress
      // but also gives us chance to inspect the state and decide whether the
      // NAT should transmit anything.
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
