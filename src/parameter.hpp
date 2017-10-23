#ifndef PARAMETER_H
#define PARAMETER_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <numeric>

#include <unistd.h>

#include <spikes/meta.hpp>
#include <spikes/array.hpp>
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
    eoi, equal, comma, value,
    enum_item,
    string,
    integer,
    real,
    boolean,
    import,
    lbracket, rbracket,
    override_keyword,
    key
  };

  using symbol_type = symbol;
  using token_type = token<symbol_type>;

  
  std::ostream& operator<<(std::ostream& stream, symbol s);
  regex_lexer<token_type> build_lexer();

  class collection;
  
  class basic_value {
  public:
    virtual ~basic_value() {}
    virtual std::string get_type() const = 0;
    virtual std::string print_value() const = 0;
    virtual basic_value* clone() const = 0;
    virtual const basic_value* eval(const collection& c) const = 0;
    
    using value_type_list = type_list<int, bool, std::string, double>;
    static constexpr const char* type_names[] = {"integer", "boolean", "string", "real"};
  };

  class enum_value: public basic_value {
  public:
    enum_value(const std::string& token_value): token_value(token_value) {}

    virtual std::string get_type() const {
      return "enum";
    }

    virtual std::string print_value() const {
      return std::string("#") + token_value;
    }
    
    virtual basic_value* clone() const {
      return new enum_value(*this);
    }

    virtual const basic_value* eval(const collection& c) const {
      return clone();
    }

    const std::string& get_token_value() const {
      return token_value;
    }

  private:
    const std::string token_value;
  };
  
  template<typename value_type>
  class value: public basic_value {
  public:
    value(const value_type& v): v(v) {}
      
    virtual std::string get_type() const {
      return type_names[get_index_of_element<value_type, value_type_list>::value];
    }

    virtual std::string print_value() const;

    virtual basic_value* clone() const {
      return new value<value_type>(*this);
    }

    virtual const basic_value* eval(const collection& c) const;
    
    const value_type& get_value() const { return v; }
      
  private:
    const value_type v;
  };

  template<typename value_type>
  const basic_value* value<value_type>::eval(const collection& c) const {
      return clone();
  }

  template<>
  const basic_value* value<std::string>::eval(const collection& c) const;

  class value_ref: public basic_value {
  public:
    value_ref(const std::string& key): key(key) {}

    virtual std::string get_type() const {
      return "ref";
    }
    virtual std::string print_value() const { return key; }

    virtual basic_value* clone() const { return new value_ref(*this); }

    virtual const basic_value* eval(const collection& c) const;
    
  private:
    const std::string key;
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
    using multi_index = std::vector<std::size_t>;
    struct multi_value {
      std::size_t index_id;
      std::vector<basic_value*> values;

      basic_value* get_value(const multi_index& is) const {
        if (is[index_id] >= values.size())
          throw string_builder("index out of bound in multivalue: ")
            (is[index_id])(" >= ")(values.size())
            (" (index_id = ")(index_id)(")").str();
        return values[is[index_id]];
      }
      std::string get_type(const multi_index& is) const {
        return get_value(is)->get_type();
      }

      std::size_t get_value_number() const { return values.size(); }
      void append_value(basic_value* v) { values.push_back(v); }

      std::size_t get_index_id() const { return index_id; }
      void set_index_id(std::size_t id) { index_id = id; }
      
      multi_value(): index_id(0) {}

      multi_value(std::size_t index_id, basic_value* v): index_id(index_id) {
        values.push_back(v);
      }
      
      ~multi_value() {
        for (auto v: values)
          delete v;
      }

      multi_value(const multi_value& mv): index_id(mv.index_id) {
        for (const auto v: mv.values)
          values.push_back(v->clone());
      }

      multi_value& operator=(const multi_value& mv) {
        for (auto v: values)
          delete v;
        values.clear();
        index_id = mv.index_id;
        for (const auto v: mv.values)
          values.push_back(v->clone());

        return *this;
      }

      std::string print_values() const {
        std::ostringstream oss;
        for (std::size_t i(0); i < values.size() - 1; ++i)
          oss << values[i]->print_value() << ", ";
        oss << values.back()->print_value();
        return oss.str();
      }

      std::string print_value(const multi_index& is) const {
        return get_value(is)->print_value();
      }
    };
    
    collection() {}
    ~collection() { clear(); }

    std::size_t get_collection_size() const {
      return array_element_number(parameter_space_sizes.size(),
                                  &parameter_space_sizes[0]);
    }

    void set_current_collection(std::size_t i) {
      to_multi_index(parameter_space_sizes.size(),
                     &selected_parameter_space[0],
                     &parameter_space_sizes[0],
                     i, true);
    }
    
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

    template<typename enum_type>
    enum_type get_enum_value(const std::string& key,
                                    const std::map<std::string, enum_type>& token_map) const {
      using map_type = std::map<std::string, multi_value>;
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

      const basic_value* tmp(kv->second.get_value(selected_parameter_space)->eval(*this));
      const enum_value* v(dynamic_cast<const enum_value*>(tmp));
      if (not v) {
        delete tmp;
        throw std::string("failed to get an enum value from the key '" + kv->first
                          + "' which has type " + kv->second.get_type(selected_parameter_space));
      }

      const auto mapped_enum_value(token_map.find(v->get_token_value()));
      if (mapped_enum_value == token_map.end()) {
        std::string enum_value_set;
        for (const auto& ev: token_map) {
          enum_value_set += ev.first;
          enum_value_set += " ";
        }

        delete tmp;
        throw std::string("The value '"
                          + v->get_token_value()
                          + "' is not among the enum value set. Accepted value for the key '"
                          + key
                          +"' is one of { "
                          + enum_value_set
                          + "}.");
      } else {
        auto result(mapped_enum_value->second);
        delete tmp;
        return result;
      }
    }
    
    template<typename value_type>
    value_type get_value(const std::string& key) const {
      using map_type = std::map<std::string, multi_value>;
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

      const basic_value* tmp(kv->second.get_value(selected_parameter_space)->eval(*this));
      const value<value_type>* v(dynamic_cast<const value<value_type>*>(tmp));
      if (not v)
        throw std::string("failed to get a "
                          + std::string(basic_value::type_names[get_index_of_element<value_type, basic_value::value_type_list>::value])
                          + " from the key '" + kv->first
                          + "' which has type " + kv->second.get_type(selected_parameter_space));

      value_type result(v->get_value());
      delete tmp;
      return result;
    }

    const basic_value* get_basic_value(const std::string& key) const {
      using map_type = std::map<std::string, multi_value>;
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

      return kv->second.get_value(selected_parameter_space);
    }

    void clear() {
      key_value.clear();
      parameter_space_sizes.clear();
      selected_parameter_space.clear();
    }

    void print_key_values(std::ostream& stream) const {
      for (const auto& kv: key_value)
        stream << kv.second.get_type(selected_parameter_space) << " " << kv.first
          << " = "
          << kv.second.print_value(selected_parameter_space) << std::endl;
    }
    
  private:
    std::map<std::string, multi_value> key_value;
    multi_index parameter_space_sizes;
    multi_index selected_parameter_space;

    struct key_value_definition {
      bool is_overriding;
      std::string key;
      multi_value mv;
      std::string coordinates;
    };

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
      using map_type = std::map<std::string, multi_value>;
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

    void set_key_value_group(const std::vector<key_value_definition>& defs) {
      if (defs.size()) {
        const std::size_t index_id(parameter_space_sizes.size());
        for (const auto& def: defs) {
          using map_type = std::map<std::string, multi_value>;
          using map_iterator_type = map_type::iterator;

          map_iterator_type kv(key_value.find(def.key));
          if (kv == key_value.end()) {
            (key_value[def.key] = def.mv).set_index_id(index_id);
          
            if (def.is_overriding)
              throw string_builder("attempt to redefine key '")(def.key)("' which is not (yet) defined.").str();

          } else {
            const std::size_t old_index_id(kv->second.get_index_id());
            kv->second = def.mv;
            kv->second.set_index_id(index_id);

            parameter_space_sizes[old_index_id] = 1;
              selected_parameter_space[old_index_id] = 0;

            if (not def.is_overriding)
              std::cerr << "warning: redefinition of key '" << def.key
                        << "' at " << def.coordinates << ". If it is the intended action, "
                        << "prefix the definition by the 'override' keyword." << std::endl;;
          }
        }

        std::size_t set_size(defs.front().mv.get_value_number());
        for (const auto& def: defs)
          if (def.mv.get_value_number() != set_size)
            throw string_builder("group definition has incoherent element number in definition near ")(def.coordinates)(".").str();
        
        parameter_space_sizes.push_back(set_size);
        selected_parameter_space.push_back(0ul);
      }
    }
    
    /* 
     * return false of initial definition, true on redefinition 
     */
    bool set_key_value(const std::string& key, basic_value* v) {
      return set_key_value(key, multi_value(0, v));
    }

    bool set_key_value(const std::string& key, const multi_value& mv) {
      using map_type = std::map<std::string, multi_value>;
      using map_iterator_type = map_type::iterator;

      map_iterator_type kv(key_value.find(key));
      if (kv == key_value.end()) {
        const std::size_t index_id(parameter_space_sizes.size());
        (key_value[key] = mv).set_index_id(index_id);
        
        parameter_space_sizes.push_back(mv.get_value_number());
        selected_parameter_space.push_back(0ul);

        return false;
      } else {
        const std::size_t index_id(kv->second.get_index_id());
        kv->second = mv;
        kv->second.set_index_id(index_id);

        parameter_space_sizes[index_id] = mv.get_value_number();
        selected_parameter_space[index_id] = 0;

        return true;
      }
    }
    
    void append_value(const std::string& key, basic_value* v) {
      using map_type = std::map<std::string, multi_value>;
      using map_iterator_type = map_type::iterator;

      map_iterator_type kv(key_value.find(key));
      if (kv == key_value.end()) {
        throw std::string("trying to append a value to an undefined key '") + key + "'";
      } else {
        const std::size_t index_id(kv->second.get_index_id());
        kv->second.append_value(v);

        parameter_space_sizes[index_id] += 1;
        selected_parameter_space[index_id] = 0;
      }
    }

    std::string enum_item_token_to_enum_item(token_type* t) {
      // remove the leading '#'
      std::string str(t->value.substr(1, t->value.size() - 1));
      return str;
    }
    
    std::string string_token_to_string(token_type* t) {
      // remove the quoting characters and escaped sequences
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
        case symbol::override_keyword:
          parse_global_definition(ts);
          break;
        case symbol::lbracket:
          parse_group_definition(ts);
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
    
    void parse_group_definition(token_source<token_type>& ts) {
      token_type* lbracket_token(ts.get());
      if (lbracket_token->symbol != symbol::lbracket)
        throw string_builder("unexpected ")
          (lbracket_token->symbol)
          (" token at ")
          (lbracket_token->render_coordinates())
          (" instead of a ")
          (symbol::lbracket)
          (" token").str();

      std::vector<key_value_definition> defs;

      bool done(false);
      while (not done) {
        token_type* current_token(ts.peek());

        switch (current_token->symbol) {
        case symbol::override_keyword:
        case symbol::key:
          defs.push_back(parse_key_value_definition(ts));
          break;

        case symbol::rbracket:
          delete ts.get();
          done = true;
          break;

        default:
          throw string_builder("unexpected ")
            (current_token->symbol)
            (" token at ")
            (current_token->render_coordinates()).str();
        }
      }

      set_key_value_group(defs);
      
      delete lbracket_token;
    }

    void parse_global_definition(token_source<token_type>& ts) {
      key_value_definition def(parse_key_value_definition(ts));

      const bool redefinition(set_key_value(def.key, def.mv));

      if (def.is_overriding and not redefinition)
        throw string_builder("attempt to redefine key '")(def.key)("' which is not (yet) defined at ")(def.coordinates)(".").str();

      if (redefinition and not def.is_overriding)
        std::cerr << "warning: redefinition of key '" << def.key
                  << "' at " << def.coordinates << ". If it is the intended action, "
                  << "prefix the definition by the 'override' keyword." << std::endl;
    }

    

    
    // FIRST(key_value) = {key, override_keyword}
    key_value_definition parse_key_value_definition(token_source<token_type>& ts) {
      key_value_definition def;
      def.is_overriding = false;
      
      token_type *override_keyword_token(ts.peek());
      if (override_keyword_token->symbol == symbol::override_keyword) {
        def.is_overriding = true;
        delete ts.get();
      }

      
      token_type
        *key_token(ts.get()),
        *equal_token(ts.get());

      if (equal_token->symbol != symbol::equal)
        throw string_builder("unexpected ")
          (key_token->symbol)
          (" token at ")
          (key_token->render_coordinates())
          (" instead of a ")
          (symbol::key).str();

      if (equal_token->symbol != symbol::equal)
        throw string_builder("unexpected ")
          (equal_token->symbol)
          (" token at ")
          (equal_token->render_coordinates())
          (" instead of a ")
          (symbol::equal).str();

      def.coordinates = equal_token->render_coordinates();
      def.key = key_token->value;

      token_type* value_token(ts.peek());
      switch (value_token->symbol) {
      case symbol::integer:
      case symbol::real:
      case symbol::boolean:
      case symbol::string:
      case symbol::enum_item:
      case symbol::key:
        def.mv = parse_value_list(ts);
        break;
        
      default:
        throw string_builder("unexpected ")
          (value_token->symbol)
          (" token at ")
          (value_token->render_coordinates()).str();
      }

      delete key_token;
      delete equal_token;

      return def;
    }

    multi_value parse_value_list(token_source<token_type>& ts) {
      multi_value v;
      
      bool done(false);
      while (not done) {
        token_type* current_token(ts.peek());

        switch (current_token->symbol) {
        case symbol::integer:
          v.append_value(parse_integer_value(ts));
          break;
          
        case symbol::real:
          v.append_value(parse_real_value(ts));
          break;
          
        case symbol::boolean:
          v.append_value(parse_boolean_value(ts));
          break;
          
        case symbol::string:
          v.append_value(parse_string_value(ts));
          break;
          
        case symbol::enum_item:
          v.append_value(parse_enum_item(ts));
          break;

        case symbol::key:
          v.append_value(parse_key_value(ts));
          break;
          
        default:
          throw string_builder("unexpected ")
          (current_token->symbol)
          (" token at ")
          (current_token->render_coordinates()).str();
        }
        
        token_type* comma_token(ts.peek());
        if (comma_token->symbol == symbol::comma)
          delete ts.get();
        else
          done = true;
      }

      return v;
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

    // FIRST(enum_token) = {enum_token}
    basic_value* parse_enum_item(token_source<token_type>& ts) {
      token_type* enum_item_token(ts.get());
      if (enum_item_token->symbol != symbol::enum_item)
        throw string_builder("unexpected ")
          (enum_item_token->symbol)
          (" token at ")
          (enum_item_token->render_coordinates())
          (" instead of a ")(symbol::real).str();

      basic_value* v(new ::parameter::enum_value(enum_item_token_to_enum_item(enum_item_token)));
      
      delete enum_item_token;

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

    basic_value* parse_key_value(token_source<token_type>& ts) {
      token_type* key_token(ts.get());
      if (key_token->symbol != symbol::key)
        throw string_builder("unexpected ")
          (key_token->symbol)(" token at ")
          (key_token->render_coordinates())(" instead of a ")(symbol::key).str();

      basic_value* v(new ::parameter::value_ref(key_token->value));

      delete key_token;

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
