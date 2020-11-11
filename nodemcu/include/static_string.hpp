#ifndef IOP_STATIC_STRING_H_
#define IOP_STATIC_STRING_H_

class StaticString {
private:
  const char * str;

public:
  constexpr StaticString(const char * string): str(string) {}
  constexpr StaticString(const StaticString& other): str(other.str) {}
  void operator=(const StaticString& other) { this->str = other.str; }
  void operator=(const StaticString&& other) { this->str = other.str; }
  constexpr const char * const get() const { return this->str; }
  constexpr const char * const operator->() const { return this->get(); }
  constexpr const char * const operator*() const { return this->get(); }
};

#define STATIC_STRING(msg) StaticString(PSTR(msg))

#endif