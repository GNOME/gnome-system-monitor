#pragma once

#include <cstddef>
#include <iterator>
#include <numeric>
#include <type_traits>

namespace procman
{

template<typename T>
concept non_array = !std::is_array_v<T>;

// The size calculation would be wrong for a string literal as a separator, so
// require the type is not an array to avoid that case. Users can pass in a
// `std::string_view` to properly wrap their string literals.
template<typename Result>
auto
join (const auto& ranges, const non_array auto& separator) -> Result
{
  auto r = Result ();
  if (std::size (ranges) == 0)
    {
      return r;
    }
  const auto inner_sizes = std::accumulate (
    std::begin (ranges),
    std::end (ranges),
    std::size_t (0),
    [](const std::size_t count, const auto& inner) { return count + std::size (inner); }
  );
  const auto number_of_separators = std::size (ranges) - 1;
  r.reserve (inner_sizes + number_of_separators * std::size (separator));
  bool first = true;

  for (const auto& range : ranges)
    {
      if (!first)
        {
          r.append (std::begin (separator), std::end (separator));
        }
      first = false;
      r.append (std::begin (range), std::end (range));
    }

  return r;
}

} // namespace procman
