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

  void send(int source, program::value_type destination,
            program::value_type x, program::value_type y) {
    check(0 <= destination && destination < 50);
    std::cout << source << " -> " << destination << ": " << x << " " << y
              << '\n';
    auto& target = computers[destination];
    target.buffered_input.push(x);
    target.buffered_input.push(y);
    if (!target.running) {
      //std::cout << "resume " << destination << "\n";
      target.running = true;
    }
  }

  struct run_result {
    bool hit_255 = false;
    program::value_type x, y;
  };

  run_result run(int index) {
    constexpr int steps = 10;
    auto& computer = computers[index];
    computer.running = true;
    for (int i = 0; i < steps; i++) {
      switch (computer.state) {
        case program::ready: break;
        case program::waiting_for_input:
          if (computer.buffered_input.empty()) {
            computer.running = false;
            return {false, 0, 0};
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
              return {true, x, y};
            } else {
              send(index, address, x, y);
            }
          }
          break;
        case program::halt:
          //std::cout << "halt\n";
          computer.running = false;
          return {false, 0, 0};
      }
      computer.state = computer.cpu.resume();
    }
    return {false, 0, 0};
  }
};

int part1(network network) {
  int generation = 0;
  while (true) {
    std::array<bool, 50> running_before, running_after;
    do {
      if (++generation % 10'000 == 0) {
        std::map<program::state, int> states;
        for (auto& x : network.computers) states[x.state]++;
        std::cout << "\rgeneration " << generation << ": "
                  << states[program::ready] << " ready, "
                  << states[program::waiting_for_input] << " waiting for input, "
                  << states[program::output] << " pending output, "
                  << states[program::halt] << " halted.     " << std::flush;
      }
      for (int i = 0; i < 50; i++) {
        running_before[i] = network.computers[i].running;
      }
      for (int i = 0; i < 50; i++) {
        if (network.computers[i].running) {
          if (auto result = network.run(i); result.hit_255) {
            return result.y;
          }
        }
      }
      for (int i = 0; i < 50; i++) {
        running_after[i] = network.computers[i].running;
      }
    } while (running_before != running_after);
    for (int i = 0; i < 50; i++) {
      network.computers[i].buffered_input.push(-1);
      if (auto result = network.run(i); result.hit_255) {
        return result.y;
      }
    }
  }
}

int part2(network network) {
  int generation = 0;
  std::optional<program::value_type> previous_y;
  program::value_type x = 0, y = 0;
  while (true) {
    std::array<bool, 50> running_before, running_after;
    do {
      if (++generation % 1'000'000 == 0) {
        std::map<program::state, int> states;
        for (auto& x : network.computers) states[x.state]++;
        std::cout << "\rgeneration " << generation << ": "
                  << states[program::ready] << " ready, "
                  << states[program::waiting_for_input] << " waiting for input, "
                  << states[program::output] << " pending output, "
                  << states[program::halt] << " halted.     " << std::flush;
      }
      for (int i = 0; i < 50; i++) {
        running_before[i] = network.computers[i].running;
      }
      for (int i = 0; i < 50; i++) {
        if (network.computers[i].running) {
          if (auto result = network.run(i); result.hit_255) {
            x = result.x, y = result.y;
          }
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
        network.send(255, 0, x, y);
        if (previous_y && y == *previous_y) return y;
        previous_y = y;
      }
    }
    for (int i = 0; i < 50; i++) {
      network.computers[i].buffered_input.push(-1);
      if (auto result = network.run(i); result.hit_255) {
        x = result.x, y = result.y;
      }
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
