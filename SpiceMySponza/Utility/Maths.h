#pragma once

#if !defined    _UTIL_MATHS_
#define         _UTIL_MATHS_


// STL headers.
#include <cmath>
#include <type_traits>


namespace util
{
    #pragma region Miscellaneous

    /// <summary> Checks if two float values are relatively equal to each other. </summary>
    /// <param name="margin"> The absolute margin of error between the two floats. Must be a positive value. </param>
    inline bool roughlyEquals (const float lhs, const float rhs, const float margin = 0.1f)
    {
        // Test the upper and lower limits.
        return std::abs (lhs - rhs) <= margin;
    }

    #pragma endregion

    #pragma region Comparison functions

    /// <summary> Returns the minimum value, passed by value for arithmetic types. </summary>
    template <typename T> typename std::enable_if<std::is_arithmetic<T>::value, T>::type min (const T a, const T b)
    {
        return a < b ? a : b;
    }
    

    /// <summary> Returns the maximum value, passed by value for arithmetic types. </summary>
    template <typename T> typename std::enable_if<std::is_arithmetic<T>::value, T>::type max (const T a, const T b)
    {
        return a > b ? a : b;
    }


    /// <summary> Returns the minimum value, passed by reference for non-arithmetic types. </summary>
    template <typename T> typename std::enable_if<!std::is_arithmetic<T>::value, T>::type& min (const T& a, const T& b)
    {
        return a < b ? a : b;
    }


    /// <summary> Returns the maximum value, passed by reference for non-arithmetic types. </summary>
    template <typename T> typename std::enable_if<!std::is_arithmetic<T>::value, T>::type& max (const T& a, const T& b)
    {
        return a > b ? a : b;
    }
    

    /// <summary> Clamps a value between a given minimum and maximum value. Arithmetic types are passed by value. </summary>
    /// <param name="value"> The value to clamp. </param>
    template <typename T> typename std::enable_if<std::is_arithmetic<T>::value, T>::type clamp (const T value, const T min, const T max)
    {
        if (value < min)
        {
            return min;
        }

        if (value > max)
        {
            return max;
        }

        return value;
    }


    /// <summary> Clamps a value between a given minimum and maximum value. Non-arithmetic types are passed by reference. </summary>
    /// <param name="value"> The value to clamp. </param>
    template <typename T> typename std::enable_if<!std::is_arithmetic<T>::value, T>::type clamp (const T& value, const T& min, const T& max)
    {
        if (value < min)
        {
            return min;
        }

        if (value > max)
        {
            return max;
        }

        return value;
    }

    #pragma endregion
}

#endif // _UTIL_MATHS_