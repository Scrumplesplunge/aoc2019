export module util.vec2;

import <iostream>;
import <type_traits>;

export template <typename T>
struct vec2 {
  constexpr vec2() = default;

  template <typename X, typename Y,
            typename = std::enable_if_t<
                std::is_convertible_v<X, T> && std::is_convertible_v<Y, T>>>
  constexpr vec2(X x, Y y) : x(x), y(y) {}

  constexpr vec2(vec2&& other) = default;
  constexpr vec2(const vec2& other) = default;
  constexpr vec2& operator=(vec2&& other) = default;
  constexpr vec2& operator=(const vec2& other) = default;

  template <typename U>
  constexpr explicit vec2(vec2<U> other) : x(other.x), y(other.y) {}

  constexpr auto manhattan_length() const {
    constexpr auto abs = [](auto x) { return x < 0 ? -x : x; };
    return abs(x) + abs(y);
  }

  T x = {}, y = {};
};

export using vec2i = vec2<int>;

// BUG: Currently (2019-11-14) this exported deduction guide doesn't seem to
// work :( Adding the "export" keyword in front of the class declaration above
// seems to make this line stop having any effect. I'll leave it here just in
// case it starts working with newer compilers.
export template <typename T>
vec2(T, T) -> vec2<T>;

export template <typename T>
constexpr auto operator+(vec2<T> a, vec2<T> b) {
  return vec2<T>{a.x + b.x, a.y + b.y};
}

export template <typename T>
constexpr auto operator-(vec2<T> a, vec2<T> b) {
  return vec2<T>{a.x - b.x, a.y - b.y};
}

export template <typename S, typename T>
constexpr auto operator*(S s, vec2<T> v) {
  return vec2<T>{s * v.x, s * v.y};
}

export template <typename S, typename T>
constexpr auto operator*(vec2<T> v, S s) {
  return vec2<T>{v.x * s, v.y * s};
}

export template <typename S, typename T>
constexpr auto operator/(vec2<T> v, S s) {
  return vec2<T>{v.x / s, v.y / s};
}

export template <typename T, typename U>
constexpr vec2<T>& operator+=(vec2<T>& l, vec2<U> r) {
  l.x += r.x;
  l.y += r.y;
  return l;
}

export template <typename T, typename U>
constexpr vec2<T>& operator-=(vec2<T>& l, vec2<U> r) {
  l.x -= r.x;
  l.y -= r.y;
  return l;
}

export template <typename T, typename S>
constexpr vec2<T>& operator*=(vec2<T>& l, S r) {
  l.x *= r;
  l.y *= r;
  return l;
}

export template <typename T, typename S>
constexpr vec2<T>& operator/=(vec2<T>& l, S r) {
  l.x /= r;
  l.y /= r;
  return l;
}

export template <typename T>
constexpr bool operator==(vec2<T> a, vec2<T> b) {
  return a.x == b.x && a.y == b.y;
}

export template <typename T>
constexpr bool operator<(vec2<T> a, vec2<T> b) {
  if (a.y < b.y) return true;
  if (a.y > b.y) return false;
  return a.x < b.x;
}

export template <typename T>
constexpr bool operator!=(vec2<T> a, vec2<T> b) { return !(a == b); }

export template <typename T>
constexpr bool operator>(vec2<T> a, vec2<T> b) { return b < a; }

export template <typename T>
constexpr bool operator<=(vec2<T> a, vec2<T> b) { return !(b < a); }

export template <typename T>
constexpr bool operator>=(vec2<T> a, vec2<T> b) { return !(a < b); }

export template <typename T>
struct std::hash<vec2<T>> {
  constexpr auto operator()(vec2<T> v) const {
    return (v.x * 19) ^ (v.y * 37);
  }
};

export template <typename T>
std::ostream& operator<<(std::ostream& output, vec2<T> v) {
  return output << "(" << v.x << ", " << v.y << ")";
}
