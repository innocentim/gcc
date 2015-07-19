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
 *  @file bits/regex_backend.h
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{regex}
 */

#ifndef _GLIBCXX_REGEX_STACK_BLOCK_SIZE
#define _GLIBCXX_REGEX_STACK_BLOCK_SIZE 1024
#endif

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION
_GLIBCXX_BEGIN_NAMESPACE_CXX11

  template<typename>
    class sub_match;

_GLIBCXX_END_NAMESPACE_CXX11
_GLIBCXX_END_NAMESPACE_VERSION

namespace __detail
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  /**
   * @addtogroup regex-detail
   * @{
   */

  class _Dynamic_stack
  {
  public:
    _Dynamic_stack() : _M_blocks(1), _M_base(_M_end()), _M_top_ptr(_M_end()) { }

    template<typename _Tp>
      void
      _M_push(_Tp&& __val);

    template<typename _Tp>
      void
      _M_pop() noexcept;

    template<typename _Tp>
      _Tp&
      _M_top_item()
      {
	_GLIBCXX_DEBUG_ASSERT(_M_in_current_block(_M_top_ptr));
	using _Elem = std::pair<void*, _Tp>;
	return reinterpret_cast<_Elem*>(_M_top_ptr)->second;
      }

    void*
    _M_top() const noexcept
    {
      _GLIBCXX_DEBUG_ASSERT(_M_in_current_block(_M_top_ptr));
      return _M_top_ptr;
    }

    void
    _M_jump(void* __old_top) noexcept;

#ifdef _GLIBCXX_DEBUG
    bool
    _M_empty() const noexcept
    { return _M_top() == _M_base; }
#endif

    bool
    _M_in_current_block(void* __top) const noexcept
    { return _M_begin() <= __top && (__top < _M_end() || __top == _M_base); }

  private:
    struct _Block
    { char _M_data[_GLIBCXX_REGEX_STACK_BLOCK_SIZE]; };

    size_t
    _M_avail() const noexcept
    { return reinterpret_cast<const char*>(_M_top_ptr) - reinterpret_cast<const char*>(_M_blocks.front()._M_data); }

    void*
    _M_begin() const noexcept
    { return const_cast<void*>(reinterpret_cast<const void*> (_M_blocks.front()._M_data)); }

    void*
    _M_end() const noexcept
    { return const_cast<void*>(reinterpret_cast<const void*>(std::end(_M_blocks.front()._M_data))); }

    std::forward_list<_Block> _M_blocks;
    void* _M_base;
    void* _M_top_ptr;
  };

  template<typename _Fp>
    class _Cleanup
    {
    public:
      _Cleanup(_Fp&& __f) : _M_f(std::move(__f)) { }

      ~_Cleanup()
      { _M_f(); }

    private:
      _Fp _M_f;
    };

  template<typename _Fp>
    _Cleanup<_Fp>
    __make_cleanup(_Fp&& __f)
    { return _Cleanup<_Fp>(std::move(__f)); }

  inline _Dynamic_stack&
  __get_dynamic_stack()
  {
    thread_local _Dynamic_stack __stack;
    return __stack;
  }

  template<typename _Bi_iter>
    class _Comparable_iter_wrapper : public _Bi_iter
    {
    public:
      _Comparable_iter_wrapper() = default;

      explicit
      _Comparable_iter_wrapper(_Bi_iter __iter)
      : _Bi_iter(std::move(__iter)), _M_position(0) { }

      _Comparable_iter_wrapper(const _Comparable_iter_wrapper& __rhs) = default;

      _Comparable_iter_wrapper&
      operator=(const _Comparable_iter_wrapper& __rhs) = default;

      _Comparable_iter_wrapper&
      operator++()
      {
	_M_position++;
	_Bi_iter::operator++();
	return *this;
      }

      _Comparable_iter_wrapper
      operator++(int)
      {
	auto __tmp = *this;
	++*this;
	return __tmp;
      }

      _Comparable_iter_wrapper&
      operator--()
      {
	_M_position--;
	_Bi_iter::operator--();
	return *this;
      }

      _Comparable_iter_wrapper
      operator--(int)
      {
	auto __tmp = *this;
	--*this;
	return __tmp;
      }

      bool
      operator<(const _Comparable_iter_wrapper& __rhs) const
      { return _M_position < __rhs._M_position; }

      bool
      operator>(const _Comparable_iter_wrapper& __rhs) const
      { return _M_position > __rhs._M_position; }

    private:
      long _M_position;
    };

  struct __comparable_iter_helper
  {
    template<typename _Bi_iter,
	     typename = decltype(std::declval<_Bi_iter>()
				 < std::declval<_Bi_iter>())>
      static _Bi_iter
      __test(int);

    template<typename _Bi_iter>
      static _Comparable_iter_wrapper<_Bi_iter>
      __test(...);
  };

  template<typename _Bi_iter>
    using _Comparable_iter = decltype(__comparable_iter_helper::__test<_Bi_iter>(0));

  enum class _Regex_search_mode : unsigned char { _Exact, _Prefix };

  enum class _RegexExecutorPolicy : int { _S_auto, _S_alternate };

  template<typename _Bi_iter>
    struct _Regex_scope
    {
      using _Char = typename iterator_traits<_Bi_iter>::value_type;

      template<typename _Executer, typename _Traits, bool __is_ecma>
	class _Context;

      class _Capture
      {
      private:
	enum _State
	{
	  _S_none,
	  _S_only_left,
	  _S_left_and_right,
	};

      public:
	_Capture() : _M_state(_S_none) {}

	bool
	_M_matched() const
	{
	  _GLIBCXX_DEBUG_ASSERT(_M_state != _S_only_left);
	  return _M_state == _S_left_and_right;
	}

	_Bi_iter
	_M_get_left() const
	{
	  _GLIBCXX_DEBUG_ASSERT(_M_state != _S_none);
	  return _M_left;
	}

	void
	_M_set_left(_Bi_iter __iter)
	{
	  _GLIBCXX_DEBUG_ASSERT(_M_state != _S_only_left);
	  _M_state = _S_only_left;
	  _M_left = __iter;
	}

	_Bi_iter
	_M_get_right() const
	{
	  _GLIBCXX_DEBUG_ASSERT(_M_state == _S_left_and_right);
	  return _M_right;
	}

	void
	_M_set_right(_Bi_iter __iter)
	{
	  _GLIBCXX_DEBUG_ASSERT(_M_state == _S_only_left);
	  _M_state = _S_left_and_right;
	  _M_right = __iter;
	}

	bool
	operator<(const _Capture& __rhs) const;

      private:
	_State _M_state;
	_Bi_iter _M_left;
	_Bi_iter _M_right;
      };

      using _Captures = std::vector<_Capture>;

      static bool
      _M_leftmost_longest(const _Captures& __lv, const _Captures& __rv)
      {
	_GLIBCXX_DEBUG_ASSERT(__lv.size() == __rv.size());
	return __lv < __rv;
      }

      struct _Match_head
      {
	_Match_head()
	{ _M_found = false; }

	_Match_head(_StateIdT __state, size_t __size) : _Match_head()
	{
	  _M_state = __state;
	  _M_parens.resize(__size);
	}

	_StateIdT _M_state;
	_Captures _M_parens;
	bool _M_found;
      };

      struct _Saved_tag
      {
	enum _Type
	  {
	    _S_saved_state,
	    _S_saved_paren,
	    _S_saved_position,
	    _S_saved_dfs_repeat,
	  };

	explicit
	_Saved_tag(_Type __tag) : _M_tag(__tag) { }

	_Type _M_tag;
      };

      struct _Saved_state : public _Saved_tag
      {
	explicit
	_Saved_state(_StateIdT __state)
	: _Saved_tag(_Saved_tag::_S_saved_state), _M_state(__state) { }

	_StateIdT _M_state;
      };

      struct _Saved_paren : public _Saved_tag
      {
	explicit
	_Saved_paren(unsigned int __index, _Capture __paren)
	: _Saved_tag(_Saved_tag::_S_saved_paren), _M_index(__index), _M_paren(std::move(__paren)) { }

	unsigned int _M_index;
	_Capture _M_paren;
      };

      struct _Saved_position : public _Saved_tag
      {
	explicit
	_Saved_position(_Bi_iter __position)
	: _Saved_tag(_Saved_tag::_S_saved_position), _M_position(std::move(__position)) { }

	_Bi_iter _M_position;
      };

      struct _Saved_dfs_repeat : public _Saved_tag
      {
	explicit
	_Saved_dfs_repeat(_StateIdT __next, const std::pair<int, _Bi_iter>& __last, const _Bi_iter& __current)
	: _Saved_tag(_Saved_tag::_S_saved_dfs_repeat), _M_next(__next), _M_last(__last), _M_current(__current) { }

	_StateIdT _M_next;
	std::pair<int, _Bi_iter> _M_last;
	_Bi_iter _M_current;
      };

      class _Stack_handlers
      {
      public:
	explicit
	_Stack_handlers(_Dynamic_stack& __stack) : _M_stack(__stack) { }

	template<typename _Tp, typename... _Args>
	  void
	  _M_push(_Args&&... __args)
	  { _M_stack._M_push<_Tp>(_Tp(std::forward<_Args>(__args)...)); }

	template<typename _Context_t>
	  void
	  _M_exec(_Match_head& __head, _Context_t& __context);

      private:
	template<typename _Context_t>
	  static void
	  _M_handle(const _Saved_state& __save, _Context_t& __context, _Match_head& __head);

	template<typename _Context_t>
	  static void
	  _M_handle(const _Saved_paren& __save, _Context_t& __context, _Match_head& __head);

	template<typename _Context_t>
	  static void
	  _M_handle(const _Saved_position& __save, _Context_t& __context, _Match_head& __head);

	template<typename _Context_t>
	  static void
	  _M_handle(const _Saved_dfs_repeat& __save, _Context_t& __context, _Match_head& __head);

	void
	_M_cleanup(void* __old_top) noexcept;

	_Dynamic_stack& _M_stack;
      };

      template<typename _Traits, bool __is_ecma>
	class _Context_new
	{
	public:
	  void
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

	  const _Traits&
	  _M_traits() const
	  { return _M_nfa->_M_traits; }

	  const _State<_Traits>&
	  _M_get_state(_StateIdT __state_id) const
	  { return (*_M_nfa)[__state_id]; }

	  bool
	  _M_is_word(_Char __ch) const
	  {
	    static const _Char __s[2] = { 'w' };
	    return _M_traits().isctype
	      (__ch, _M_traits().lookup_classname(__s, __s+1));
	  }

	  bool
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

	  bool
	  _M_match_backref(unsigned int __index, _Match_head& __head)
	  {
	    const auto& __capture = __head._M_parens[__index];
	    if (!__capture._M_matched())
	      return false;

	    auto __new_current = _M_current;
	    auto __start = __capture._M_get_left();
	    auto __end = __capture._M_get_right();
	    bool __ret = [&]
	      {
		for (auto __len = std::distance(__start, __end); __len > 0; __len--)
		  {
		    if (__new_current == _M_end)
		      return false;
		    ++__new_current;
		  }
		if (_M_nfa->_M_options() & regex_constants::collate)
		  return _M_traits().transform(__start, __end)
		    == _M_traits().transform(_M_current, __new_current);
		return std::equal(__start, __end, _M_current);
	      }();
	    if (__ret)
	      {
		_M_current = __new_current;
		return true;
	      }
	    return false;
	  }

	  _Bi_iter _M_begin;
	  _Bi_iter _M_end;
	  _Bi_iter _M_current;
	  const _NFA<_Traits>* _M_nfa;
	  regex_constants::match_flag_type _M_flags;
	  _Regex_search_mode _M_search_mode;
	};

      class _Dfs_ecma_mixin
      {
      public:
	void _M_reset(_StateIdT __start, size_t __size)
	{
	  _M_head._M_state = __start;
	  _M_head._M_parens.clear();
	  _M_head._M_parens.resize(__size);
	}

	_Match_head&
	_M_get_head()
	{ return _M_head; }

	void
	_M_get_result(_Captures& __result)
	{ __result = std::move(_M_head._M_parens); }

	void
	_M_update(const _Captures& __captures) { }

      private:
	_Match_head _M_head;
      };

      class _Dfs_posix_mixin
      {
      public:
	void _M_reset(_StateIdT __start, size_t __size)
	{
	  _M_head._M_state = __start;
	  _M_head._M_parens.clear();
	  _M_head._M_parens.resize(__size);
	  _M_result.clear();
	}

	_Match_head&
	_M_get_head()
	{ return _M_head; }

	void
	_M_get_result(_Captures& __result)
	{ __result = std::move(_M_result); }

	void
	_M_update(const _Captures& __captures)
	{
	  if (_M_result.empty() || _M_leftmost_longest(__captures, _M_result))
	    _M_result = __captures;
	}

      private:
	_Match_head _M_head;
	_Captures _M_result;
      };

      template<typename _Traits, bool __is_ecma>
	class _Dfs_executer : private std::conditional<__is_ecma, _Dfs_ecma_mixin, _Dfs_posix_mixin>::type
	{
	private:
	  using _Context_t = _Context_new<_Traits, __is_ecma>;

	public:
	  _Dfs_executer() : _M_stack(__get_dynamic_stack()) { }

	  void
	  _M_init(_Context_t&& __context)
	  { _M_context = std::move(__context); }

	  template<typename _Bp, typename _Alloc>
	    bool
	    _M_match(std::vector<sub_match<_Bp>, _Alloc>& __res)
	    {
	      _Captures __result;
	      if (_M_match_impl(_M_context._M_nfa->_M_start(), _M_context._M_nfa->_M_sub_count(), __result))
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

	private:
	  bool
	  _M_match_impl(_StateIdT __start, size_t __size, _Captures& __result)
	  {
	    if (_M_context._M_search_mode == _Regex_search_mode::_Exact)
	      return _M_search_from_first_helper(__start, __result);
	    if (_M_search_from_first_helper(__start, __result))
	      return true;
	    if (_M_context._M_flags & regex_constants::match_continuous)
	      return false;
	    _M_context._M_flags |= regex_constants::match_prev_avail;
	    while (_M_context._M_begin != _M_context._M_end)
	      {
		++_M_context._M_begin;
		if (_M_search_from_first_helper(__start, __result))
		  return true;
	      }
	    return false;
	  }

	  bool
	  _M_search_from_first_helper(_StateIdT __start, _Captures& __result)
	  {
	    _M_context._M_current = _M_context._M_begin;
	    return _M_search_from_first(__start, _M_context._M_nfa->_M_sub_count(), __result);
	  }

	  void
	  _M_dfs(_Match_head& __head)
	  {
#define __TAIL_RECURSE(__new_state) { __head._M_state = (__new_state); continue; }
	    while (1)
	      {
		const auto& __state = _M_context._M_get_state(__head._M_state);
		auto& __current = _M_context._M_current;
		switch (__state._M_opcode)
		  {
		  case _S_opcode_repeat:
		    if (_M_handle_repeat(__state, __head))
		      continue;
		    break;
		  case _S_opcode_subexpr_begin:
		    {
		      auto& __paren = __head._M_parens[__state._M_subexpr];
		      _M_push<_Saved_paren>(__state._M_subexpr, __paren);
		      __paren._M_set_left(__current);
		      __TAIL_RECURSE(__state._M_next)
		      break;
		    }
		  case _S_opcode_subexpr_end:
		    {
		      auto& __paren = __head._M_parens[__state._M_subexpr];
		      _M_push<_Saved_paren>(__state._M_subexpr, __paren);
		      __paren._M_set_right(__current);
		      __TAIL_RECURSE(__state._M_next)
		    }
		    break;
		  case _S_opcode_alternative:
		    _M_push<_Saved_state>(__state._M_next);
		    _M_handle_alt();
		    __TAIL_RECURSE(__state._M_alt)
		    break;
		  case _S_opcode_line_begin_assertion:
		    if (__current == _M_context._M_begin
			&& !(_M_context._M_flags & (regex_constants::match_not_bol | regex_constants::match_prev_avail)))
		      __TAIL_RECURSE(__state._M_next)
		    break;
		  case _S_opcode_line_end_assertion:
		    if (__current == _M_context._M_end && !(_M_context._M_flags & regex_constants::match_not_eol))
		      __TAIL_RECURSE(__state._M_next)
		    break;
		  case _S_opcode_word_boundary:
		    if (_M_context._M_word_boundary() == !__state._M_neg)
		      __TAIL_RECURSE(__state._M_next)
		    break;
		  case _S_opcode_subexpr_lookahead:
		    {
		      _Dfs_executer __executer;
		      {
			_Context_t __context;
			__context._M_init(__current, _M_context._M_end, *_M_context._M_nfa, _M_context._M_flags, _M_context._M_search_mode);
			__executer._M_init(std::move(__context));
		      }
		      _Captures __captures;
		      bool __ret = __executer._M_match_impl(__state._M_alt, __head._M_parens.size(), __captures);
		      if (__ret != __state._M_neg)
			{
			  if (__ret)
			    {
			      auto& __res = __head._M_parens;
			      _GLIBCXX_DEBUG_ASSERT(__res.size() == __captures.size());
			      for (size_t __i = 0; __i < __captures.size(); __i++)
				if (__captures[__i]._M_matched())
				  {
				    _M_push<_Saved_paren>(__i, __res[__i]);
				    __res[__i] = __captures[__i];
				  }
			    }
			  __TAIL_RECURSE(__state._M_next)
			}
		    }
		    break;
		  case _S_opcode_match:
		    if (_M_context._M_end == __current)
		      break;
		    if (_M_handle_match(__state, __head))
		      continue;
		    break;
		  case _S_opcode_backref:
		    if (_M_handle_backref(__state, __head))
		      continue;
		    break;
		  case _S_opcode_accept:
		    if (!__head._M_found)
		      {
			if (_M_context._M_search_mode == _Regex_search_mode::_Exact)
			  __head._M_found = _M_context._M_end == __current;
			else
			  __head._M_found = true;
			if (_M_context._M_flags & regex_constants::match_not_null)
			  __head._M_found = __head._M_found && _M_context._M_begin != __current;
		      }
		    if (__head._M_found)
		      _M_handle_accept(__head._M_parens);
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

	private:
	  void
	  _M_exec(_Match_head& __head)
	  {
	    const auto __cleanup = [this](void* __old_top)
	    {
	      if (is_trivially_destructible<_Bi_iter>::value)
		_M_stack._M_jump(__old_top);
	      else
		while (_M_stack._M_top() != __old_top)
		  switch (_M_stack._M_top_item<_Saved_tag>()._M_tag)
		    {
		    case _Saved_tag::_S_saved_state:
		      _M_stack._M_pop<_Saved_state>(); break;
		    case _Saved_tag::_S_saved_paren:
		      _M_stack._M_pop<_Saved_paren>(); break;
		    case _Saved_tag::_S_saved_position:
		      _M_stack._M_pop<_Saved_position>(); break;
		    case _Saved_tag::_S_saved_dfs_repeat:
		      _M_stack._M_pop<_Saved_dfs_repeat>(); break;
		    };
	    };

	    void* __top = _M_stack._M_top();
	    __try
	      {
		_M_handle(_Saved_state(__head._M_state), __head);
		while (_M_stack._M_top() != __top)
		  {
		    if (__is_ecma && __head._M_found)
		      {
			__cleanup(__top);
			break;
		      }
		    switch (_M_stack.template _M_top_item<_Saved_tag>()._M_tag)
		      {
#define __HANDLE(_Type) \
			  {\
			    auto __save = std::move(_M_stack._M_top_item<_Type>());\
			    _M_stack.template _M_pop<_Type>();\
			    _M_handle(__save, __head);\
			  }
		      case _Saved_tag::_S_saved_state: __HANDLE(_Saved_state); break;
		      case _Saved_tag::_S_saved_paren: __HANDLE(_Saved_paren); break;
		      case _Saved_tag::_S_saved_position: __HANDLE(_Saved_position); break;
		      case _Saved_tag::_S_saved_dfs_repeat: __HANDLE(_Saved_dfs_repeat); break;
#undef __HANDLE
		      };
		  }
	      }
	    __catch (...)
	      {
		__cleanup(__top);
		__throw_exception_again;
	      }
	  }

	  template<typename _Tp, typename... _Args>
	    void
	    _M_push(_Args&&... __args)
	    { _M_stack._M_push<_Tp>(_Tp(std::forward<_Args>(__args)...)); }

	  bool
	  _M_search_from_first(_StateIdT __start, size_t __size, _Captures& __result)
	  {
	    _M_last.first = 0;
	    this->_M_reset(__start, __size);
	    _M_exec(this->_M_get_head());
	    if (this->_M_get_head()._M_found)
	      {
		this->_M_get_result(__result);
		return true;
	      }
	    return false;
	  }

	  bool
	  _M_handle_repeat(const _State<_Traits>& __state, _Match_head& __head)
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
		_M_last.first = 0;
		_M_last.second = __current;
	      }
	    _M_last.first++;

	    __head._M_state = __first;
	    return true;
	  }

	  bool
	  _M_handle_alt()
	  { this->_M_push<_Saved_position>(_M_context._M_current); }

	  bool
	  _M_handle_match(const _State<_Traits>& __state, _Match_head& __head)
	  {
	    if (__state._M_matches(*_M_context._M_current))
	      {
		++_M_context._M_current;
		__head._M_state = __state._M_next;
		return true;
	      }
	    return false;
	  }

	  bool
	  _M_handle_backref(const _State<_Traits>& __state, _Match_head& __head)
	  {
	    if (_M_context._M_match_backref(__state._M_backref_index, __head))
	      {
		__head._M_state = __state._M_next;
		return true;
	      }
	    return false;
	  }

	  void
	  _M_handle_accept(const _Captures& __captures)
	  { this->_M_update(__captures); }

	  void
	  _M_handle(const _Saved_state& __save, _Match_head& __head)
	  {
	    __head._M_state = __save._M_state;
	    _M_dfs(__head);
	  }

	  void
	  _M_handle(const _Saved_paren& __save, _Match_head& __head)
	  { __head._M_parens[__save._M_index] = std::move(__save._M_paren); }

	  void
	  _M_handle(const _Saved_position& __save, _Match_head& __head)
	  { _M_context._M_current = std::move(__save._M_position); }

	  void
	  _M_handle(const _Saved_dfs_repeat& __save, _Match_head& __head)
	  {
	    _M_last = __save._M_last;
	    _M_context._M_current = __save._M_current;
	    __head._M_state = __save._M_next;
	    _M_dfs(__head);
	  }

	private:
	  _Context_t _M_context;
	  std::pair<int, _Bi_iter> _M_last;
	  _Dynamic_stack& _M_stack;
	};

      class _Bfs_ecma_mixin
      {
      public:
	void
	_M_reset(size_t __size)
	{ _M_is_visited.resize(__size); }

	void
	_M_clear()
	{ std::fill_n(_M_is_visited.begin(), _M_is_visited.size(), false); }

	bool
	_M_visited_impl(_StateIdT __state_id, _Match_head& __head);

	void
	_M_visit_impl(_StateIdT __state_id, _Match_head& __head)
	{ _M_is_visited[__state_id] = true; }

      private:
	std::vector<bool> _M_is_visited;
      };

      class _Bfs_posix_mixin
      {
      public:
	void
	_M_reset(size_t __size)
	{
	  _M_is_visited.resize(__size);
	  _M_current_positions.resize(__size);
	}

	void
	_M_clear()
	{ std::fill_n(_M_is_visited.begin(), _M_is_visited.size(), false); }

	bool
	_M_visited_impl(_StateIdT __state_id, _Match_head& __head);

	void
	_M_visit_impl(_StateIdT __state_id, _Match_head& __head)
	{
	  _M_is_visited[__state_id] = true;
	  _M_current_positions[__state_id] = __head._M_parens;
	}

      private:
	std::vector<bool> _M_is_visited;
	std::vector<_Captures> _M_current_positions;
      };

      template<typename _Traits, bool __is_ecma>
	class _Bfs_executer : private std::conditional<__is_ecma, _Bfs_ecma_mixin, _Bfs_posix_mixin>::type
	{
	private:
	  using _Context_t = _Context<_Bfs_executer, _Traits, __is_ecma>;

	public:
	  _Bfs_executer() : _M_stack(__get_dynamic_stack()) {}

	  template<typename _Tp, typename... _Args>
	    void
	    _M_push(_Args&&... __args)
	    { _M_stack._M_push<_Tp>(_Tp(std::forward<_Args>(__args)...)); }

	  bool
	  _M_search_from_first(_Context_t& __context, _StateIdT __start, size_t __size, _Captures& __result);

	  bool
	  _M_visited(_StateIdT __state_id, _Match_head& __head)
	  { return this->_M_visited_impl(__state_id, __head); }

	  void
	  _M_visit(_StateIdT __state_id, _Match_head& __head)
	  { return this->_M_visit_impl(__state_id, __head); }

	  bool
	  _M_handle_repeat(_Context_t& __context, const _State<_Traits>& __state, _Match_head& __head);

	  bool
	  _M_handle_alt(_Context_t& __context) { }

	  bool
	  _M_handle_match(_Context_t& __context, const _State<_Traits>& __state, const _Match_head& __head);

	  void
	  _M_handle_accept(const _Captures& __captures);

	  bool
	  _M_handle_backref(_Context_t& __context, const _State<_Traits>& __state, _Match_head& __head)
	  {
	    _GLIBCXX_DEBUG_ASSERT(false);
	    return false;
	  }

	  void
	  _M_restore_dfs_repeat(const _Saved_dfs_repeat& __save, _Context_t& __context, _Match_head& __head) { }

	private:
	  bool
	  _M_search_from_first_impl(_Context_t& __context);

	  std::vector<_Match_head> _M_heads;
	  _Captures _M_result;
	  _Stack_handlers _M_stack;
	  bool _M_found;
	};

      template<typename _Executer, typename _Traits, bool __is_ecma>
	class _Context
	{
	public:
	  static constexpr bool _S_is_ecma = __is_ecma;

	  void
	  _M_init(_Bi_iter __begin, _Bi_iter __end, const _NFA<_Traits>& __nfa,
		  regex_constants::match_flag_type __flags,
		  _Regex_search_mode __search_mode);

	  template<typename _Bp, typename _Alloc>
	    bool
	    _M_match(std::vector<sub_match<_Bp>, _Alloc>& __res);

	private:
	  bool
	  _M_match_impl(_StateIdT __start, size_t __size, _Captures& __result);

	  bool
	  _M_search_from_first_helper(_StateIdT __start, _Captures& __result)
	  {
	    _M_current = _M_begin;
	    return _M_executer._M_search_from_first(*this, __start, _M_nfa->_M_sub_count(), __result);
	  }

	  void
	  _M_dfs(_Match_head& __head);

	  const _Traits&
	  _M_traits() const
	  { return _M_nfa->_M_traits; }

	  const _State<_Traits>&
	  _M_get_state(_StateIdT __state_id) const
	  { return (*_M_nfa)[__state_id]; }

	  bool
	  _M_is_word(_Char __ch) const;

	  bool
	  _M_word_boundary() const;

	  bool
	  _M_match_backref(unsigned int __index, _Match_head& __head);

	  _Bi_iter _M_begin;
	  _Bi_iter _M_end;
	  _Bi_iter _M_current;
	  const _NFA<_Traits>* _M_nfa;
	  regex_constants::match_flag_type _M_flags;
	  _Regex_search_mode _M_search_mode;
	  _Executer _M_executer;

	  friend class _Stack_handlers;
	  template<typename, bool> friend class _Bfs_executer;
	};

      template<_RegexExecutorPolicy __policy, typename _Bp, typename _Alloc, typename _Traits>
	static bool
	__run(_Bi_iter __s, _Bi_iter __e,
	      std::vector<sub_match<_Bp>, _Alloc>& __res,
	      const _NFA<_Traits>& __nfa,
	      regex_constants::match_flag_type __flags,
	      _Regex_search_mode __search_mode);
    };

 //@} regex-detail
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace __detail
} // namespace std

#include <bits/regex_backend.tcc>
