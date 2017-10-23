#include "parameter.hpp"

namespace parameter {

  std::ostream& operator<<(std::ostream& stream, symbol s) {
    switch (s) {
    case symbol::eoi:
      stream << "<eoi>"; break;
    case symbol::key:
      stream << "<key>"; break;
    case symbol::import:
      stream << "<import>"; break;
    case symbol::equal:
      stream << "<equal>"; break;
    case symbol::value:
      stream << "<value>"; break;
    case symbol::string:
      stream << "<string>"; break;
    case symbol::real:
      stream << "<real>"; break;
    case symbol::integer:
      stream << "<integer>"; break;
    case symbol::boolean:
      stream << "<boolean>"; break;
    case symbol::enum_item:
      stream << "<enum_token>"; break;
    case symbol::override_keyword:
      stream << "<override>"; break;
    case symbol::comma:
      stream << "<comma>"; break;
    case symbol::lbracket:
      stream << "<lbracket>"; break;
    case symbol::rbracket:
      stream << "<rbracket>"; break;
    }
    return stream;
  }

  
  regex_lexer<token_type> build_lexer() {
    regex_lexer_builder<token_type> rlb(symbol::eoi); {
      rlb.emit(symbol::key, "[-_a-zA-Z0-9]+");
      rlb.emit(symbol::boolean, "(true)|(false)|(yes)|(no)|(on)|(off)");
      rlb.emit(symbol::string, "\"([^\"\\\\]|(\\\\\")|(\\\\\\\\))*\"");
      rlb.emit(symbol::real,
               "[+-]?"
               "((\\.\\d+)|(\\d+\\.)|(\\d+\\.\\d+)|(\\d+))"
               "([eE][+-]?\\d+)?");
      rlb.emit(symbol::integer, "[+-]?\\d+");
      rlb.emit(symbol::equal, "=|:|(->)");
      rlb.emit(symbol::enum_item, "#[-_a-zA-Z0-9]+");
      rlb.emit(symbol::import, "import");
      rlb.emit(symbol::override_keyword, "override");
      rlb.emit(symbol::comma, ",");
      rlb.emit(symbol::lbracket, "\\[");
      rlb.emit(symbol::rbracket, "\\]");
      
      rlb.skip("(\\s|(;[^\\n]*\\n))*");
    }

    return rlb.build();
  }


  const char* const basic_value::type_names[4];
  
  template<>
  std::string value<std::string>::print_value() const {
    std::ostringstream oss; oss << "\"" << v << "\"";
    return oss.str();
  }

  
  template<>
  std::string value<bool>::print_value() const {
    std::ostringstream oss; oss << std::boolalpha << v;
    return oss.str();
  }


  template<>
  const basic_value* value<std::string>::eval(const collection& c) const {
    std::string result;
    
    // Detect top-level matched curly braces
    std::size_t level(0);
    std::size_t opening_brace_location(std::string::npos);
    for (std::size_t i(0); i < v.size(); ++i) {
      if (v[i] == '{') {
        level += 1;
        if (level == 1)
          opening_brace_location = i;
      } else if (v[i] == '}') {
        if (level > 0)
          level -= 1;
        
        if (level == 0) {
          const std::string v_name(v.substr(opening_brace_location + 1,
                                            i - opening_brace_location - 1));
          const basic_value* v_ptr(c.get_basic_value(v_name)->eval(c));
          result += v_ptr->print_value();
          delete v_ptr;
        }
      } else if (level == 0) {
        result += v[i];
      }
    }
    
    return new value<std::string>(result);
  }
  
  const basic_value* value_ref::eval(const collection& c) const { return c.get_basic_value(key)->clone(); }

}
