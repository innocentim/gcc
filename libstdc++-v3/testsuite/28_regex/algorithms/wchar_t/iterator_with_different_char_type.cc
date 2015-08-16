// { dg-options "-std=gnu++11" }
// { dg-do compile }

//
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

// 28.11.2 regex_match
// Tests Extended bracket expression against a C-string.

#include <testsuite_hooks.h>
#include <testsuite_regex.h>

using namespace __gnu_test;
using namespace std;

void
test01()
{
  bool test __attribute__((unused)) = true;

  {
    const char s[] = "a";
    cmatch m;
    regex_match(s, s+1, m, wregex(L"a"));
    regex_search(s, s+1, m, wregex(L"a"));
  }
  {
    const wchar_t s[] = L"a";
    wcmatch m;
    regex_match(s, s+1, m, regex("a"));
    regex_search(s, s+1, m, regex("a"));
  }
}

int
main()
{
  test01();

  return 0;
}
