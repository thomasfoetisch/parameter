#include "parameter.hpp"

int main(int argc, char** argv) {
  try {  
    parameter::collection p;
    p.read_from_file(argv[1]);

    for (std::size_t i(0); i < p.get_collection_size(); ++i) {
      std::cout << "collection id " << i << " is:" << std::endl;
      p.set_current_collection(i);
      p.print_key_values(std::cout);
      std::cout << std::endl;
    }
  }
  catch (const std::string& e) {
    std::cout << e << std::endl;
  }
    
  return 0;
}
