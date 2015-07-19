// class template regex -*- C++ -*-

// Copyright (C) 2015 Free Software Foundation, Inc.
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

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/**
 *  @file bits/regex_backend.tcc
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{regex}
 */

namespace std _GLIBCXX_VISIBILITY(default)
{
namespace __detail
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  template<typename _Tp>
    void
    _Dynamic_stack::_M_push(_Tp&& __val)
    {
      _GLIBCXX_DEBUG_ASSERT(_M_in_current_block(_M_top_ptr));
      using _Elem = std::pair<void*, _Tp>;
      static_assert(sizeof(_Elem) < _GLIBCXX_REGEX_STACK_BLOCK_SIZE, "Iterator size is too large");
      bool __need_push = sizeof(_Elem) > _M_avail();
      _Elem* __new_top = reinterpret_cast<_Elem*>(__need_push ? (_M_blocks.emplace_front(), _M_end()) : _M_top_ptr) - 1;
      _GLIBCXX_DEBUG_ASSERT(sizeof(_Elem) <= _M_avail());
      __try
	{
	  new (__new_top) _Elem(_M_top_ptr, std::forward<_Tp>(__val));
	}
      __catch(...)
	{
	  if (__need_push)
	    _M_blocks.pop_front();
	  __throw_exception_again;
	}
      _M_top_ptr = __new_top;
      _GLIBCXX_DEBUG_ASSERT(_M_in_current_block(_M_top_ptr));
    }

  template<typename _Tp>
    void
    _Dynamic_stack::_M_pop() noexcept
    {
      _GLIBCXX_DEBUG_ASSERT(_M_in_current_block(_M_top_ptr));
      using _Elem = std::pair<void*, _Tp>;
      auto __elem = reinterpret_cast<_Elem*>(_M_top_ptr);
      if (__elem + 1 >= _M_end() && _M_base != _M_end())
	{
	  _M_top_ptr = __elem->first;
	  __elem->~_Elem();
	  _M_blocks.pop_front();
	}
      else
	{
	  _M_top_ptr = __elem->first;
	  __elem->~_Elem();
	}
      _GLIBCXX_DEBUG_ASSERT(_M_in_current_block(_M_top_ptr));
    }

  inline void
  _Dynamic_stack::_M_jump(void* __old_top) noexcept
  {
    while (!_M_in_current_block(__old_top))
      _M_blocks.pop_front();
    _M_top_ptr = __old_top;
  }

  template<typename _Bi_iter>
    bool
    _Regex_scope<_Bi_iter>::_Capture::operator<(const _Capture& __rhs) const
    {
      _GLIBCXX_DEBUG_ASSERT((_M_state == _S_only_left)
			    == (__rhs._M_state == _S_only_left));
      if (_M_state == _S_none)
	return false;
      if (__rhs._M_state == _S_none)
	return true;
      if (_M_state == _S_only_left)
	return _M_left < __rhs._M_left;
      if (_M_left < __rhs._M_left)
	return true;
      if (_M_left > __rhs._M_left)
	return false;
      return _M_right > __rhs._M_right;
    }

  template<typename _Bi_iter>
    bool
    _Regex_scope<_Bi_iter>::_Bfs_ecma_mixin::
    _M_visited_impl(_StateIdT __state_id, _Match_head& __head)
    { return _M_is_visited[__state_id]; }

  template<typename _Bi_iter>
    bool
    _Regex_scope<_Bi_iter>::_Bfs_posix_mixin::
    _M_visited_impl(_StateIdT __state_id, _Match_head& __head)
    {
      if (_M_is_visited[__state_id])
	if (!_M_leftmost_longest(__head._M_parens, _M_current_positions[__state_id]))
	  return true;
      return false;
    }

  template<typename _Bi_iter>
  template<_RegexExecutorPolicy __policy, typename _Bp, typename _Alloc, typename _Traits>
    bool
    _Regex_scope<_Bi_iter>::__run(_Bi_iter __s, _Bi_iter __e,
				std::vector<sub_match<_Bp>, _Alloc>& __res,
				const _NFA<_Traits>& __nfa,
				regex_constants::match_flag_type __flags,
				_Regex_search_mode __search_mode)
    {
      bool __use_bfs =
	(__nfa._M_options() & regex_constants::__polynomial)
	|| (__policy == _RegexExecutorPolicy::_S_alternate
	    && !__nfa._M_has_backref);
      bool __is_ecma = __nfa._M_options() & regex_constants::ECMAScript;
      bool __ret;
#define _RUN(_Executer_type, __IS_ECMA) \
	do \
	  { \
	    _Executer_type<_Traits, __IS_ECMA> __executer; \
	    __executer._M_get_context()._M_init(__s, __e, __nfa, __flags, __search_mode); \
	    __ret = __match(__executer, __res); \
	  } \
	while (false)
      if (!__use_bfs && __is_ecma)
	_RUN(_Dfs_executer, true);
      else if (!__use_bfs && !__is_ecma)
	_RUN(_Dfs_executer, false);
      else if (__use_bfs && __is_ecma)
	_RUN(_Bfs_executer, true);
      else
	_RUN(_Bfs_executer, false);
#undef _RUN
      _GLIBCXX_DEBUG_ASSERT(__get_dynamic_stack()._M_empty());
      return __ret;
    }

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace __detail
} // namespace
