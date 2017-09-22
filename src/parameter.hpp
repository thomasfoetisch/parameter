#ifndef PARAMETER_H
#define PARAMETER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <numeric>

#include <unistd.h>

#include <spikes/meta.hpp>
#include <lexer/lexer.hpp>

namespace parameter {

  class resource_locator {
  public:
    resource_locator(const std::string& path)
      : separator('/'), is_absolute(false) {
      if (path[0] == '/')
        is_absolute = true;
      components = split(path, separator);
    }
    
    std::string resource_name() const { return components.back(); }
    
    resource_locator resource_path() const {
      if (components.size() == 1)
        return resource_locator("./");
      else
        return resource_locator(components.begin(), components.end() - 1, is_absolute);
    }
    
    std::string to_string() const {
      std::string str;

      if (is_absolute) str += separator;

      for (std::size_t i(0); i < components.size() - 1; ++i) {
        str += components[i];
        str += '/';
      }
      str += components.back();

      return str;
    }

  private:
    std::vector<std::string> components;
    char separator;
    bool is_absolute;

  private:
    resource_locator(std::vector<std::string>::const_iterator begin,
                     std::vector<std::string>::const_iterator end,
                     bool is_absolute)
      : components(begin, end), separator('/'), is_absolute(is_absolute) {}

    static std::vector<std::string> split(const std::string& str, char separator) {
      std::vector<std::string> components;
      std::string c;
      
      std::size_t i(0);
      while (i < str.size()) {
        if (str[i] == separator) {
          if (c.size())
            components.push_back(c);
          c.clear();
          i += 1;
        } else {
          c += str[i];
          i += 1;
        }
      }
      
      if (c.size())
        components.push_back(c);

      return components;
    }
  };

  inline
  resource_locator get_current_working_directory() {
    std::vector<char> buffer(255);
    char* b(&buffer[0]);
    while (getcwd(b, buffer.size()) == nullptr)
      buffer.resize(buffer.size() * 2);
    return resource_locator(std::string(b));
  }

  inline
  void change_directory(resource_locator l) {
    std::string path(l.to_string());
    if (path != ".") {

      int res(chdir(path.c_str()));

      if (res != 0) {
        switch (errno) {
        case EACCES:
          throw std::string("chdir error EACCES");
        case EFAULT:
          throw std::string("chdir error EFAULT");
        case EIO:
          throw std::string("chdir error EIO");
        case ELOOP:
          throw std::string("chdir error ELOOP");
        case ENAMETOOLONG:
          throw std::string("chdir error ENAMETOOLONG");
        case ENOENT:
          throw std::string("chdir error ENOENT");
        case ENOMEM:
          throw std::string("chdir error ENOMEM");
        case ENOTDIR:
          throw std::string("chdir error ENOTDIR");
        default:
          throw std::string("chdir unknown error");
        }
      }
    }
  }
  
  class string_builder {
  public:
    template<typename value_type>
    string_builder(const value_type& v) {
      oss << v;
    }

    template<typename value_type>
    string_builder& operator()(const value_type& v) {
      oss << v;
      return *this;
    }

    operator std::string() const {
      return oss.str();
    }

    std::string str() const { return oss.str(); }

  private:
    std::ostringstream oss;
  };
  
  template<typename token_type>
  class token_source {
  public:
    token_source(regex_lexer<token_type>* l)
      : lex(l), current(lex->get()) {}

    ~token_source() { delete current; }
    
    token_type* get() {
      token_type* c(current);
      current = lex->get();
      return c;
    }
    
    token_type* peek() {
      return current;
    }

  private:
    regex_lexer<token_type>* lex;
    token_type* current;
  };

  
  enum class symbol {
    eoi, equal, value,
    string,
    integer,
    real,
    boolean,
    import,
    key
  };

  using symbol_type = symbol;
  using token_type = token<symbol_type>;

  
  std::ostream& operator<<(std::ostream& stream, symbol s);
  regex_lexer<token_type> build_lexer();

  
  class basic_value {
  public:
    virtual ~basic_value() {}
    virtual std::string get_type() const = 0;
    virtual std::string print_value() const = 0;

    using value_type_list = type_list<int, bool, std::string, double>;
    static constexpr const char* type_names[] = {"integer", "boolean", "string", "real"};
  };

  template<typename value_type>
  class value: public basic_value {
  public:
    value(const value_type& v): v(v) {}
      
    virtual std::string get_type() const {
      return type_names[get_index_of_element<value_type, value_type_list>::value];
    }

    virtual std::string print_value() const;

    const value_type& get_value() const { return v; }
      
  private:
    const value_type v;
  };

  
  template<typename value_type>
  std::string value<value_type>::print_value() const {
    std::ostringstream oss; oss << v;
    return oss.str();
  }

  template<>
  std::string value<std::string>::print_value() const;

  template<>
  std::string value<bool>::print_value() const;

  
  class collection {
  public:
    collection() {}
    ~collection() { clear(); }
      
    void read_from_file(const std::string& filename) {
      resource_locator config_file(filename);
      resource_locator cwd(get_current_working_directory());
      change_directory(config_file.resource_path()); {
      
        std::ifstream f(config_file.resource_name().c_str(), std::ios::in);
        if (not f)
          throw std::string("file '" + filename + "' is not accessible");

        regex_lexer<token_type> lex(build_lexer());
      
        file_source<token_type> fs(&f, filename);
        lex.set_source(&fs);

        token_source<token_type> ts(&lex);
        parse_parameter_list(ts);
      }

      change_directory(cwd);
    }

    void set_key_value(const std::string& key, double value) {
      set_key_value(key, new ::parameter::value<double>(value));
    }
    void set_key_value(const std::string& key, bool value) {
      set_key_value(key, new ::parameter::value<bool>(value));
    }
    void set_key_value(const std::string& key, int value) {
      set_key_value(key, new ::parameter::value<int>(value));
    }
    void set_key_value(const std::string& key, const std::string& value) {
      set_key_value(key, new ::parameter::value<std::string>(value));
    }

    template<typename value_type>
    const value_type& get_value(const std::string& key) const {
      using map_type = std::map<std::string, basic_value*>;
      using map_iterator_type = map_type::const_iterator;

      map_iterator_type kv(key_value.find(key));
      if (kv == key_value.end()) {
        std::string suggestion;
        if (make_suggestion(key, suggestion)) {
            throw std::string("the key '"
                              + key
                              + "' is not found in the parameter collection, did you mean '"
                              + suggestion
                              + "'?");
        } else {
            throw std::string("the key '"
                              + key
                              + "' is not found in the parameter collection");
        }
      }

      const value<value_type>* v(dynamic_cast<const value<value_type>*>(kv->second));
      if (not v)
        throw std::string("failed to get a "
                          + std::string(basic_value::type_names[get_index_of_element<value_type, basic_value::value_type_list>::value])
                          + "from the key '" + kv->first
                          + "' which has type " + kv->second->get_type());

      return v->get_value();
    }

    void clear() {
      for (auto kv: key_value) {
        delete kv.second;
      }
    }

    void print_key_values(std::ostream& stream) const {
      for (const auto& kv: key_value)
        stream << kv.second->get_type() << " " << kv.first
          << " = "
          << kv.second->print_value() << std::endl;
    }
    
  private:
    std::map<std::string, basic_value*> key_value;

  private:
    std::size_t levenshtein_distance(const std::string& s1, const std::string& s2) const {
      std::vector<int>
        v0(s2.size() + 1),
        v1(s2.size() + 1);
      
      std::iota(v0.begin(), v0.end(), 0);

      for (std::size_t i(0); i < s1.size(); ++i) {
        v0[0] = i + 1;

        int substitution_cost(0);
        for (std::size_t j(0); j < s2.size(); ++j) {
          if (s1[i] == s2[j])
            substitution_cost = 0;
          else
            substitution_cost = 1;

          v1[j + 1] = std::min({v1[j] + 1,
                                v0[j + 1] + 1,
                                v0[j] + substitution_cost});
        }
        std::swap(v0, v1);
      }
      return v0.back();
    }
    
    bool make_suggestion(const std::string& key, std::string& suggestion) const {
      using map_type = std::map<std::string, basic_value*>;
      using map_iterator_type = map_type::const_iterator;

      map_iterator_type closest_key(
        std::min_element(key_value.cbegin(),
                         key_value.cend(),
                         [&](const map_type::value_type& kv1,
                             const map_type::value_type& kv2) {
                           return levenshtein_distance(kv1.first, key) < levenshtein_distance(kv2.first, key);
                         }));
      
      if (closest_key != key_value.end()) {
        suggestion = closest_key->first;
        return true;
      } else {
        return false;
      }
    }
    
    void set_key_value(const std::string& key, basic_value* v) {
      using map_type = std::map<std::string, basic_value*>;
      using map_iterator_type = map_type::iterator;

      map_iterator_type kv(key_value.find(key));
      if (kv == key_value.end()) {
        key_value[key] = v;
      } else {
        delete kv->second;
        kv->second = v;
      }
    }

    std::string string_token_to_string(token_type* t) {
      std::string str(t->value.substr(1, t->value.size() - 2));

      std::string::iterator i(str.begin());
      while (i != str.end()) {
        if (*i == '\\') {
          i = str.erase(i);
          if (i != str.end())
            ++i;
        } else {
          ++i;
        }
      }
      
      return str;
    }
    
    // FIRST(parameter list) = {key}
    void parse_parameter_list(token_source<token_type>& ts) {
      token_type* t(ts.peek());

      while (t->symbol != symbol::eoi) {
        switch (t->symbol) {
        case symbol::key:
          parse_key_value(ts);
          break;
        case symbol::import:
          parse_import_statment(ts);
          break;
        default:
          throw string_builder("unexpected ")
            (t->symbol)
            (" token at ")
            (t->render_coordinates())
            (" instead of a ")
            (symbol::key)
            (" token").str();
        }
        t = ts.peek();
      }
    }

    // FIRST(key_value) = {key}
    void parse_key_value(token_source<token_type>& ts) {
      token_type
        *key_token(ts.get()),
        *equal_token(ts.get());

      if (equal_token->symbol != symbol::equal)
        throw string_builder("unexpected ")
          (equal_token->symbol)
          (" token at ")
          (equal_token->render_coordinates())
          (" instead of a ")
          (symbol::equal).str();

      token_type* value_token(ts.peek());
      switch (value_token->symbol) {
      case symbol::integer:
        set_key_value(key_token->value, parse_integer_value(ts));
        break;
      case symbol::real:
        set_key_value(key_token->value, parse_real_value(ts));
        break;
      case symbol::boolean:
        set_key_value(key_token->value, parse_boolean_value(ts));
        break;
      case symbol::string:
        set_key_value(key_token->value, parse_string_value(ts));
        break;
      default:
        throw string_builder("unexpected ")
          (value_token->symbol)
          (" token at ")
          (value_token->render_coordinates())
          (" instead of a ")
          (symbol::value).str();
      }

      delete key_token;
      delete equal_token;
    }

    // FIRST(integer_value) = {integer}
    basic_value* parse_integer_value(token_source<token_type>& ts) {
      token_type* integer_token(ts.get());
      if (integer_token->symbol != symbol::integer)
        throw string_builder("unexpected ")
          (integer_token->symbol)
          (" token at ")
          (integer_token->render_coordinates())
          (" instead of a ")(symbol::integer).str();

      std::size_t pos(0);
      basic_value* v(new ::parameter::value<int>(std::stoi(integer_token->value, &pos)));
      if (pos != integer_token->value.size())
        throw string_builder("failed to convert ")
          (integer_token->symbol)
          (" token at ")
          (integer_token->render_coordinates())
          (" to an integer value ").str();
      
      delete integer_token;

      return v;
    }

    // FIRST(integer_value) = {real}
    basic_value* parse_real_value(token_source<token_type>& ts) {
      token_type* real_token(ts.get());
      if (real_token->symbol != symbol::real)
        throw string_builder("unexpected ")
          (real_token->symbol)
          (" token at ")
          (real_token->render_coordinates())
          (" instead of a ")
          (symbol::real).str();

      std::size_t pos(0);
      basic_value* v(new ::parameter::value<double>(std::stod(real_token->value, &pos)));
      if (pos != real_token->value.size())
        throw string_builder("failed to convert ")
          (real_token->symbol)
          (" token at ")
          (real_token->render_coordinates())
          (" to an real value ").str();
      
      delete real_token;

      return v;
    }

    // FIRST(integer_value) = {string}
    basic_value* parse_string_value(token_source<token_type>& ts) {
      token_type* string_token(ts.get());
      if (string_token->symbol != symbol::string)
        throw string_builder("unexpected ")
          (string_token->symbol)
          (" token at ")
          (string_token->render_coordinates())
          (" instead of a ")(symbol::real).str();

      basic_value* v(new ::parameter::value<std::string>(string_token_to_string(string_token)));
      
      delete string_token;

      return v;
    }

    // FIRST(integer_value) = {boolean}
    basic_value* parse_boolean_value(token_source<token_type>& ts) {
      token_type* boolean_token(ts.get());
      if (boolean_token->symbol != symbol::boolean)
        throw string_builder("unexpected ")
          (boolean_token->symbol)
          (" token at ")
          (boolean_token->render_coordinates())
          (" instead of a ")(symbol::real).str();

      basic_value* v(nullptr);
      if (boolean_token->value == "on" or
          boolean_token->value == "yes" or
          boolean_token->value == "true")
        v = new ::parameter::value<bool>(true);
      else if (boolean_token->value == "off" or
               boolean_token->value == "no" or
               boolean_token->value == "false")
        v = new ::parameter::value<bool>(false);
      else
        throw string_builder("failed to convert ")
          (boolean_token->symbol)
          (" token at ")
          (boolean_token->render_coordinates())
          (" to an real value ").str();
      
      delete boolean_token;

      return v;
    }

    void parse_import_statment(token_source<token_type>& ts) {
      token_type
        *import_token(ts.get()),
        *string_token(ts.get());

      if (import_token->symbol != symbol::import)
        throw string_builder("unexpected ")
          (import_token->symbol)
          (" token at ")
          (import_token->render_coordinates())
          (" instead of a ")(symbol::import).str();

      if (string_token->symbol != symbol::string)
        throw string_builder("unexpected ")
          (string_token->symbol)
          (" token at ")
          (string_token->render_coordinates())
          (" instead of a ")(symbol::string).str();

      read_from_file(string_token_to_string(string_token));
    }
  };
  
}

#endif /* PARAMETER_H */
