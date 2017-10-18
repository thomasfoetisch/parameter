#include "parameter.hpp"

enum class bc_type {neumann, dirichlet, robin};


int main(int argc, char** argv) {
  std::map<std::string, bc_type> enum_value_map;
  enum_value_map["neumann"] = bc_type::neumann;
  enum_value_map["dirichlet"] = bc_type::dirichlet;
  enum_value_map["robin"] = bc_type::robin;

  
  try {  
    parameter::collection p;
    p.read_from_file(argv[1]);

    p.get_enum_value<bc_type>("left-bc-type", enum_value_map);
    p.get_enum_value<bc_type>("right-bc-type", enum_value_map);
  }
  catch (const std::string& e) {
    std::cout << e << std::endl;
  }
    
  return 0;
}
