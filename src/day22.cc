import "util/check.h";
import <array>;
import <charconv>;  // bug
import <experimental/coroutine>;  // bug
import <iostream>;
import <optional>;  // bug
import <span>;
import <numeric>;
import util.io;

template <std::int64_t n>
constexpr std::int64_t mod_mul(std::int64_t a, std::int64_t b) {
  // __int128_t is not standard but is portable across most compilers and
  // modern platforms.
  return (__int128_t)a * b % n;
}

// Compute a^x mod n
template <std::int64_t n>
constexpr std::int64_t mod_exp(std::int64_t a, std::int64_t x) {
  std::int64_t result = 1;
  for (int i = 0; i < 63; i++) {
    result = mod_mul<n>(result, result);
    if ((x >> (62 - i)) & 1) result = mod_mul<n>(result, a);
  }
  return result;
}

// Compute the modular inverse of a.
template <std::int64_t prime>
std::int64_t mod_inv(std::int64_t a) {
  check(std::gcd(a, prime) == 1);
  constexpr auto x = prime - 2;  // for primes, phi(n) - 1 = n - 2.
  return mod_exp<prime>(a, x);
}

template <std::int64_t prime>
struct state {
  // Both 10'007 and 119'315'717'514'047 are prime, but any prime will work.
  static constexpr std::int64_t size = prime;
  std::int64_t offset = 0;
  std::int64_t increment = 1;

  std::int64_t operator[](std::int64_t index) const {
    //return mod_mul<size>((index + offset) % size, mod_inv<size>(increment));
    return (offset + mod_mul<size>(index, mod_inv<size>(increment))) % size;
  }

  state& rotate(std::int64_t amount) {
    offset = (offset + mod_mul<size>(amount, mod_inv<size>(increment))) % size;
    return *this;
  }

  state& deal(std::int64_t amount) {
    increment = mod_mul<size>(increment, amount);
    check(std::gcd(increment, size) == 1);
    return *this;
  }

  state& reverse() { return deal(size - 1).rotate(1); }
};

template <std::int64_t n>
state<n> shuffle(std::string_view input) {
  scanner scanner(input);
  state<n> state;
  while (!scanner.done()) {
    scanner >> whitespace;
    if (scanner.remaining().starts_with("cut ")) {
      int position;
      (scanner >> exact("cut ") >> position).check_ok();
      check(-10'007 <= position && position <= 10'007);
      state.rotate(position);
    } else if (scanner.remaining().starts_with("deal into new stack")) {
      (scanner >> exact("deal into new stack")).check_ok();
      state.reverse();
    } else {
      int increment;
      (scanner >> exact("deal with increment ") >> increment).check_ok();
      check(std::gcd(10'007, increment) == 1);
      state.deal(increment);
    }
  }
  return state;
}

template <std::int64_t n>
state<n>& operator*=(state<n>& l, state<n> r) {
  return l.rotate(r.offset).deal(r.increment);
}

template <std::int64_t n>
state<n> extrapolate(state<n> input, std::int64_t repetitions) {
  const auto step = input;
  state<n> total;
  for (int i = 0; i < 63; i++) {
    total *= total;
    if ((repetitions >> (62 - i)) & 1) {
      total *= step;
    }
  }
  return total;
}

int main(int argc, char* argv[]) {
  auto input = init(argc, argv);

  auto part1 = shuffle<10'007>(input);
  int i = 0;
  while (i < 10'007 && part1[i] != 2019) i++;
  check(i != 10'007);
  std::cout << "part1 " << i << '\n';

  auto part2 = extrapolate(
      shuffle<119'315'717'514'047>(input), 101'741'582'076'661);
  std::cout << "part2 " << part2[2020] << '\n';
}
