// { dg-options "-std=gnu++11" }

// Copyright (C) 2015 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include <testsuite_hooks.h>
#include <testsuite_regex.h>

using namespace __gnu_test;
using namespace std;

void
test01()
{
  bool test __attribute__((unused)) = true;

  for (auto flags : { regex_constants::ECMAScript, regex_constants::extended })
    {
      cmatch m;
      VERIFY(regex_match_debug("b", m, regex("((a*)|(b*))+", flags)));
      VERIFY(m[0].matched);
      VERIFY(m[0].str() == "b");
      VERIFY(m[1].matched);
      VERIFY(m[1].str() == "b");
      VERIFY(m[2].matched);
      VERIFY(m[2].str() == "");
      VERIFY(m[3].matched);
      VERIFY(m[3].str() == "b");
    }
}

int
main()
{
  test01();

  return 0;
}
