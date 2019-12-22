import "util/check.h";
import <array>;
import <charconv>;  // bug
import <experimental/coroutine>;  // bug
import <iostream>;
import <optional>;  // bug
import <span>;
import <vector>;
import <map>;
import <numeric>;
import <unordered_map>;
import <set>;
import <unordered_set>;
import util.coroutine;
import util.io;
import util.vec2;

void deal(std::span<int> cards, int increment) {
  std::vector<int> temp(cards.begin(), cards.end());
  for (int i = 0, j = 0, n = cards.size(); i < n;
       i++, j = (j + increment) % n) {
    cards[j] = temp[i];
  }
}

std::int64_t phi(std::int64_t x) {
  std::int64_t total = 1;
  for (std::int64_t i = 2; x != 1; i++) {
    while (x % i == 0) x /= i, total *= i - 1;
  }
  return total;
}

constexpr std::int64_t mod_mul(std::int64_t a, std::int64_t b, std::int64_t n) {
  return (__int128_t)a * b % n;
}

// Compute a^x mod n
constexpr std::int64_t mod_exp(std::int64_t a, std::int64_t x, std::int64_t n) {
  std::int64_t result = 1;
  for (int i = 0; i < 63; i++) {
    result = mod_mul(result, result, n);
    if ((x >> (62 - i)) & 1) result = mod_mul(result, a, n);
  }
  return result;
}

// Compute the modular inverse of a.
std::int64_t mod_inv(std::int64_t a, std::int64_t n) {
  check(std::gcd(a, n) == 1);
  auto x = mod_exp(a, phi(n) - 1, n);
  //std::cout << "inv(" << a << ") = " << x << '\n';
  //check(a * x % n == 1);
  return x;
}

int main(int argc, char* argv[]) {
  scanner scanner(init(argc, argv));

  std::vector<int> cards(10'007);
  std::iota(cards.begin(), cards.end(), 0);

  while (!scanner.done()) {
    scanner >> whitespace;
    if (scanner.remaining().starts_with("cut ")) {
      int position;
      (scanner >> exact("cut ") >> position).check_ok();
      check(-10'007 <= position && position <= 10'007);
      if (position < 0) {
        std::rotate(cards.begin(), cards.end() + position, cards.end());
      } else {
        std::rotate(cards.begin(), cards.begin() + position, cards.end());
      }
    } else if (scanner.remaining().starts_with("deal into new stack")) {
      (scanner >> exact("deal into new stack")).check_ok();
      std::reverse(cards.begin(), cards.end());
    } else {
      int increment;
      (scanner >> exact("deal with increment ") >> increment).check_ok();
      check(std::gcd(10'007, increment) == 1);
      deal(cards, increment);
      //std::vector<int> temp(10'007);
      //for (int i = 0, j = 0; i < 10'007; i++, j = (j + increment) % 10'007) {
      //  temp[j] = cards[i];
      //}
      //std::swap(cards, temp);
    }
  }

  auto i = std::find(cards.begin(), cards.end(), 2019) - cards.begin();
  std::cout << "part1 " << i << '\n';  // wa 2363, 9744, 3xx

  // (rotate a, rotate b) == rotate (a + b)
  {
    std::vector<int> cards(10'007);
    std::iota(cards.begin(), cards.end(), 0);
    auto left = cards;
    std::rotate(left.begin(), left.begin() + 5, left.end());
    std::rotate(left.begin(), left.begin() + 5, left.end());
    auto right = cards;
    std::rotate(right.begin(), right.begin() + 10, right.end());
    check(left == right);
  }
  // (deal a, deal b) == deal (a * b) ?
  {
    std::vector<int> cards(10'007);
    std::iota(cards.begin(), cards.end(), 0);
    auto left = cards;
    deal(left, 5);
    deal(left, 5);
    auto right = cards;
    deal(right, 25);
    check(left == right);
  }
  // (rotate a, deal b) == (deal b, rotate a * b)
  {
    std::vector<int> cards(10'007);
    std::iota(cards.begin(), cards.end(), 0);
    auto left = cards;
    std::rotate(left.begin(), left.begin() + 5, left.end());
    deal(left, 3);
    auto right = cards;
    deal(right, 3);
    std::rotate(right.begin(), right.begin() + 3 * 5, right.end());
    check(left == right);
  }
  // (deal a, rotate b) == (rotate b * inv(a), deal a)
  {
    std::vector<int> cards(10'007);
    std::iota(cards.begin(), cards.end(), 0);
    auto left = cards;
    deal(left, 3);
    std::rotate(left.begin(), left.begin() + 5, left.end());
    auto right = cards;
    std::rotate(right.begin(),
                right.begin() + mod_mul(5, mod_inv(3, 10'007), 10'007),
                right.end());
    deal(right, 3);
    check(left == right);
  }
  // std::cout << "part2 " << ??? << '\n';

  constexpr std::int64_t size = 10'007; //119'315'717'514'047;
  struct state {
    std::int64_t offset = 0;
    std::int64_t increment = 1;

    // Simulate state().rotate(offset).deal(increment)
    std::int64_t operator[](std::int64_t index) {
      return mod_mul(index + offset, mod_inv(increment, size), size);
    }

    state& rotate(std::int64_t amount) {
      offset += mod_mul(amount, mod_inv(increment, size), size);
      offset %= size;
      if (offset < 0) offset += size;
      return *this;
    }

    state& deal(std::int64_t amount) {
      offset = (offset + (amount - 1)) % size;
      increment *= amount;
      increment %= size;
      check(increment != 0);
      return *this;
    }
  };

  for (int d = 1; d <= 4; d++) {
    constexpr int r = 1;
    std::cout << "with r=" << r << ", d=" << d << "\n";
    std::vector<int> cards(size);
    std::iota(cards.begin(), cards.end(), 0);
    std::rotate(cards.begin(), cards.begin() + r, cards.end());
    deal(cards, d);
    //std::cout << "cards:";
    //for (int i = 0; i < size; i++) std::cout << " " << cards[i];
    //std::cout << '\n';
    //std::cout << "state:";
    //for (int i = 0; i < size; i++) {
    //  std::cout << " " << state().rotate(r).deal(d)[i];
    //}
    //std::cout << '\n';
    for (int i = 0; i < size; i++) {
      check(cards[i] == state().rotate(r).deal(d)[i]);
    }
  }
}
