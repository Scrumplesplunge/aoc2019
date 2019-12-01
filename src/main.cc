// BUG: <charconv> is not used by this file. It is used in the body of
// a template function which is imported from io. I think the compiler should
// not require this to be imported here but it does.
import <charconv>;
import <iostream>;
import <optional>;
import io;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: main <filename>\n";
    return 1;
  }

  scanner s{contents(argv[1])};
  std::string_view name;
  int x;
  s >> exact{"Hello, "} >> sequence<is_alpha>{name} >> exact{"!"};
  s >> x;
  std::cout << "name: " << name << "\n"
            << "x: " << x << "\n";
}
