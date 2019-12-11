import "util/check.h";
import <fstream>;
import <iostream>;

int fuel(int mass) {
  int x = mass / 3 - 2;
  return x > 0 ? x + fuel(x) : 0;
}

int main(int argc, char* argv[]) {
  check(argc == 2);
  std::ifstream input(argv[1]);
  check(input.good());
  int part_1 = 0, part_2 = 0;
  int x;
  while (input >> x) {
    part_1 += x / 3 - 2;
    part_2 += fuel(x);
  }
  check(input.eof());
  std::cout << "part1 " << part_1 << "\npart2 " << part_2 << "\n";
}
