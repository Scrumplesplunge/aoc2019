import util.coroutine;
import <chrono>;
import <coroutine>;
import <iostream>;
import <optional>;
import <vector>;

using namespace std::chrono_literals;

promise<int> demo() {
  co_await 1s;
  co_return 42;
}

promise<int> consume() {
  std::cout << "Entry\n";
  auto x = demo();
  co_await 500ms;
  auto y = demo();
  std::cout << "Waiting for results...\n";
  std::cout << co_await x + co_await y << "\n";
  co_return 0;
}

struct noisy {
  noisy() { std::cout << "noisy()\n"; }
  ~noisy() { std::cout << "~noisy()\n"; }
};

generator<int> gen() {
  noisy n;
  for (int i = 0; i < 42; i++) {
    co_yield i * i;
  }
}

int main() {
  int total = 0;
  for (auto x : gen()) total += x;
  std::cout << "total: " << total << '\n';
  series_executor executor;
  set_executor set(&executor);
  consume();
  executor.run();
}
