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
    inline void
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
    inline void
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
    inline bool
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
  template<typename _Traits>
    inline void
    _Regex_scope<_Bi_iter>::_Context<_Traits>::
    _M_init(_Bi_iter __begin, _Bi_iter __end,
	    const _NFA<_Traits>& __nfa,
	    regex_constants::match_flag_type __flags,
	    _Regex_search_mode __search_mode)
    {
      _M_begin = __begin;
      _M_end = __end;
      _M_flags = __flags;
      _M_nfa = &__nfa;
      _M_search_mode = __search_mode;
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    inline bool
    _Regex_scope<_Bi_iter>::_Context<_Traits>::
    _M_word_boundary() const
    {
      bool __left_is_word =
	(_M_current != _M_begin
	 || (_M_flags & regex_constants::match_prev_avail))
	&& _M_is_word(*std::prev(_M_current));
      bool __right_is_word =
	_M_current != _M_end && _M_is_word(*_M_current);

      if (__left_is_word == __right_is_word)
	return false;
      if (__left_is_word && !(_M_flags & regex_constants::match_not_eow))
	return true;
      if (__right_is_word && !(_M_flags & regex_constants::match_not_bow))
	return true;
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Executer>
    inline void
    _Regex_scope<_Bi_iter>::
    _M_dfs(_Executer& __executer, _Match_head& __head)
    {
      while (1)
	{
	  if (__executer._M_visited(__head._M_state, __head))
	    return;
	  __executer._M_visit(__head._M_state, __head);
	  const auto& __state = __executer._M_get_context()._M_get_state(__head._M_state);
	  switch (__state._M_opcode())
	    {
#define _DFS_DISPATCH_ENTRY(__opcode, __func_name) \
	    case __opcode: \
	      { \
		if (__executer.__func_name(__state, __head)) \
		  continue; \
		break; \
	      }
	    _DFS_DISPATCH_ENTRY(_S_opcode_repeat, _M_handle_repeat);
	    _DFS_DISPATCH_ENTRY(_S_opcode_subexpr_begin, _M_handle_subexpr_begin);
	    _DFS_DISPATCH_ENTRY(_S_opcode_subexpr_end, _M_handle_subexpr_end);
	    _DFS_DISPATCH_ENTRY(_S_opcode_alternative, _M_handle_alternative);
	    _DFS_DISPATCH_ENTRY(_S_opcode_line_begin_assertion, _M_handle_line_begin_assertion);
	    _DFS_DISPATCH_ENTRY(_S_opcode_line_end_assertion, _M_handle_line_end_assertion);
	    _DFS_DISPATCH_ENTRY(_S_opcode_word_boundary, _M_handle_word_boundary);
	    _DFS_DISPATCH_ENTRY(_S_opcode_subexpr_lookahead, _M_handle_subexpr_lookahead);
	    _DFS_DISPATCH_ENTRY(_S_opcode_match, _M_handle_match);
	    _DFS_DISPATCH_ENTRY(_S_opcode_backref, _M_handle_backref);
	    _DFS_DISPATCH_ENTRY(_S_opcode_accept, _M_handle_accept);
#undef _DFS_DISPATCH_ENTRY
	    case _S_opcode_unknown:
	    case _S_opcode_dummy:
	      _GLIBCXX_DEBUG_ASSERT(false);
	    }
	  break;
	}
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    inline bool
    _Regex_scope<_Bi_iter>::
    _M_handle_subexpr_begin_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head)
    {
      auto& __capture = __head._M_captures[__state._M_subexpr];
      __context._M_stack._M_push(_Saved_capture(__state._M_subexpr, __capture));
      __capture._M_set_left(__context._M_current);
      __head._M_state = __state._M_next;
      return true;
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    inline bool
    _Regex_scope<_Bi_iter>::
    _M_handle_subexpr_end_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head)
    {
      auto& __capture = __head._M_captures[__state._M_subexpr];
      __context._M_stack._M_push(_Saved_capture(__state._M_subexpr, __capture));
      __capture._M_set_right(__context._M_current);
      __head._M_state = __state._M_next;
      return true;
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    inline bool
    _Regex_scope<_Bi_iter>::
    _M_handle_alternative_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head)
    {
      __context._M_stack._M_push(_Saved_state(__state._M_next));
      __head._M_state = __state._M_alt;
      return true;
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    inline bool
    _Regex_scope<_Bi_iter>::
    _M_handle_line_begin_assertion_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head)
    {
      if (__context._M_current == __context._M_begin
	  && !(__context._M_flags & (regex_constants::match_not_bol | regex_constants::match_prev_avail)))
	{
	  __head._M_state = __state._M_next;
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    inline bool
    _Regex_scope<_Bi_iter>::
    _M_handle_line_end_assertion_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head)
    {
      if (__context._M_current == __context._M_end && !(__context._M_flags & regex_constants::match_not_eol))
	{
	  __head._M_state = __state._M_next;
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    inline bool
    _Regex_scope<_Bi_iter>::
    _M_handle_word_boundary_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head)
    {
      if (__context._M_word_boundary() == !__state._M_neg)
	{
	  __head._M_state = __state._M_next;
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Executer, typename _Traits>
    inline bool
    _Regex_scope<_Bi_iter>::
    _M_handle_subexpr_lookahead_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head)
    {
      _Executer __executer;
      __executer._M_get_context()._M_init(__context._M_current, __context._M_end, *__context._M_nfa, __context._M_flags, __context._M_search_mode);
      _Captures __captures;
      bool __ret = __match_impl(__executer, __state._M_alt, __captures);
      if (__ret != __state._M_neg)
	{
	  if (__ret)
	    {
	      auto& __res = __head._M_captures;
	      _GLIBCXX_DEBUG_ASSERT(__res.size() == __captures.size());
	      for (size_t __i = 0; __i < __captures.size(); __i++)
		if (__captures[__i]._M_matched())
		  {
		    __context._M_stack._M_push(_Saved_capture(__i, __res[__i]));
		    __res[__i] = __captures[__i];
		  }
	    }
	  __head._M_state = __state._M_next;
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    inline void
    _Regex_scope<_Bi_iter>::
    _M_handle_accept_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head)
    {
      if (!__head._M_found)
	{
	  if (__context._M_search_mode == _Regex_search_mode::_Exact)
	    __head._M_found = __context._M_end == __context._M_current;
	  else
	    __head._M_found = true;
	  if (__context._M_flags & regex_constants::match_not_null)
	    __head._M_found = __head._M_found && __context._M_begin != __context._M_current;
	}
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_search_from_first(_StateIdT __start, _Captures& __result)
    {
      _M_last.first = 0;
      this->_M_reset(__start, _M_context._M_nfa->_M_sub_count());
      _M_exec(this->_M_get_head());
      if (this->_M_get_head()._M_found)
	{
	  this->_M_get_result(__result);
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline void
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_exec(_Match_head& __head)
    {
      auto& __stack = _M_context._M_stack;
      const auto __cleanup = [&__stack](void* __old_top)
      {
	if (is_trivially_destructible<_Bi_iter>::value)
	  __stack._M_jump(__old_top);
	else
	  while (__stack._M_top() != __old_top)
	    switch (__stack.template _M_top_item<_Saved_tag>()._M_tag)
	      {
	      case _Saved_tag::_S_saved_state:
		__stack.template _M_pop<_Saved_state>(); break;
	      case _Saved_tag::_S_saved_capture:
		__stack.template _M_pop<_Saved_capture>(); break;
	      case _Saved_tag::_S_saved_position:
		__stack.template _M_pop<_Saved_position>(); break;
	      case _Saved_tag::_S_saved_dfs_repeat:
		__stack.template _M_pop<_Saved_dfs_repeat>(); break;
	      default: _GLIBCXX_DEBUG_ASSERT(false);
	      };
      };

      void* __top = __stack._M_top();
      __try
	{
	  _M_restore(_Saved_state(__head._M_state), __head);
	  while (__stack._M_top() != __top)
	    {
	      if (__is_ecma && __head._M_found)
		{
		  __cleanup(__top);
		  break;
		}
	      switch (__stack.template _M_top_item<_Saved_tag>()._M_tag)
		{
#define __HANDLE(_Type) \
		    {\
		      auto __save = std::move(__stack.template _M_top_item<_Type>());\
		      __stack.template _M_pop<_Type>();\
		      _M_restore(__save, __head);\
		    }
		case _Saved_tag::_S_saved_state: __HANDLE(_Saved_state); break;
		case _Saved_tag::_S_saved_capture: __HANDLE(_Saved_capture); break;
		case _Saved_tag::_S_saved_position: __HANDLE(_Saved_position); break;
		case _Saved_tag::_S_saved_dfs_repeat: __HANDLE(_Saved_dfs_repeat); break;
#undef __HANDLE
		default: _GLIBCXX_DEBUG_ASSERT(false);
		};
	    }
	}
      __catch (...)
	{
	  __cleanup(__top);
	  __throw_exception_again;
	}
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_handle_repeat(const _State<_Char_type>& __state, _Match_head& __head)
    {
      const auto& __current = _M_context._M_current;
      if (_M_last.first == 2 && _M_last.second == __current)
	return false;

      _StateIdT __first = __state._M_alt, __second = __state._M_next;
      if (__state._M_neg)
	swap(__first, __second);
      _M_push<_Saved_dfs_repeat>(__second, _M_last, __current);
      if (_M_last.first == 0 || _M_last.second != __current)
	{
	  _M_last.first = 1;
	  _M_last.second = __current;
	}
      else
	{
	  _M_last.first++;
	}

      __head._M_state = __first;
      return true;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_handle_alternative(const _State<_Char_type>& __state, _Match_head& __head)
    {
      _M_push<_Saved_state>(__state._M_next);
      this->_M_push<_Saved_position>(_M_context._M_current);
      __head._M_state = __state._M_alt;
      return true;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_handle_match(const _State<_Char_type>& __state, _Match_head& __head)
    {
      if (_M_context._M_end == _M_context._M_current)
	return false;
      if (__state._M_matches(*_M_context._M_current))
	{
	  ++_M_context._M_current;
	  __head._M_state = __state._M_next;
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_handle_backref(const _State<_Char_type>& __state, _Match_head& __head)
    {
      const auto& __capture = __head._M_captures[__state._M_backref_index];
      if (!__capture._M_matched())
	return false;

      auto& __current = _M_context._M_current;
      auto __new_current = __current;
      auto __start = __capture._M_get_left();
      auto __end = __capture._M_get_right();
      bool __ret = [&]
	{
	  for (auto __len = std::distance(__start, __end); __len > 0; __len--)
	    {
	      if (__new_current == _M_context._M_end)
		return false;
	      ++__new_current;
	    }
	  if (_M_context._M_nfa->_M_options() & regex_constants::collate)
	    return _M_context._M_traits().transform(__start, __end)
	      == _M_context._M_traits().transform(__current, __new_current);
	  return std::equal(__start, __end, __current);
	}();
      if (__ret)
	{
	  __current = __new_current;
	  __head._M_state = __state._M_next;
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_handle_accept(const _State<_Char_type>& __state, _Match_head& __head)
    {
      _M_handle_accept_common(_M_context, __state, __head);
      if (__head._M_found)
	this->_M_update(__head._M_captures);
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_search_from_first(_StateIdT __start, _Captures& __result)
    {
      this->_M_reset(_M_context._M_nfa->size());
      _M_heads.clear();
      _M_heads.emplace_back(__start, _M_context._M_nfa->_M_sub_count());
      if (_M_search_from_first_impl())
	{
	  __result = std::move(_M_result);
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline void
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_exec(_Match_head& __head)
    {
      auto& __stack = _M_context._M_stack;
      const auto __cleanup = [&__stack](void* __old_top)
      {
	if (is_trivially_destructible<_Bi_iter>::value)
	  __stack._M_jump(__old_top);
	else
	  while (__stack._M_top() != __old_top)
	    switch (__stack.template _M_top_item<_Saved_tag>()._M_tag)
	      {
	      case _Saved_tag::_S_saved_state:
		__stack.template _M_pop<_Saved_state>(); break;
	      case _Saved_tag::_S_saved_capture:
		__stack.template _M_pop<_Saved_capture>(); break;
	      default: _GLIBCXX_DEBUG_ASSERT(false);
	      };
      };

      void* __top = __stack._M_top();
      __try
	{
	  _M_restore(_Saved_state(__head._M_state), __head);
	  while (__stack._M_top() != __top)
	    {
	      if (__is_ecma && __head._M_found)
		{
		  __cleanup(__top);
		  break;
		}
	      switch (__stack.template _M_top_item<_Saved_tag>()._M_tag)
		{
#define __HANDLE(_Type) \
		    {\
		      auto __save = std::move(__stack.template _M_top_item<_Type>());\
		      __stack.template _M_pop<_Type>();\
		      _M_restore(__save, __head);\
		    }
		case _Saved_tag::_S_saved_state: __HANDLE(_Saved_state); break;
		case _Saved_tag::_S_saved_capture: __HANDLE(_Saved_capture); break;
#undef __HANDLE
		default: _GLIBCXX_DEBUG_ASSERT(false);
		};
	    }
	}
      __catch (...)
	{
	  __cleanup(__top);
	  __throw_exception_again;
	}
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_handle_repeat(const _State<_Char_type>& __state, _Match_head& __head)
    {
      _StateIdT __first = __state._M_alt, __second = __state._M_next;
      if (__state._M_neg)
	swap(__first, __second);
      _M_push<_Saved_state>(__second);
      __head._M_state = __first;
      return true;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_handle_alternative(const _State<_Char_type>& __state, _Match_head& __head)
    {
      _M_push<_Saved_state>(__state._M_next);
      __head._M_state = __state._M_alt;
      return true;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_handle_match(const _State<_Char_type>& __state, const _Match_head& __head)
    {
      if (_M_context._M_end == _M_context._M_current)
	return false;
      if (!__state._M_matches(*_M_context._M_current))
	return false;
      if (__is_ecma && _M_found)
	return false;
      _M_heads.emplace_back(__head);
      _M_heads.back()._M_state = __state._M_next;
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_handle_accept(const _State<_Char_type>& __state, _Match_head& __head)
    {
      _M_handle_accept_common(_M_context, __state, __head);
      if (__head._M_found)
	{
	  _M_found = true;
	  auto& __captures = __head._M_captures;
	  if (__is_ecma || (_M_result.empty() || _M_leftmost_longest(__captures, _M_result)))
	    _M_result = __captures;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    inline bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_search_from_first_impl()
    {
      bool __found = false;
      while (!_M_heads.empty())
	{
	  _M_found = false;
	  this->_M_clear();
	  auto __heads = std::move(_M_heads);
	  for (auto& __head : __heads)
	    _M_exec(__head);
	  __found = __found || _M_found;
	  if (_M_context._M_current != _M_context._M_end)
	    ++_M_context._M_current;
	}
      return __found;
    }

  template<typename _Bi_iter>
  template<typename _Executer, typename _Bp, typename _Alloc>
    inline bool
    _Regex_scope<_Bi_iter>::
    __match(_Executer& __executer, std::vector<sub_match<_Bp>, _Alloc>& __res)
    {
      _Captures __result;
      if (__match_impl(__executer, __executer._M_get_context()._M_nfa->_M_start(), __result))
	{
	  _GLIBCXX_DEBUG_ASSERT(__res.size() >= __result.size());
	  for (size_t __i = 0; __i < __result.size(); __i++)
	    if ((__res[__i].matched = __result[__i]._M_matched()))
	      {
		__res[__i].first = __result[__i]._M_get_left();
		__res[__i].second = __result[__i]._M_get_right();
	      }
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Executer>
    inline bool
    _Regex_scope<_Bi_iter>::
    __match_impl(_Executer& __executer, _StateIdT __start, _Captures& __result)
    {
      auto& __context = __executer._M_get_context();
      auto __size = __context._M_nfa->_M_sub_count();
      const auto __search_from_first_helper = [&]() -> bool
      {
	__context._M_current = __context._M_begin;
	return __executer._M_search_from_first(__start, __result);
      };

      if (__context._M_search_mode == _Regex_search_mode::_Exact)
	return __search_from_first_helper();
      if (__search_from_first_helper())
	return true;
      if (__context._M_flags & regex_constants::match_continuous)
	return false;
      __context._M_flags |= regex_constants::match_prev_avail;
      while (__context._M_begin != __context._M_end)
	{
	  ++__context._M_begin;
	  if (__search_from_first_helper())
	    return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<_RegexExecutorPolicy __policy, typename _Bp, typename _Alloc, typename _Traits>
    inline bool
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
      static std::mutex __mutex;
#define _RUN(_Executer_type, __IS_ECMA) \
	do \
	  { \
	    __mutex.lock(); \
	    _Executer_type<_Traits, __IS_ECMA> __executer; \
	    __executer._M_get_context()._M_init(__s, __e, __nfa, __flags, __search_mode); \
	    __ret = __match(__executer, __res); \
	    __mutex.unlock(); \
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
