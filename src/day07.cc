import "util/check.h";
import <array>;
import <charconv>;  // bug
import <optional>;  // bug
import <span>;
import <vector>;  // bug
import util.io;
import intcode;

program::value_type part1(program::const_span source) {
  program::value_type output[1000];
  program::value_type max_signal = -1'000'000'000;
  std::array<program::value_type, 5> signal = {0, 1, 2, 3, 4}, best_signal = {};
  do {
    program::value_type value = 0;
    for (program::value_type phase : signal) {
      program::value_type input[] = {phase, value};
      auto out = program(source).run(input, output);
      check(out.size() == 1);
      value = out[0];
    }
    if (value > max_signal) {
      max_signal = value;
      best_signal = signal;
    }
  } while (std::next_permutation(std::begin(signal), std::end(signal)));
  return max_signal;
}

program::value_type part2(program::const_span source) {
  program::value_type max_signal = -1'000'000'000;
  std::array<program::value_type, 5> signal = {5, 6, 7, 8, 9}, best_signal = {};
  do {
    program amplifier[5];
    for (int i = 0; i < 5; i++) {
      amplifier[i] = program(source);
      check(amplifier[i].resume() == program::waiting_for_input);
      amplifier[i].provide_input(signal[i]);
      check(amplifier[i].resume() == program::waiting_for_input);
    }
    program::value_type value = 0;
    while (!amplifier[4].done()) {
      for (int i = 0; i < 5; i++) {
        check(!amplifier[i].done());
        amplifier[i].provide_input(value);
        auto state = amplifier[i].resume();
        if (state == program::halt) goto done;
        check(state == program::output);
        value = amplifier[i].get_output();
        amplifier[i].resume();
      }
    }
  done:
    if (value > max_signal) {
      max_signal = value;
      best_signal = signal;
    }
  } while (std::next_permutation(std::begin(signal), std::end(signal)));
  return max_signal;
}

int main(int argc, char* argv[]) {
  program::buffer buffer;
  auto source = program::load(init(argc, argv), buffer);
  std::cout << "part1 " << part1(source) << "\n"
            << "part2 " << part2(source) << "\n";
}
