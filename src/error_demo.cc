import util.io;
import <charconv>;
import <optional>;
import <iostream>;

constexpr bool nothing(char) { return false; }

int main() {
  scanner s("aaaaabbbbbcccccdddddeeeeefffffggggghhhhhiiiiijjjjjkkkkklllllmmmmmnnnnnooooopppppqqqqq");
  for (int i = 0; i < 85; i++) {
    char a, b;
    if (s >> a >> matches<nothing>(b, "nothing")) {
      std::cout << "ok\n";
    } else {
      std::cout << s.error() << "\n";
    }
    s.clear();
  }
}
