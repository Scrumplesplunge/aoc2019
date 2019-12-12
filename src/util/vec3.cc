export module util.vec3;

import <type_traits>;

export template <typename T>
struct vec3 {
  constexpr vec3() = default;

  template <typename X, typename Y, typename Z,
            typename = std::enable_if_t<std::is_convertible_v<X, T> &&
                                        std::is_convertible_v<Y, T> &&
                                        std::is_convertible_v<Z, T>>>
  constexpr vec3(X x, Y y, Z z) : x(x), y(y), z(z) {}

  constexpr vec3(vec3&& other) = default;
  constexpr vec3(const vec3& other) = default;
  constexpr vec3& operator=(vec3&& other) = default;
  constexpr vec3& operator=(const vec3& other) = default;

  template <typename U>
  constexpr explicit vec3(vec3<U> other) : x(other.x), y(other.y), z(other.z) {}

  constexpr auto manhattan_length() const {
    constexpr auto abs = [](auto x) { return x < 0 ? -x : x; };
    return abs(x) + abs(y) + abs(z);
  }

  T x = {}, y = {}, z = {};
};

export using vec3i = vec3<int>;

// BUG: Currently (2019-11-14) this exported deduction guide doesn't seem to
// work :( Adding the "export" keyword in front of the class declaration above
// seems to make this line stop having any effect. I'll leave it here just in
// case it starts working with newer compilers.
export template <typename T>
vec3(T, T) -> vec3<T>;

export template <typename T>
constexpr auto operator+(vec3<T> a, vec3<T> b) {
  return vec3<T>{a.x + b.x, a.y + b.y, a.z + b.z};
}

export template <typename T>
constexpr auto operator-(vec3<T> a, vec3<T> b) {
  return vec3<T>{a.x - b.x, a.y - b.y, a.z - b.z};
}

export template <typename S, typename T>
constexpr auto operator*(S s, vec3<T> v) {
  return vec3<T>{s * v.x, s * v.y, s * v.z};
}

export template <typename S, typename T>
constexpr auto operator*(vec3<T> v, S s) {
  return vec3<T>{v.x * s, v.y * s, v.z * s};
}

export template <typename S, typename T>
constexpr auto operator/(vec3<T> v, S s) {
  return vec3<T>{v.x / s, v.y / s, v.z / s};
}

export template <typename T>
constexpr bool operator==(vec3<T> a, vec3<T> b) {
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

export template <typename T, typename U>
constexpr vec3<T>& operator+=(vec3<T>& a, vec3<U> b) {
  a.x += b.x;
  a.y += b.y;
  a.z += b.z;
  return a;
}

export template <typename T, typename U>
constexpr vec3<T>& operator-=(vec3<T>& a, vec3<U> b) {
  a.x -= b.x;
  a.y -= b.y;
  a.z -= b.z;
  return a;
}

export template <typename T, typename S>
constexpr vec3<T>& operator*=(vec3<T>& a, S s) {
  a.x *= s;
  a.y *= s;
  a.z *= s;
  return a;
}

export template <typename T, typename S>
constexpr vec3<T>& operator/=(vec3<T>& a, S s) {
  a.x /= s;
  a.y /= s;
  a.z /= s;
  return a;
}

export template <typename T>
constexpr bool operator<(vec3<T> a, vec3<T> b) {
  if (a.z < b.z) return true;
  if (a.z > b.z) return false;
  if (a.y < b.y) return true;
  if (a.y > b.y) return false;
  return a.x < b.x;
}

export template <typename T>
constexpr bool operator!=(vec3<T> a, vec3<T> b) { return !(a == b); }

export template <typename T>
constexpr bool operator>(vec3<T> a, vec3<T> b) { return b < a; }

export template <typename T>
constexpr bool operator<=(vec3<T> a, vec3<T> b) { return !(b < a); }

export template <typename T>
constexpr bool operator>=(vec3<T> a, vec3<T> b) { return !(a < b); }

export template <typename T>
struct std::hash<vec3<T>> {
  constexpr auto operator()(vec3<T> v) const {
    using word = std::size_t;
    return (word(v.x) * 19) ^ (word(v.y) * 37) ^ (word(v.z) * 53);
  }
};
