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

#include "rational_number_test.h"
#include <gap_util/rational_number.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <cassert>

void TestConstruction()
{
	/*c2cuda::Rational<int> a, b(7, 10);
	assert(a.numerator() == 0 && a.denominator() == 1);
	assert(b.numerator() == 7 && b.denominator() == 10);

	c2cuda::Rational<short> c(13, 6);
	c2cuda::Rational<long long> d(c);
	c2cuda::Rational<unsigned int> e(b);

	assert(d.numerator() == static_cast<long long>(c.numerator()) && d.denominator() == static_cast<long long>(c.denominator()));
	assert(e.numerator() == static_cast<unsigned int>(b.numerator()) && e.denominator() == static_cast<unsigned int>(b.denominator()));

	a = c;
	d = e;
	b = d.numerator();

	assert(a.numerator() == static_cast<int>(c.numerator()) && a.denominator() == static_cast<int>(c.denominator()));
	assert(d.numerator() == static_cast<long long>(e.numerator()) && d.denominator() == static_cast<long long>(e.denominator()));
	assert(b.numerator() == static_cast<int>(d.numerator()) && b.denominator() == 1);*/
}

void TestArithmeticLogicOps()
{
	/*long k(4);
	c2cuda::Rational<int> a(4, 10), b(5, 7);
	c2cuda::Rational<int> c = a + b;
	assert(c.numerator() == a.numerator() * b.denominator() + b.numerator() * a.denominator());
	assert(c.denominator() == a.denominator() * b.denominator());

	a = b + int(k);
	assert(a.numerator() == b.numerator() + k * b.denominator());
	assert(a.denominator() == b.denominator());

	assert(c2cuda::Rational<long>(5, 6) * long(2) == c2cuda::Rational<int>(10, 6));
	assert(c2cuda::Rational<int>(3, 5) + 2 - c2cuda::Rational<int>(-1, 9) + 1 >= 0);

	a = { 4, -5 };
	b = { -4, 5 };
	assert(a.numerator() < 0 && a.denominator() > 0);
	assert(a == b);
	assert(!(a < b) && !(a > b));
	assert(a >= b && a <= b);
	assert(!(a != b));

	a = { -2, -10 };
	b = { 4, 20 };
	assert(a.numerator() > 0 && a.denominator() > 0);
	assert(a == b);
	assert(a - b == b - a);
	assert(a - b == 0);
	assert(0 == b - a && a - b == c2cuda::Rational<long>());

	auto old_a(a);
	a *= 5;
	assert(old_a == a / 5);
	assert(a > old_a);
	assert(a >= old_a);
	assert(a != old_a);

	a = b - b;
	assert(a == 0);
	assert(a.numerator() == 0 && a.denominator() > 0);

	auto old_b(b);
	b /= 10;
	assert(old_b == b * c2cuda::Rational<int>(10));
	assert(old_b != b);
	assert(old_b / b == 10);
	assert(b / old_b == c2cuda::Rational<int>(1, 10));

	c2cuda::Rational<long long> d(10, 8);
	auto e = a + d;

	try
	{
		c2cuda::Rational<short>(4, 0);
		assert(!"runtime_error should've occured");
	}
	catch (const std::runtime_error&)
	{

	}*/
}

void TestOutput()
{
	/*c2cuda::Rational<long long> a(4, 100), b(4, -5);
	std::stringstream ss, ss_expected;
	ss << a;
	ss_expected << "(" << a.numerator() << " / " << a.denominator() << ")";
	assert(ss.str() == ss_expected.str());

	ss.str("");
	ss_expected.str("");
	ss << b;
	ss_expected << "(" << b.numerator() << " / " << b.denominator() << ")";
	assert(ss.str() == ss_expected.str());

	ss.str("");
	ss_expected.str("");
	ss << a + b;
	ss_expected << "(" << (a + b).numerator() << " / " << (a + b).denominator() << ")";
	assert(ss.str() == ss_expected.str());*/
}