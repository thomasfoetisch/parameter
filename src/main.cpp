#include "parameter.hpp"

int main(int argc, char** argv) {
  try {  
    parameter::collection p;
    p.read_from_file(argv[1]);
    std::cout << "collection size is " << p.get_collection_size() << std::endl;
    p.print_key_values(std::cout);

    std::cout << "value of space-subdivisions: " << p.get_value<int>("space-subdivisions") << std::endl;
    std::cout << "value of time-subdivisions: " << p.get_value<int>("time-subdivisions") << std::endl;
    std::cout << "value of output-prefix: " << p.get_value<std::string>("output-prefix") << std::endl;
  }
  catch (const std::string& e) {
    std::cout << e << std::endl;
  }
    
  return 0;
}
