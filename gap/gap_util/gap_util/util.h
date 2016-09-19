///==============================================================================
/// GAP (General Autonomous Parallelizer) License
///==============================================================================
///
/// GAP is distributed under the following BSD-style license:
///
/// Copyright (c) 2016 Dzanan Bajgoric
/// All rights reserved.
/// 
/// Redistribution and use in source and binary forms, with or without modification,
/// are permitted provided that the following conditions are met:
/// 
/// 1. Redistributions of source code must retain the above copyright notice, this
///    list of conditions and the following disclaimer.
/// 
/// 2. Redistributions in binary form must reproduce the above copyright notice, this
///    list of conditions and the following disclaimer in the documentation and/or other
///    materials provided with the distribution.
/// 
/// 3. The name of the author may not be used to endorse or promote products derived from
///    this software without specific prior written permission from the author.
/// 
/// 4. Products derived from this software may not be called "GAP" nor may "GAP" appear
///    in their names without specific prior written permission from the author.
/// 
/// THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING,
/// BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
/// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
/// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO
/// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
/// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
/// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
/// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GAP_UTIL_UTIL_H
#define GAP_UTIL_UTIL_H

#include <cmath>

namespace gap
{
namespace util
{

	/**
	 * @brief epsilon value used when comparing floating point types.
	 */
	#define C2CUDA_EPSILON 1e-12

	/**
	 * @brief Rational<T> class template declaration - for definition @see rational_number.hpp.
	 */
	template<typename T>
	class Rational;

	/**
	 * @brief returns sign of a number.
	 */
	template<typename T>
	inline T signum(T val)
	{
		return (T(0) < val) - (val < T(0));
		//return val >= 0 ? 1 : -1;
	}

	/**
	 * @brief returns sign of a rational number.
	 */
	template<typename T>
	inline Rational<T> signum(const Rational<T>& val)
	{
		return (Rational<T>(0) < val) - (val < Rational<T>(0));
		//return val >= T(0) ? 1 : -1;
	}

	/**
	 * @brief checks if two double precision floating point values are (almost) equal.
	 */
	inline bool equal(const long double value1, const long double value2)
	{
		return std::abs(value1 - value2) < C2CUDA_EPSILON;
	}

	/**
	 * @brief checks if two double precision floating point values are not equal.
	 */
	inline bool not_equal(const long double value1, const long double value2)
	{
		return !equal(value1, value2);
	}

	/**
	 * @brief lower-then for double precision floating point values.
	 */
	inline bool lower(const long double value1, const long double value2)
	{
		return !equal(value1, value2) && value2 - value1 > 0.;
	}

	/**
	 * @brief lower-then or equal for double precision floating point values.
	 */
	inline bool lower_equal(const long double value1, const long double value2)
	{
		return equal(value1, value2) || value2 - value1 > 0.;
	}

	/**
	* @brief greater-then for double precision floating point values.
	*/
	inline bool greater(const long double value1, const long double value2)
	{
		return !equal(value1, value2) && value1 - value2 > 0.;
	}

	/**
	* @brief greater-then or equal for double precision floating point values.
	*/
	inline bool greater_equal(const long double value1, const long double value2)
	{
		return equal(value1, value2) || value1 - value2 > 0.;
	}

} /// namespace util
} /// namespace gap


namespace std
{

	/**
	 * @brief returns absolute value of the rational number.
	 */
	template<typename T>
	inline gap::util::Rational<T> abs(const gap::util::Rational<T>& value)
	{
		return { abs(value.numerator()), abs(value.denominator()) };
	}

	/**
	 * @brief returns a new rational number that has been rounded down to nearest integer.
	 * @example floor(3/7) => 0/1, floor(9/4) => 2/1, floor(20/5) => 4/1
	 *
	 * @note maybe it should return double to respect std::ceil(integral_type) overload
	 */
	template<typename T>
	inline T floor(const gap::util::Rational<T>& value)
	{
		T mul = value.numerator() >= 0 ? 0 : 1;

		/** No need to cast to integral type as Rational<T> template can be instantiated only with integral argument. */
		return (value.numerator() - mul * (value.denominator() - T(1))) / value.denominator();
	}

	/**
	 * @brief returns a new rational number that has been rounded up to nearest integer.
	 * @example ceil(3/7) => 1/1, ceil(9/4) => 3/1, ceil(20/5) => 4/1
	 *
	 *  @note maybe it should return double to respect std::ceil(integral_type) overload
	 */
	template<typename T>
	inline T ceil(const gap::util::Rational<T>& value)
	{
		T mul = value.numerator() >= 0 ? 1 : 0;

		/** No need to cast to integral type as Rational<T> template can be instantiated only with integral argument. */
		return (value.numerator() + mul * (value.denominator() - T(1))) / value.denominator();
	}

} /// namespace std

#endif /// GAP_UTIL_UTIL_H
