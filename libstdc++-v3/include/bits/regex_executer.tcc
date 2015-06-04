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
 *  @file bits/regex_executer.tcc
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
      using _Elem = pair<size_t, _Tp>;
      static_assert(sizeof(_Elem) <= _GLIBCXX_REGEX_STACK_BLOCK_SIZE,
		    "sizeof(_Elem) is too large");
      auto __allocated = _M_allocate<sizeof(_Elem)>();
      __try
	{
	  new (_M_top_frame()) _Elem(__allocated, std::forward<_Tp>(__val));
	}
      __catch(...)
	{
	  _M_deallocate(__allocated);
	  __throw_exception_again;
	}
    }

  template<typename _Tp>
    void
    _Dynamic_stack::_M_pop() noexcept
    {
      using _Elem = pair<size_t, _Tp>;
      auto __p = static_cast<_Elem*>(_M_top_frame());
      auto __allocated = __p->first;
      __p->second.~_Tp();
      _M_deallocate(__allocated);
    }

  template<typename _Tp>
    _Tp&
    _Dynamic_stack::_M_top_item()
    {
      using _Elem = pair<size_t, _Tp>;
      auto __p = static_cast<_Elem*>(_M_top_frame());
      return __p->second;
    }

  template<size_t __size>
    size_t
    _Dynamic_stack::_M_allocate()
    {
      if (_M_blocks.front()._M_avail() < __size)
	_M_blocks.emplace_front();
      _M_blocks.front()._M_top =
	static_cast<void*>(static_cast<char*>(_M_top_frame()) - __size);
      return __size;
    }

  inline void
  _Dynamic_stack::_M_deallocate(size_t __size) noexcept
  {
    _M_blocks.front()._M_top =
      static_cast<void*>(static_cast<char*>(_M_top_frame()) + __size);
    if (_M_blocks.front()._M_avail() >= _GLIBCXX_REGEX_STACK_BLOCK_SIZE
	&& _M_top() != _M_base)
      _M_blocks.pop_front();
  }

  template<typename _Bi_iter>
  template<template<typename, bool> class _Executer_tmpl, typename _Traits, bool __is_ecma>
    void
    _Regex_run<_Bi_iter>::_Regex_stack::
    _M_exec(_Regex_context& __context, _Regex_runner<_Executer_tmpl<_Traits, __is_ecma>>& __runner)
    {
      void* __top = _M_stack._M_top();
      __try
	{
	  _M_handle(_Saved_state(__context._M_state), __runner, __context);
	  while (_M_stack._M_top() != __top)
	    {
	      if (__is_ecma && __context._M_found)
		{
		  _M_cleanup(__top);
		  break;
		}
	      switch (_M_stack._M_top_item<_Tag>()._M_tag)
		{
#define __HANDLE(_Type) \
		    {\
		      auto __save = std::move(_M_stack._M_top_item<_Type>());\
		      _M_stack._M_pop<_Type>();\
		      _M_handle(__save, __runner, __context);\
		    }
		case _Tag::_S_Saved_state: __HANDLE(_Saved_state); break;
		case _Tag::_S_Saved_paren: __HANDLE(_Saved_paren); break;
		case _Tag::_S_Saved_position: __HANDLE(_Saved_position); break;
		case _Tag::_S_Saved_last: __HANDLE(_Saved_last); break;
#undef __HANDLE
		};
	    }
	}
      __catch (...)
	{
	  _M_cleanup(__top);
	  __throw_exception_again;
	}
    }

  template<typename _Bi_iter>
  template<typename _Runner>
    void
    _Regex_run<_Bi_iter>::_Regex_stack::
    _M_handle(const _Saved_state& __save, _Runner& __runner, _Regex_context& __context)
    {
      __context._M_state = __save._M_state;
      __runner._M_dfs(__context);
    }

  template<typename _Bi_iter>
  template<typename _Runner>
    void
    _Regex_run<_Bi_iter>::_Regex_stack::
    _M_handle(const _Saved_paren& __save, _Runner& __runner, _Regex_context& __context)
    { __context._M_parens[__save._M_index] = std::move(__save._M_paren); }

  template<typename _Bi_iter>
  template<typename _Runner>
    void
    _Regex_run<_Bi_iter>::_Regex_stack::
    _M_handle(const _Saved_position& __save, _Runner& __runner, _Regex_context& __context)
    { __runner._M_exec_context._M_current = std::move(__save._M_position); }

  template<typename _Bi_iter>
  template<typename _Runner>
    void
    _Regex_run<_Bi_iter>::_Regex_stack::
    _M_handle(const _Saved_last& __save, _Runner& __runner, _Regex_context& __context)
    { __runner._M_executer._M_set_last(__save._M_last); }

  template<typename _Bi_iter>
  template<template<typename, bool> class _Executer_tmpl, typename _Traits, bool __is_ecma>
    void
    _Regex_run<_Bi_iter>::_Regex_runner<_Executer_tmpl<_Traits, __is_ecma>>::
    _M_dfs(_Regex_context& __context)
    {
#define __TAIL_RECURSE(__new_state) { __context._M_state = (__new_state); continue; }
      while (1)
	{
	  if (_M_executer._M_visited(__context._M_state, __context))
	    return;
	  const auto& __state = _M_exec_context._M_get_state(__context._M_state);
	  const auto& __current = _M_exec_context._M_current;
	  switch (__state._M_opcode)
	    {
	    case _S_opcode_repeat:
	      if (_M_executer._M_handle_repeat(*this, __state, __context))
		continue;
	      break;
	    case _S_opcode_alternative:
	      _M_stack.template _M_push<typename _Regex_stack::_Saved_state>(__state._M_next);
	      __TAIL_RECURSE(__state._M_alt)
	      break;
	    case _S_opcode_subexpr_begin:
	      {
		auto& __paren = __context._M_parens[__state._M_subexpr];
		_M_stack.template _M_push<typename _Regex_stack::_Saved_paren>(__state._M_subexpr, __paren);
		__paren._M_set_left(__current);
		__TAIL_RECURSE(__state._M_next)
		break;
	      }
	    case _S_opcode_subexpr_end:
	      {
		auto& __paren = __context._M_parens[__state._M_subexpr];
		_M_stack.template _M_push<typename _Regex_stack::_Saved_paren>(__state._M_subexpr, __paren);
		__paren._M_set_right(__current);
		__TAIL_RECURSE(__state._M_next)
	      }
	      break;
	    case _S_opcode_line_begin_assertion:
	      if (_M_exec_context._M_at_begin()
		  && !(_M_exec_context._M_flags & (regex_constants::match_not_bol
						   | regex_constants::match_prev_avail)))
		__TAIL_RECURSE(__state._M_next)
	      break;
	    case _S_opcode_line_end_assertion:
	      if (_M_exec_context._M_at_end()
		  && !(_M_exec_context._M_flags & regex_constants::match_not_eol))
		__TAIL_RECURSE(__state._M_next)
	      break;
	    case _S_opcode_word_boundary:
	      if (_M_exec_context._M_word_boundary() == !__state._M_neg)
		__TAIL_RECURSE(__state._M_next)
	      break;
	    case _S_opcode_subexpr_lookahead:
	      {
		_Regex_runner __runner(_M_stack._M_stack);
		__runner._M_init(__current, _M_exec_context._M_end,
				 *_M_exec_context._M_nfa, _M_exec_context._M_flags,
				 _M_exec_context._M_search_mode);
		_Captures __captures;
		bool __ret = __runner._M_match_impl(__state._M_alt, __context._M_parens.size(), __captures);
		if (__ret != __state._M_neg)
		  {
		    if (__ret)
		      {
			auto& __res = __context._M_parens;
			_GLIBCXX_DEBUG_ASSERT(__res.size() == __captures.size());
			for (size_t __i = 0; __i < __captures.size(); __i++)
			  if (__captures[__i]._M_matched())
			    {
			      _M_stack.template _M_push<typename _Regex_stack::_Saved_paren>(__i, __res[__i]);
			      __res[__i] = __captures[__i];
			    }
		      }
		    __TAIL_RECURSE(__state._M_next)
		  }
	      }
	      break;
	    case _S_opcode_match:
	      if (_M_exec_context._M_at_end())
		break;
	      if (_M_executer._M_handle_match(*this, __state, __context))
		continue;
	      break;
	    case _S_opcode_backref:
	      if (_M_executer._M_handle_backref(*this, __state, __context))
		continue;
	      break;
	    case _S_opcode_accept:
	      if (!__context._M_found)
		{
		  if (_M_exec_context._M_search_mode == _Regex_search_mode::_Exact)
		    __context._M_found = _M_exec_context._M_at_end();
		  else
		    __context._M_found = true;
		  if (_M_exec_context._M_flags & regex_constants::match_not_null)
		    __context._M_found = __context._M_found && !_M_exec_context._M_at_begin();
		}
	      if (__context._M_found)
		_M_executer._M_handle_accept(__context._M_parens);
	      break;
	    case _S_opcode_unknown:
	    case _S_opcode_dummy:
	      _GLIBCXX_DEBUG_ASSERT(false);
	      break;
	    }
	  break;
	}
#undef __TAIL_RECURSE
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    bool
    _Regex_run<_Bi_iter>::_Executer_context<_Traits>::_M_is_word(_Char __ch) const
    {
      static const _Char __s[2] = { 'w' };
      return _M_traits().isctype
	(__ch, _M_traits().lookup_classname(__s, __s+1));
    }

  template<typename _Bi_iter>
  template<typename _Traits>
    bool
    _Regex_run<_Bi_iter>::_Executer_context<_Traits>::_M_at_begin() const
    { return _M_current == _M_begin; }

  template<typename _Bi_iter>
  template<typename _Traits>
    bool
    _Regex_run<_Bi_iter>::_Executer_context<_Traits>::_M_at_end() const
    { return _M_current == _M_end; }

  template<typename _Bi_iter>
  template<typename _Traits>
    bool
    _Regex_run<_Bi_iter>::_Executer_context<_Traits>::_M_word_boundary() const
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
  template<typename _Traits>
    bool
    _Regex_run<_Bi_iter>::_Executer_context<_Traits>::
    _M_match_backref(unsigned int __index, _Regex_context& __context, _Bi_iter& __output)
    {
      const auto& __capture = __context._M_parens[__index];
      if (!__capture._M_matched())
	return false;

      auto __start = __capture._M_get_left();
      auto __end = __capture._M_get_right();
      auto __current = _M_current;
      bool __ret = [&]
	{
	  for (auto __len = std::distance(__start, __end); __len > 0; __len--)
	    {
	      if (_M_current == _M_end)
		return false;
	      ++_M_current;
	    }

	  if (_M_flags & regex_constants::collate)
	    return _M_traits().transform(__start, __end)
	      == _M_traits().transform(__current, _M_current);
	  return std::equal(__start, __end, __current);
	}();
      if (__ret)
	__output = __current;
      else
	_M_current = __current;
      return __ret;
    }

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace __detail
} // namespace
