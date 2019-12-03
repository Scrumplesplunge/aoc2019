module;

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

export module io;

import <array>;
import <charconv>;
import <iomanip>;
import <iostream>;
import <optional>;
import <span>;
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

export class scanner_error : public std::runtime_error {
 public:
  using runtime_error::runtime_error;
};

export template <auto predicate, typename T>
struct match_type { T& out; };

export template <auto predicate, typename T>
auto matches(T& out) { return match_type<predicate, T>{out}; }

export template <auto predicate>
struct sequence_type { std::string_view& out; };

export template <auto predicate>
auto sequence(std::string_view& out) { return sequence_type<predicate>{out}; }

export class scanner {
 public:
  struct end_type {};
  static constexpr end_type end;

  scanner(std::string_view source) : source_(source) {}

  bool ok() const { return !error_; }
  operator bool() const { return ok() && !done(); }
  std::string_view error() const { return error_ ? *error_ : ""; }

  void check_ok() const { if (!ok()) throw scanner_error(*error_); }

  template <typename Arithmetic,
            typename = std::enable_if_t<std::is_arithmetic_v<Arithmetic>>>
  [[nodiscard]] scanner& operator>>(Arithmetic& a) {
    if (error_.has_value()) return *this;
    // BUG: std::from_chars is supposed to behave like std::strtol, and
    // std::strtol is supposed to ignore leading whitespace, but unless I skip
    // the whitespace it fails to parse numbers.
    *this >> whitespace;
    auto [ptr, error] =
        std::from_chars(source_.data(), source_.data() + source_.size(), a);
    if (error != std::errc()) return set_error("expected arithmetic type.");
    advance(ptr - source_.data());
    return *this;
  }

  [[nodiscard]] scanner& operator>>(exact e) {
    if (error_.has_value()) return *this;
    if (!source_.starts_with(e.value)) {
      std::ostringstream message;
      message << "expected literal string " << std::quoted(e.value) << ".";
      return set_error(message.str());
    }
    advance(e.value.size());
    return *this;
  }

  [[nodiscard]] scanner& operator>>(char& c) {
    if (error_.has_value()) return *this;
    if (source_.empty()) return set_error("unexpected end of input.");
    c = source_.front();
    advance(1);
    return *this;
  }

  scanner& operator>>(whitespace_type) {
    if (error_.has_value()) return *this;
    const auto first = source_.data(), last = first + source_.size();
    const auto word_start = std::find_if_not(first, last, is_space);
    advance(word_start - first);
    return *this;
  }

  template <auto predicate, typename T>
  [[nodiscard]] scanner& operator>>(match_type<predicate, T> m) {
    if (error_.has_value()) return *this;
    *this >> whitespace;
    location l{source_, line_, column_};
    if (!(*this >> m.out)) return *this;
    if (!predicate(m.out)) return set_error(l, "invalid input.");
    return *this;
  }

  template <auto predicate>
  [[nodiscard]] scanner& operator>>(sequence_type<predicate> s) {
    if (error_.has_value()) return *this;
    *this >> whitespace;
    if (source_.empty()) return set_error("unexpected end of input.");
    const auto word_start = source_.data(), last = word_start + source_.size();
    const auto word_end = std::find_if_not(word_start, last, predicate);
    if (word_start == word_end) return set_error("invalid input.");
    s.out = std::string_view(word_start, word_end - word_start);
    advance(word_end - word_start);
    return *this;
  }

  [[nodiscard]] scanner& operator>>(std::string_view& v) {
    if (error_.has_value()) return *this;
    constexpr auto not_space = [](char c) { return !is_space(c); };
    return *this >> sequence<+not_space>(v);
  }

  template <typename T>
  [[nodiscard]] scanner& operator>>(std::span<T>& s) {
    if (error_.has_value()) return *this;
    std::size_t count = 0;
    try {
      for (T& t : s) {
        *this >> t;
        count++;
      }
    } catch (const scanner_error& error) {}
    s = s.subspan(0, count);
    return *this;
  }

  [[nodiscard]] scanner& operator>>(end_type) {
    if (error_.has_value()) return *this;
    *this >> whitespace;
    if (!source_.empty()) {
      return set_error("trailing characters after expected end of input.");
    }
    return *this;
  }

  bool done() const {
    const auto first = source_.data(), last = first + source_.size();
    return std::find_if_not(first, last, is_space) == last;
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
  struct location {
    std::string_view source;
    int line = 1, column = 1;
  };

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

  scanner& set_error(location l, std::string_view message) {
    const auto line_start = l.source.data() - (l.column - 1);
    const auto line_end =
        std::find(l.source.data(), l.source.data() + l.source.size(), '\n');
    const auto line_contents =
        std::string_view(line_start, line_end - line_start);
    std::ostringstream output;
    output << l.line << ':' << l.column << ": " << message << "\n"
           << "    " << line_contents << "\n"
           << std::string(3 + l.column, ' ') << "^\n";
    error_ = output.str();
    return *this;
  }

  scanner& set_error(std::string_view message) {
    return set_error(location{source_, line_, column_}, message);
  }

  std::optional<std::string> error_;
  std::string_view source_;
  int line_ = 1, column_ = 1;
};

export std::string_view init(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <filename>\n";
    std::exit(1);
  }
  return contents(argv[1]);
}
