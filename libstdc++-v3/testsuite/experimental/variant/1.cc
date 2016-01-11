// { dg-options "-std=gnu++14" }
// { dg-do compile }

// Copyright (C) 2016 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

#include <experimental/variant>
#include <string>
#include <testsuite_hooks.h>

using namespace std;
using namespace experimental;

struct AllDeleted
{
  AllDeleted() = delete;
  AllDeleted(const AllDeleted&) = delete;
  AllDeleted(AllDeleted&&) = delete;
  AllDeleted& operator=(const AllDeleted&) = delete;
  AllDeleted& operator=(AllDeleted&&) = delete;
};

struct Empty
{
  Empty() { };
  Empty(const Empty&) { };
  Empty(Empty&&) { };
  Empty& operator=(const Empty&) { return *this; };
  Empty& operator=(Empty&&) { return *this; };
};

struct DefaultNoexcept
{
  DefaultNoexcept() noexcept = default;
  DefaultNoexcept(const DefaultNoexcept&) noexcept = default;
  DefaultNoexcept(DefaultNoexcept&&) noexcept = default;
  DefaultNoexcept& operator=(const DefaultNoexcept&) noexcept = default;
  DefaultNoexcept& operator=(DefaultNoexcept&&) noexcept = default;
};

void default_ctor()
{
  static_assert(is_default_constructible_v<variant<int, string>>, "");
  static_assert(is_default_constructible_v<variant<string, string>>, "");
  //static_assert(!is_default_constructible_v<variant<>>, "");
  static_assert(!is_default_constructible_v<variant<AllDeleted, string>>, "");
  static_assert(is_default_constructible_v<variant<string, AllDeleted>>, "");

  static_assert(noexcept(variant<int>()), "");
  static_assert(!noexcept(variant<Empty>()), "");
  static_assert(noexcept(variant<DefaultNoexcept>()), "");
}

void copy_ctor()
{
  static_assert(is_copy_constructible_v<variant<int, string>>, "");
  static_assert(!is_copy_constructible_v<variant<AllDeleted, string>>, "");

  {
    variant<int> a;
    static_assert(noexcept(variant<int>(a)), "");
  }
  {
    variant<string> a;
    static_assert(!noexcept(variant<string>(a)), "");
  }
  {
    variant<int, string> a;
    static_assert(!noexcept(variant<int, string>(a)), "");
  }
  {
    variant<int, char> a;
    static_assert(noexcept(variant<int, char>(a)), "");
  }
}

void move_ctor()
{
  static_assert(is_move_constructible_v<variant<int, string>>, "");
  static_assert(!is_move_constructible_v<variant<AllDeleted, string>>, "");
}

void copy_assign()
{
  static_assert(is_copy_assignable_v<variant<int, string>>, "");
  static_assert(!is_copy_assignable_v<variant<AllDeleted, string>>, "");
}

void move_assign()
{
  static_assert(is_move_assignable_v<variant<int, string>>, "");
  static_assert(!is_move_assignable_v<variant<AllDeleted, string>>, "");
}

void dtor()
{
  static_assert(is_destructible_v<variant<int, string>>, "");
  static_assert(is_destructible_v<variant<AllDeleted, string>>, "");
}
