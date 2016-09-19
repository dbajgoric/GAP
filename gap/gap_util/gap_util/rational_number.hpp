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

#ifndef GAP_UTIL_RATIONAL_NUMBER_H
#define GAP_UTIL_RATIONAL_NUMBER_H

#include <armadillo/armadillo>
#include <stdexcept>
#include <type_traits>
#include <limits>
#include "column_vector.h"
#include "util.h"


namespace gap
{
namespace util
{

	template<typename T>
	class Rational
	{
		static_assert(std::is_integral<T>::value, "Rational class template can be instantiated with integral types only!");

	public:

		template<typename U>
		friend class Rational;

		Rational() : m_numerator(T(0)), m_denominator(T(1))
		{
		}

		Rational(const arma::arma_rng::randu<Rational<T>>& rand_val) :
			m_numerator(T(0)), m_denominator(T(1)) { }

		Rational(const arma::arma_rng::randn<Rational<T>>& rand_val) :
			m_numerator(T(0)), m_denominator(T(1)) { }

		template<typename U>
		Rational(const U numerator_, const U denominator_ = U(1)) :
			m_numerator(T(numerator_)), m_denominator(T(denominator_))
		{
			if (m_denominator == 0)
			{
				throw std::runtime_error("Rational::Rational(): denominator mustn't be equal to 0");
			}
			else if (m_denominator < 0)
			{
				/** When denominator is negative, multiply numerator and denominator with -1 (this simplifies comparison operators.		*/
				m_numerator *= -1;
				m_denominator *= -1;
			}
			else if (m_numerator == 0)
			{
				m_denominator = 1;
			}

			/**																															*/
			/* Once numerator and/or denominator exceed certain size limit, attempt to normalize numerator and denominator by dividing	*/
			/* dividing both with GCD(nom, den).																						*/
			/*																															*/
			/*if (m_numerator >= T(0.01 * std::numeric_limits<T>::max()) || m_denominator >= T(0.01 * std::numeric_limits<T>::max()))
			{*/
				T gcd_val = gcd(ColVector<T>({ m_numerator, m_denominator }));
				m_numerator /= gcd_val;
				m_denominator /= gcd_val;
			//}
		}

		template<typename U>
		Rational(const Rational<U>& other) :
			m_numerator(static_cast<T>(other.m_numerator)), m_denominator(static_cast<T>(other.m_denominator))
		{
		}

		template<typename U>
		Rational<T>& operator=(const Rational<U>& other)
		{
			this->m_numerator = static_cast<T>(other.m_numerator);
			this->m_denominator = static_cast<T>(other.m_denominator);
			return *this;
		}

		operator T() const
		{
			if (static_cast<int>(std::abs(m_denominator)) != 1)
				throw std::runtime_error("Rational::operator T(): rational can be cast to integral type only when denominator is 1");

			return m_numerator * m_denominator;
		}

		operator long double() const
		{
			return double(m_numerator) / m_denominator;
		}

		operator float() const
		{
			return static_cast<float>(m_numerator) / m_denominator;
		}

		/*******************************************************************************************************************************/
		/*************************************************** Arithmetic operators ******************************************************/
		template<typename U>
		Rational& operator+=(const Rational<U>& other)
		{
			*this = *this + Rational<T>(other);
			return *this;
		}

		template<typename U>
		Rational& operator+=(const U other)
		{
			*this = *this + Rational<T>(other);
			return *this;
		}

		template<typename U>
		Rational& operator-=(const Rational<U>& other)
		{
			*this = *this - Rational<T>(other);
			return *this;
		}

		template<typename U>
		Rational& operator-=(const U other)
		{
			*this = *this - Rational<T>(other);
			return *this;
		}

		template<typename U>
		Rational& operator*=(const Rational<U>& other)
		{
			*this = *this * Rational<T>(other);
			return *this;
		}

		template<typename U>
		Rational& operator*=(const U other)
		{
			*this = *this * Rational<T>(other);
			return *this;
		}

		template<typename U>
		Rational& operator/=(const Rational<U>& other)
		{
			*this = *this / Rational<T>(other);
			return *this;
		}

		template<typename U>
		Rational& operator/=(const U other)
		{
			*this = *this / Rational<T>(other);
			return *this;
		}

		/*******************************************************************************************************************************/
		/**************************************************** Insertion operator *******************************************************/
		friend std::ostream& operator<<(std::ostream& out, const Rational& value)
		{
			out << "(" << value.m_numerator << " / " << value.m_denominator << ")";
			return out;
		}

		/*******************************************************************************************************************************/
		/********************************************************** Getters ************************************************************/
		T numerator() const { return m_numerator; }
		T denominator() const { return m_denominator; }

	private:

		T m_numerator;
		T m_denominator;
	};


	/*******************************************************************************************************************************/
	/********************* Free standing arithmetic operator function templates for Rational class template. ***********************/

	template<typename T1, typename T2>
	inline Rational<typename std::common_type<T1, T2>::type> operator+(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		return { v1.numerator() * v2.denominator() + v2.numerator() * v1.denominator(), v1.denominator() * v2.denominator() };
	}

	template<typename T>
	inline Rational<T> operator+(const Rational<T>& v1, const T v2)
	{
		return { v1.numerator() + v2 * v1.denominator(), v1.denominator() };
	}

	template<typename T>
	inline Rational<T> operator+(const T v1, const Rational<T>& v2)
	{
		return { v1 * v2.denominator() + v2.numerator(), v2.denominator() };
	}

	template<typename T1, typename T2>
	inline Rational<typename std::common_type<T1, T2>::type> operator-(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		return { v1.numerator() * v2.denominator() - v2.numerator() * v1.denominator(), v1.denominator() * v2.denominator() };
	}

	template<typename T>
	inline Rational<T> operator-(const Rational<T>& v1, const T v2)
	{
		return { v1.numerator() - v2 * v1.denominator(), v1.denominator() };
	}

	template<typename T>
	inline Rational<T> operator-(const T v1, const Rational<T>& v2)
	{
		return { v1 * v2.denominator() - v2.numerator(), v2.denominator() };
	}

	template<typename T1, typename T2>
	inline Rational<typename std::common_type<T1, T2>::type> operator*(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		return { v1.numerator() * v2.numerator(), v1.denominator() * v2.denominator() };
	}

	template<typename T>
	inline Rational<T> operator*(const Rational<T>& v1, const T v2)
	{
		return { v1.numerator() * v2, v1.denominator() };
	}

	template<typename T>
	inline Rational<T> operator*(const T v1, const Rational<T>& v2)
	{
		return { v1 * v2.numerator(), v2.denominator() };
	}

	template<typename T1, typename T2>
	inline Rational<typename std::common_type<T1, T2>::type> operator/(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		return { v1.numerator() * v2.denominator(), v1.denominator() * v2.numerator() };
	}

	template<typename T>
	inline Rational<T> operator/(const Rational<T>& v1, const T v2)
	{
		return { v1.numerator(), v1.denominator() * v2 };
	}

	template<typename T>
	inline Rational<T> operator/(const T v1, const Rational<T>& v2)
	{
		return { v1 * v2.denominator(), v2.numerator() };
	}


	/*******************************************************************************************************************************/
	/************************ Free standing logical operator function templates for Rational class template. ***********************/
	/**																															   */
	/* Note: because of the possibility of overflow when multiplying numerators and denominators of two rationals, comparison is   */
	/* done by first converting Rationals to doubles. Doubles are considered equal when difference between them is < 10^-8.		   */
	/*																															   */

	template<typename T1, typename T2>
	inline bool operator<(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		//return v1.numerator() * v2.denominator() < v2.numerator() * v1.denominator();
		return lower(v1, v2);
	}

	template<typename T>
	inline bool operator<(const Rational<T>& v1, const T v2)
	{
		//return v1.numerator() < v2 * v1.denominator();
		return lower(v1, v2);
	}

	template<typename T>
	inline bool operator<(const T v1, const Rational<T>& v2)
	{
		//return v1 * v2.denominator() < v2.numerator();
		return lower(v1, v2);
	}

	template<typename T1, typename T2>
	inline bool operator>(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		//return v1.numerator() * v2.denominator() > v2.numerator() * v1.denominator();
		return greater(v1, v2);
	}

	template<typename T>
	inline bool operator>(const Rational<T>& v1, const T v2)
	{
		//return v1.numerator() > v2 * v1.denominator();
		return greater(v1, v2);
	}

	template<typename T>
	inline bool operator>(const T v1, const Rational<T>& v2)
	{
		//return v1 * v2.denominator() > v2.numerator();
		return greater(v1, v2);
	}

	template<typename T1, typename T2>
	inline bool operator==(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		//return v1.numerator() * v2.denominator() == v2.numerator() * v1.denominator();
		return equal(v1, v2);
	}

	template<typename T>
	inline bool operator==(const Rational<T>& v1, const T v2)
	{
		//return v1.numerator() == v2 * v1.denominator();
		return equal(v1, v2);
	}

	template<typename T>
	inline bool operator==(const T v1, const Rational<T>& v2)
	{
		//return v1 * v2.denominator() == v2.numerator();
		return equal(v1, v2);
	}

	template<typename T1, typename T2>
	inline bool operator<=(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		//return v1.numerator() * v2.denominator() <= v2.numerator() * v1.denominator();
		return lower_equal(v1, v2);
	}

	template<typename T>
	inline bool operator<=(const Rational<T>& v1, const T v2)
	{
		//return v1.numerator() <= v2 * v1.denominator();
		return lower_equal(v1, v2);
	}

	template<typename T>
	inline bool operator<=(const T v1, const Rational<T>& v2)
	{
		//return v1 * v2.denominator() <= v2.numerator();
		return lower_equal(v1, v2);
	}

	template<typename T1, typename T2>
	inline bool operator>=(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		//return v1.numerator() * v2.denominator() >= v2.numerator() * v1.denominator();
		return greater_equal(v1, v2);
	}

	template<typename T>
	inline bool operator>=(const Rational<T>& v1, const T v2)
	{
		//return v1.numerator() >= v2 * v1.denominator();
		return greater_equal(v1, v2);
	}

	template<typename T>
	inline bool operator>=(const T v1, const Rational<T>& v2)
	{
		//return v1 * v2.denominator() >= v2.numerator();
		return greater_equal(v1, v2);
	}

	template<typename T1, typename T2>
	inline bool operator!=(const Rational<T1>& v1, const Rational<T2>& v2)
	{
		//return v1.numerator() * v2.denominator() != v2.numerator() * v1.denominator();
		return not_equal(v1, v2);
	}

	template<typename T>
	inline bool operator!=(const Rational<T>& v1, const T v2)
	{
		//return v1.numerator() != v2 * v1.denominator();
		return not_equal(v1, v2);
	}

	template<typename T>
	inline bool operator!=(const T v1, const Rational<T>& v2)
	{
		//return v1 * v2.denominator() != v2.numerator();
		return not_equal(v1, v2);
	}

} /// namespace util
} /// namespace gap


namespace std
{

	/// Template specialization for std::numeric_limits for Rational type
	struct _Rational_base
		: public _Num_base
	{	/// base for rational types
		_STCONS(bool, is_bounded, true);
		_STCONS(bool, is_exact, true);
		_STCONS(bool, is_integer, false);
		_STCONS(bool, is_modulo, false);
		_STCONS(bool, is_specialized, true);
		_STCONS(int, radix, 2);
	};

	template<> class numeric_limits<gap::util::Rational<int>>
		: public _Rational_base
	{
	public:
		typedef gap::util::Rational<int> _Ty;

		static _Ty min()
		{	// return minimum value
			return _Ty(INT_MIN);
		}

		static _Ty max()
		{	// return maximum value
			return _Ty(INT_MAX);
		}

		static _Ty lowest()
		{	// return most negative value
			return _Ty((min)());
		}

		static _Ty epsilon()
		{	// return smallest effective increment from 1.0
			return _Ty(0);
		}

		static _Ty round_error()
		{	// return largest rounding error
			return _Ty(0);
		}

		static _Ty denorm_min()
		{	// return minimum denormalized value
			return _Ty(0);
		}

		static _Ty infinity()
		{	// return positive infinity
			return _Ty(0);
		}

		static _Ty quiet_NaN()
		{	// return non-signaling NaN
			return _Ty(0);
		}

		static _Ty signaling_NaN()
		{	// return signaling NaN
			return _Ty(0);
		}

		_STCONS(bool, is_signed, true);
		_STCONS(int, digits, CHAR_BIT * sizeof(int) - 1);
		_STCONS(int, digits10, (CHAR_BIT * sizeof(int) - 1)
			* 301L / 1000);
	};

} /// namespace std

#endif /// GAP_UTIL_RATIONAL_NUMBER_H
