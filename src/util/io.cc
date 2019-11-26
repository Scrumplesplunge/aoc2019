module;

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

export module io;

import <array>;
import <charconv>;
import <iomanip>;
import <stdexcept>;
import <string>;
import <string_view>;
import <sstream>;

using std::literals::operator""sv;

class contents_error final : public std::runtime_error {
 public:
  contents_error(const char* filename, const char* message)
      : runtime_error("Cannot retrieve contents of file \"" +
                      std::string(filename) + "\": " + message) {}
};

// Map the given file into memory. There is no cleanup done by default.
export std::string_view contents(const char* filename) {
  int fd = open(filename, O_RDONLY);
  if (fd < 0) throw contents_error(filename, "cannot open file.");
  struct stat info;
  if (fstat(fd, &info) < 0) {
    close(fd);
    throw contents_error(filename, "cannot stat file.");
  }
  const char* data =
      (const char*)mmap(nullptr, info.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);  // At this point we don't need the file descriptor any more.
  if (data == (caddr_t)-1) throw contents_error(filename, "cannot mmap file.");
  return std::string_view(data, info.st_size);
}

export struct exact { std::string_view value; };
export constexpr struct whitespace_type {} whitespace;

enum char_type : unsigned char {
  blank = 1,
  alpha = 2,
  digit = 4,
  punct = 8,
  lower = 16,
  upper = 32,
};

static constexpr auto type_map = [] {
  std::array<unsigned char, 128> map = {};
  for (char c : " \f\n\r\t\v"sv) map[c] = blank;
  for (char c = 'a'; c <= 'z'; c++) map[c] = alpha | lower;
  for (char c = 'A'; c <= 'Z'; c++) map[c] = alpha | upper;
  for (char c = '0'; c <= '9'; c++) map[c] = digit;
  for (char c : "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"sv) map[c] = punct;
  return map;
}();

export constexpr bool is_space(char c) { return type_map[c] & blank; }
export constexpr bool is_alpha(char c) { return type_map[c] & alpha; }
export constexpr bool is_digit(char c) { return type_map[c] & digit; }
export constexpr bool is_punct(char c) { return type_map[c] & punct; }
export constexpr bool is_lower(char c) { return type_map[c] & lower; }
export constexpr bool is_upper(char c) { return type_map[c] & upper; }

export template <auto predicate>
struct sequence { std::string_view& out; };

export class scanner {
 public:
  scanner(std::string_view source) : source_(source) {}

  template <typename Arithmetic,
            typename = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
  scanner& operator>>(Arithmetic& a) {
    // BUG: std::from_chars is supposed to behave like std::strtol, and
    // std::strtol is supposed to ignore leading whitespace, but unless I skip
    // the whitespace it fails to parse numbers.
    *this >> whitespace;
    auto [ptr, error] =
        std::from_chars(source_.data(), source_.data() + source_.size(), a);
    if (error != std::errc()) throw_error("expected arithmetic type.");
    advance(ptr - source_.data());
    return *this;
  }

  scanner& operator>>(exact e) {
    if (!source_.starts_with(e.value)) {
      std::ostringstream message;
      message << "expected literal string " << std::quoted(e.value) << ".";
      throw_error(message.str());
    }
    advance(e.value.size());
    return *this;
  }

  scanner& operator>>(char& c) {
    if (source_.empty()) throw_error("unexpected end of input.");
    c = source_.front();
    advance(1);
    return *this;
  }

  scanner& operator>>(whitespace_type) {
    const auto first = source_.data(), last = first + source_.size();
    const auto word_start = std::find_if_not(first, last, is_space);
    advance(word_start - first);
    return *this;
  }

  template <auto predicate>
  scanner& operator>>(sequence<predicate> s) {
    *this >> whitespace;
    if (source_.empty()) throw_error("unexpected end of input.");
    const auto word_start = source_.data(), last = word_start + source_.size();
    const auto word_end = std::find_if_not(word_start, last, predicate);
    if (word_start == word_end) throw_error("invalid input.");
    s.out = std::string_view(word_start, word_end - word_start);
    advance(word_end - word_start);
    return *this;
  }

  scanner& operator>>(std::string_view& v) {
    constexpr auto not_space = [](char c) { return !is_space(c); };
    return *this >> sequence<+not_space>{v};
  }

  std::string_view remaining() const { return source_; }
  std::string_view consume(std::size_t amount) {
    auto result = source_.substr(0, amount);
    source_.remove_prefix(result.size());
    return result;
  }

  int line() const { return line_; }
  int column() const { return column_; }

 private:
  void advance(std::size_t amount) {
    assert(amount <= source_.length());
    for (char c : source_.substr(0, amount)) {
      if (c == '\n') {
        line_++;
        column_ = 1;
      } else {
        column_++;
      }
    }
    source_.remove_prefix(amount);
  }

  void throw_error(std::string_view message) {
    const auto line_start = source_.data() - (column_ - 1);
    const auto line_end =
        std::find(source_.data(), source_.data() + source_.size(), '\n');
    const auto line_contents =
        std::string_view(line_start, line_end - line_start);
    std::ostringstream output;
    output << line_ << ':' << column_ << ": " << message << "\n"
           << "    " << line_contents << "\n"
           << std::string(3 + column_, ' ') << "^\n";
    throw std::runtime_error(output.str());
  }

  std::string_view source_;
  int line_ = 1, column_ = 1;
};
