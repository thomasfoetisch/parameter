#include "parameter.hpp"

int main(int argc, char** argv) {
  try {  
    parameter::collection p;
    p.read_from_file(argv[1]);
    p.print_key_values(std::cout);

    std::cout << std::boolalpha << p.get_value<bool>("key4") << std::endl;
  }
  catch (const std::string& e) {
    std::cout << e << std::endl;
  }
    
  
  return 0;
}
