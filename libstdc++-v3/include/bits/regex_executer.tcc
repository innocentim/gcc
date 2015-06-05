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

  inline void
  _Dynamic_stack::_M_jump(void* __old_top) noexcept
  {
    while (!(_M_blocks.front()._M_data <= __old_top
	     && (__old_top < std::end(_M_blocks.front()._M_data)
		 || __old_top == _M_base)))
      _M_blocks.pop_front();
    _M_top_frame() = __old_top;
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

  inline void*&
  _Dynamic_stack::_M_top_frame() noexcept
  {
    auto __top = _M_blocks.front()._M_top;
    _GLIBCXX_DEBUG_ASSERT(_M_blocks.front()._M_data <= __top && __top <= std::end(_M_blocks.front()._M_data));
    return _M_blocks.front()._M_top;
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
  template<typename _Runner>
    void
    _Regex_scope<_Bi_iter>::_Stack_handlers::
    _M_exec(_Match_head& __head, _Runner& __runner)
    {
      void* __top = _M_stack._M_top();
      __try
	{
	  _M_handle(_Saved_state(__head._M_state), __runner, __head);
	  while (_M_stack._M_top() != __top)
	    {
	      if (__runner._S_is_ecma && __head._M_found)
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
		      _M_handle(__save, __runner, __head);\
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
    void
    _Regex_scope<_Bi_iter>::_Stack_handlers::_M_cleanup(void* __old_top) noexcept
    {
      if (is_trivially_destructible<_Bi_iter>::value)
	_M_stack._M_jump(__old_top);
      else
	while (_M_stack._M_top() != __old_top)
	  switch (_M_stack._M_top_item<_Tag>()._M_tag)
	    {
	    case _Tag::_S_Saved_state:
	      _M_stack._M_pop<_Saved_state>(); break;
	    case _Tag::_S_Saved_paren:
	      _M_stack._M_pop<_Saved_paren>(); break;
	    case _Tag::_S_Saved_position:
	      _M_stack._M_pop<_Saved_position>(); break;
	    case _Tag::_S_Saved_last:
	      _M_stack._M_pop<_Saved_last>(); break;
	    };
    }

  template<typename _Bi_iter>
  template<typename _Runner>
    void
    _Regex_scope<_Bi_iter>::_Stack_handlers::
    _M_handle(const _Saved_state& __save, _Runner& __runner, _Match_head& __head)
    {
      __head._M_state = __save._M_state;
      __runner._M_dfs(__head);
    }

  template<typename _Bi_iter>
  template<typename _Runner>
    void
    _Regex_scope<_Bi_iter>::_Stack_handlers::
    _M_handle(const _Saved_paren& __save, _Runner& __runner, _Match_head& __head)
    { __head._M_parens[__save._M_index] = std::move(__save._M_paren); }

  template<typename _Bi_iter>
  template<typename _Runner>
    void
    _Regex_scope<_Bi_iter>::_Stack_handlers::
    _M_handle(const _Saved_position& __save, _Runner& __runner, _Match_head& __head)
    { __runner._M_current = std::move(__save._M_position); }

  template<typename _Bi_iter>
  template<typename _Runner>
    void
    _Regex_scope<_Bi_iter>::_Stack_handlers::
    _M_handle(const _Saved_last& __save, _Runner& __runner, _Match_head& __head)
    { __runner._M_executer._M_set_last(__save._M_last); }

  template<typename _Bi_iter>
  template<typename _Executer, typename _Traits, bool __is_ecma>
    void
    _Regex_scope<_Bi_iter>::_Regex_runner<_Executer, _Traits, __is_ecma>::
    _M_init(_Bi_iter __begin, _Bi_iter __end, const _NFA<_Traits>& __nfa,
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
  template<typename _Executer, typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Regex_runner<_Executer, _Traits, __is_ecma>::_M_is_word(_Char __ch) const
    {
      static const _Char __s[2] = { 'w' };
      return _M_traits().isctype
	(__ch, _M_traits().lookup_classname(__s, __s+1));
    }

  template<typename _Bi_iter>
  template<typename _Executer, typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Regex_runner<_Executer, _Traits, __is_ecma>::_M_word_boundary() const
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
  template<typename _Executer, typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Regex_runner<_Executer, _Traits, __is_ecma>::
    _M_match_backref(unsigned int __index, _Match_head& __head, _Bi_iter& __output)
    {
      const auto& __capture = __head._M_parens[__index];
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

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_search_from_first(_Runner& __runner, _StateIdT __start, size_t __size, _Captures& __result)
    {
      _M_last.first = 0;
      this->_M_reset(__start, __size);
      __runner._M_stack._M_exec(this->_M_get_head(), __runner);
      if (this->_M_get_head()._M_found)
	{
	  this->_M_get_result(__result);
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_handle_repeat(_Runner& __runner, const _State<_Traits>& __state, _Match_head& __head)
    {
      const auto& __current = __runner._M_current;
      if (_M_last.first == 2 && _M_last.second == __current)
	return false;

      _StateIdT __first = __state._M_alt, __second = __state._M_next;
      if (__state._M_neg)
	swap(__first, __second);
      __runner._M_stack.template _M_push<typename _Stack_handlers::_Saved_state>(__second);
      __runner._M_stack.template _M_push<typename _Stack_handlers::_Saved_last>(_M_last);
      if (_M_last.first == 0 || _M_last.second != __current)
	{
	  _M_last.first = 0;
	  _M_last.second = __current;
	}
      _M_last.first++;

      __head._M_state = __first;
      return true;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_handle_match(_Runner& __runner, const _State<_Traits>& __state, _Match_head& __head)
    {
      if (__state._M_matches(*__runner._M_current))
	{
	  __runner._M_stack.template _M_push<typename _Stack_handlers::_Saved_position>(__runner._M_current);
	  ++__runner._M_current;
	  __head._M_state = __state._M_next;
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Dfs_executer<_Traits, __is_ecma>::
    _M_handle_backref(_Runner& __runner, const _State<_Traits>& __state, _Match_head& __head)
    {
      _Bi_iter __current;
      if (__runner._M_match_backref(__state._M_backref_index, __head, __current))
	{
	  __runner._M_stack.template _M_push<typename _Stack_handlers::_Saved_position>(std::move(__current));
	  __head._M_state = __state._M_next;
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_visited(_StateIdT __state_id, _Match_head& __head)
    {
      if (_M_is_visited[__state_id])
	{
	  if (__is_ecma)
	    return true;
	  if (!_M_leftmost_longest(__head._M_parens, _M_current_positions[__state_id]))
	    return true;
	}
      _M_is_visited[__state_id] = true;
      if (!__is_ecma)
	_M_current_positions[__state_id] = __head._M_parens;
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_search_from_first(_Runner& __runner, _StateIdT __start, size_t __size, _Captures& __result)
    {
      _M_is_visited.resize(__runner._M_nfa->size());
      _M_current_positions.resize(__runner._M_nfa->size());
      _M_heads.clear();
      _M_heads.emplace_back(__start, __size);
      if (_M_search_from_first_impl(__runner))
	{
	  __result = std::move(_M_result);
	  return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_search_from_first_impl(_Runner& __runner)
    {
      bool __found = false;
      while (!_M_heads.empty())
	{
	  _M_found = false;
	  std::fill_n(_M_is_visited.begin(), _M_is_visited.size(), false);
	  auto __heads = std::move(_M_heads);
	  for (auto& __head : __heads)
	    __runner._M_stack._M_exec(__head, __runner);
	  __found = __found || _M_found;
	  if (__runner._M_current != __runner._M_end)
	    ++__runner._M_current;
	}
      return __found;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_handle_repeat(_Runner& __runner, const _State<_Traits>& __state, _Match_head& __head)
    {
      _StateIdT __first = __state._M_alt, __second = __state._M_next;
      if (__state._M_neg)
	swap(__first, __second);
      __runner._M_stack.template _M_push<typename _Stack_handlers::_Saved_state>(__second);
      __head._M_state = __first;
      return true;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_handle_match(_Runner& __runner, const _State<_Traits>& __state, const _Match_head& __head)
    {
      if (!__state._M_matches(*__runner._M_current))
	return false;
      if (__is_ecma && _M_found)
	return false;
      _M_heads.emplace_back(__head);
      _M_heads.back()._M_state = __state._M_next;
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Traits, bool __is_ecma>
    void
    _Regex_scope<_Bi_iter>::_Bfs_executer<_Traits, __is_ecma>::
    _M_handle_accept(const _Captures& __captures)
    {
      _M_found = true;
      if (__is_ecma || (_M_result.empty() || _M_leftmost_longest(__captures, _M_result)))
	_M_result = __captures;
    }

  template<typename _Bi_iter>
  template<typename _Executer, typename _Traits, bool __is_ecma>
  template<typename _Bp, typename _Alloc>
    bool
    _Regex_scope<_Bi_iter>::_Regex_runner<_Executer, _Traits, __is_ecma>::
    _M_match(std::vector<sub_match<_Bp>, _Alloc>& __res)
    {
      _Captures __result;
      if (_M_match_impl(_M_nfa->_M_start(), _M_nfa->_M_sub_count(), __result))
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
  template<typename _Executer, typename _Traits, bool __is_ecma>
    bool
    _Regex_scope<_Bi_iter>::_Regex_runner<_Executer, _Traits, __is_ecma>::
    _M_match_impl(_StateIdT __start, size_t __size, _Captures& __result)
    {
      if (_M_search_mode == _Regex_search_mode::_Exact)
	return _M_search_from_first_helper(__start, __result);
      if (_M_search_from_first_helper(__start, __result))
	return true;
      if (_M_flags & regex_constants::match_continuous)
	return false;
      _M_flags |= regex_constants::match_prev_avail;
      while (_M_begin != _M_end)
	{
	  ++_M_begin;
	  if (_M_search_from_first_helper(__start, __result))
	    return true;
	}
      return false;
    }

  template<typename _Bi_iter>
  template<typename _Executer, typename _Traits, bool __is_ecma>
    void
    _Regex_scope<_Bi_iter>::_Regex_runner<_Executer, _Traits, __is_ecma>::
    _M_dfs(_Match_head& __head)
    {
#define __TAIL_RECURSE(__new_state) { __head._M_state = (__new_state); continue; }
      while (1)
	{
	  if (_M_executer._M_visited(__head._M_state, __head))
	    return;
	  const auto& __state = _M_get_state(__head._M_state);
	  const auto& __current = _M_current;
	  switch (__state._M_opcode)
	    {
	    case _S_opcode_repeat:
	      if (_M_executer._M_handle_repeat(*this, __state, __head))
		continue;
	      break;
	    case _S_opcode_alternative:
	      _M_stack.template _M_push<typename _Stack_handlers::_Saved_state>(__state._M_next);
	      __TAIL_RECURSE(__state._M_alt)
	      break;
	    case _S_opcode_subexpr_begin:
	      {
		auto& __paren = __head._M_parens[__state._M_subexpr];
		_M_stack.template _M_push<typename _Stack_handlers::_Saved_paren>(__state._M_subexpr, __paren);
		__paren._M_set_left(__current);
		__TAIL_RECURSE(__state._M_next)
		break;
	      }
	    case _S_opcode_subexpr_end:
	      {
		auto& __paren = __head._M_parens[__state._M_subexpr];
		_M_stack.template _M_push<typename _Stack_handlers::_Saved_paren>(__state._M_subexpr, __paren);
		__paren._M_set_right(__current);
		__TAIL_RECURSE(__state._M_next)
	      }
	      break;
	    case _S_opcode_line_begin_assertion:
	      if (__current == _M_begin
		  && !(_M_flags & (regex_constants::match_not_bol
						   | regex_constants::match_prev_avail)))
		__TAIL_RECURSE(__state._M_next)
	      break;
	    case _S_opcode_line_end_assertion:
	      if (__current == _M_end
		  && !(_M_flags & regex_constants::match_not_eol))
		__TAIL_RECURSE(__state._M_next)
	      break;
	    case _S_opcode_word_boundary:
	      if (_M_word_boundary() == !__state._M_neg)
		__TAIL_RECURSE(__state._M_next)
	      break;
	    case _S_opcode_subexpr_lookahead:
	      {
		_Regex_runner __runner(__get_dynamic_stack());
		__runner._M_init(__current, _M_end,
				 *_M_nfa, _M_flags,
				 _M_search_mode);
		_Captures __captures;
		bool __ret = __runner._M_match_impl(__state._M_alt, __head._M_parens.size(), __captures);
		if (__ret != __state._M_neg)
		  {
		    if (__ret)
		      {
			auto& __res = __head._M_parens;
			_GLIBCXX_DEBUG_ASSERT(__res.size() == __captures.size());
			for (size_t __i = 0; __i < __captures.size(); __i++)
			  if (__captures[__i]._M_matched())
			    {
			      _M_stack.template _M_push<typename _Stack_handlers::_Saved_paren>(__i, __res[__i]);
			      __res[__i] = __captures[__i];
			    }
		      }
		    __TAIL_RECURSE(__state._M_next)
		  }
	      }
	      break;
	    case _S_opcode_match:
	      if (_M_end == __current)
		break;
	      if (_M_executer._M_handle_match(*this, __state, __head))
		continue;
	      break;
	    case _S_opcode_backref:
	      if (_M_executer._M_handle_backref(*this, __state, __head))
		continue;
	      break;
	    case _S_opcode_accept:
	      if (!__head._M_found)
		{
		  if (_M_search_mode == _Regex_search_mode::_Exact)
		    __head._M_found = _M_end == __current;
		  else
		    __head._M_found = true;
		  if (_M_flags & regex_constants::match_not_null)
		    __head._M_found = __head._M_found && _M_begin != __current;
		}
	      if (__head._M_found)
		_M_executer._M_handle_accept(__head._M_parens);
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
#define _RUN(_EXECUTER_TYPE, __IS_ECMA) \
	do \
	  { \
	    _Regex_runner<_EXECUTER_TYPE<_Traits, __IS_ECMA>, _Traits, __IS_ECMA> __runner(__get_dynamic_stack()); \
	    __runner._M_init(__s, __e, __nfa, __flags, __search_mode); \
	    __ret = __runner._M_match(__res); \
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
