# ConfigParser
Parse YAML config file

How to Use
```cpp
#include "ConfigParser.hpp"

// =======================
// 使用例 (main関数)
// =======================
int main(int argc, char* argv[]) {
  int scalar;
  Vec3<int> position;
  std::vector<int> list;

  ConfigParser parser;
  parser.add_option("scalar", scalar)->default_val("42");
  parser.add_option("vector", position)->default_val("1 2 3")->expected(3);
  parser.add_option("list", list)->default_val("10, 20, 30");

  if (2 <= argc) {
    parser.parse(argv[1]);

    std::cout << "scalar = " << scalar << "\n";
    std::cout << "vector = " << position.x() 
              << " " << position.y()
              << " " << position.z() << "\n";

    std::cout << "list = ";
    for (auto v : list)
      std::cout << v << " ";
    std::cout << std::endl;
  }

  return 0;
}

YAML file:
scalar: 100
vector: 4 5 6
list: 7
