#pragma once

#include <array>
#include <stdexcept>
#include <string>
#include <vector>

using Degrees = double;
using Pixels = signed int;

constexpr const std::array<std::string_view, 4> k_edge_names = { "left", "right", "top", "bottom" };

constexpr static const int LEFT_EDGE = 0;
constexpr static const int RIGHT_EDGE = 1;
constexpr static const int TOP_EDGE = 2;
constexpr static const int BOTTOM_EDGE = 3;

template<typename T>
class Rect
{
public:
  T left{ 0 };
  T right{ 0 };
  T top{ 0 };
  T bottom{ 0 };

  std::array<T, 4> GetArray() const { return { left, right, top, bottom }; }
  T GetWidth() { return std::abs(right - left); }
  T GetHeight() { return std::abs(top - bottom); }

  // TODO: When I update to CPP23.
  //  decltype(auto) operator[](this auto& self, std::size_t idx) { return T(5); }

  T& operator[](std::size_t idx)
  {
    switch (idx) {
      case 0:
        return left;
        break;
      case 1:
        return right;
        break;
      case 2:
        return top;
        break;
      case 3:
        return bottom;
        break;
      default:
        throw std::out_of_range("Index out of range.");
    }
  }
  const T& operator[](std::size_t idx) const
  {
    switch (idx) {
      case 0:
        return left;
        break;
      case 1:
        return right;
        break;
      case 2:
        return top;
        break;
      case 3:
        return bottom;
        break;
      default:
        throw std::out_of_range("Index out of range.");
    }
  }
};

using RectDegrees = Rect<Degrees>;
using RectPixels = Rect<Pixels>;
using RectShort = Rect<double>;

template<typename T>
class Point
{
public:
  T x;
  T y;

  Point(T x_, T y_)
    : x(x_)
    , y(y_)
  {
  }

  Point()
    : x(0)
    , y(0)
  {
  }

  bool operator==(const Point& other) const { return (x == other.x && y == other.y); }
};

// Am I using this?
// using HandleFunction = void (*)(Degrees, Degrees);
