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
 *  @file bits/regex_executer.h
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
    _Dynamic_stack() : _M_blocks(1), _M_base(_M_top()) { }

    template<typename _Tp>
      void
      _M_push(_Tp&& __val);

    template<typename _Tp>
      void
      _M_pop() noexcept;

    template<typename _Tp>
      _Tp&
      _M_top();

    void*
    _M_top() const noexcept
    { return _M_blocks.front()._M_top; }

  private:
    struct _Block
    {
      _Block() : _M_top(std::end(_M_data)) { }

      size_t
      _M_avail() const
      { return static_cast<char*>(_M_top) - _M_data; }

      char _M_data[_GLIBCXX_REGEX_STACK_BLOCK_SIZE];
      void* _M_top;
    };

    template<typename _Tp>
      static constexpr size_t _S_sizeof()
      { return ((sizeof(_Tp) - 1) & (~7)) + 8; }

    template<size_t __size>
      size_t
      _M_allocate();

    void
    _M_deallocate(size_t __size) noexcept;

    void*&
    _M_top_frame() noexcept
    { return _M_blocks.front()._M_top; }

    std::forward_list<_Block> _M_blocks;
    void* _M_base;
  };

  template<typename _Bi_iter>
    class _Comparable_iter : public _Bi_iter
    {
    public:
      _Comparable_iter() = default;

      explicit
      _Comparable_iter(_Bi_iter __iter)
      : _Bi_iter(std::move(__iter)), _M_position(0) { }

      _Comparable_iter(const _Comparable_iter& __rhs) = default;

      _Comparable_iter&
      operator=(const _Comparable_iter& __rhs) = default;

      _Comparable_iter&
      operator++()
      {
	_M_position++;
	_Bi_iter::operator++();
	return *this;
      }

      _Comparable_iter
      operator++(int)
      {
	auto __tmp = *this;
	++*this;
	return __tmp;
      }

      _Comparable_iter&
      operator--()
      {
	_M_position--;
	_Bi_iter::operator--();
	return *this;
      }

      _Comparable_iter
      operator--(int)
      {
	auto __tmp = *this;
	--*this;
	return __tmp;
      }

      bool
      operator<(const _Comparable_iter& __rhs) const
      { return _M_position < __rhs._M_position; }

      bool
      operator>(const _Comparable_iter& __rhs) const
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
      static _Comparable_iter<_Bi_iter>
      __test(...);

    template<typename _Bi_iter>
      using type = decltype(__test<_Bi_iter>(0));
  };

  inline _Dynamic_stack&
  __get_dynamic_stack()
  {
    thread_local _Dynamic_stack __stack;
    return __stack;
  }

  enum class _Regex_search_mode : unsigned char { _Exact, _Prefix };

  enum class _RegexExecutorPolicy : int { _S_auto, _S_alternate };

  template<typename _Bi_iter>
    struct _Regex_run
    {
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
	operator<(const _Capture& __rhs) const
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

      struct _Regex_context
      {
	_Regex_context()
	{
	  _M_last.first = 0;
	  _M_found = false;
	}

	_Regex_context(_StateIdT __state, size_t __size) : _Regex_context()
	{
	  _M_state = __state;
	  _M_parens.resize(__size);
	}

	_StateIdT _M_state;
	std::pair<int, _Bi_iter> _M_last;
	_Captures _M_parens;
	bool _M_found;
      };

      struct _Regex_stack
      {
      public:
	struct _Tag
	{
	  enum _Type
	    {
	      _S_Saved_state,
	      _S_Saved_paren,
	      _S_Saved_position,
	      _S_Saved_last,
	    };

	  explicit
	  _Tag(_Type __tag) : _M_tag(__tag) { }

	  _Type _M_tag;
	};

	struct _Saved_state : public _Tag
	{
	  explicit
	  _Saved_state(_StateIdT __state)
	  : _Tag(_Tag::_S_Saved_state), _M_state(__state) { }

	  _StateIdT _M_state;
	};

	struct _Saved_paren : public _Tag
	{
	  explicit
	  _Saved_paren(unsigned int __index, _Capture __paren)
	  : _Tag(_Tag::_S_Saved_paren), _M_index(__index), _M_paren(std::move(__paren)) { }

	  unsigned int _M_index;
	  _Capture _M_paren;
	};

	struct _Saved_position : public _Tag
	{
	  explicit
	  _Saved_position(_Bi_iter __position)
	  : _Tag(_Tag::_S_Saved_position), _M_position(std::move(__position)) { }

	  _Bi_iter _M_position;
	};

	struct _Saved_last : public _Tag
	{
	  explicit
	  _Saved_last(std::pair<int, _Bi_iter> __last)
	  : _Tag(_Tag::_S_Saved_last), _M_last(__last) { }

	  std::pair<int, _Bi_iter> _M_last;
	};

	explicit
	_Regex_stack(_Dynamic_stack& __stack) : _M_stack(__stack) { }

	template<typename _Tp, typename... _Args>
	  void
	  _M_push(_Args&&... __args)
	  { _M_stack._M_push<_Tp>(_Tp(std::forward<_Args>(__args)...)); }

	template<typename _Runner>
	  void
	  _M_exec(_Regex_context& __context, _Runner& __runner);

	template<typename _Runner>
	  void
	  _M_handle(_Saved_state __save, _Runner& __runner, _Regex_context& __context);

	template<typename _Runner>
	  static void
	  _M_handle(_Saved_paren __save, _Runner& __runner, _Regex_context& __context);

	template<typename _Runner>
	  static void
	  _M_handle(_Saved_position __save, _Runner& __runner, _Regex_context& __context);

	template<typename _Runner>
	  static void
	  _M_handle(_Saved_last __save, _Runner& __runner, _Regex_context& __context);

	void
	_M_cleanup(void* __old_top)
	{
	  while (_M_stack._M_top() != __old_top)
	    switch (_M_stack._M_top<_Tag>()._M_tag)
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

	_Dynamic_stack& _M_stack;
      };

      template<typename _Traits>
	struct _Executer_context
	{
	  using _Char = typename iterator_traits<_Bi_iter>::value_type;

	  void
	  _M_reset()
	  { _M_current = _M_begin; }

	  const _Traits&
	  _M_traits() const
	  { return _M_nfa->_M_traits; }

	  const _State<_Traits>&
	  _M_get_state(_StateIdT __state_id) const
	  { return (*_M_nfa)[__state_id]; }

	  bool
	  _M_is_word(_Char __ch) const;

	  bool
	  _M_at_begin() const;

	  bool
	  _M_at_end() const;

	  bool
	  _M_word_boundary() const;

	  bool
	  _M_match_backref(unsigned int __index, _Regex_context& __context, _Bi_iter& __current);

	  regex_constants::match_flag_type _M_flags;
	  _Regex_search_mode _M_search_mode;
	  _Bi_iter _M_begin;
	  _Bi_iter _M_end;
	  _Bi_iter _M_current;
	  const _NFA<_Traits>* _M_nfa;
	};

      template<typename, typename> class _Regex_runner;

      class _Dfs_ecma_mixin
      {
      public:
	void _M_reset(_StateIdT __start, size_t __size)
	{
	  _M_context._M_state = __start;
	  _M_context._M_parens.clear();
	  _M_context._M_parens.resize(__size);
	}

	_Regex_context&
	_M_get_context()
	{ return _M_context; }

	void
	_M_get_result(_Captures& __result)
	{ __result = std::move(_M_context._M_parens); }

	void
	_M_update(const _Captures& __captures) { }

      private:
	_Regex_context _M_context;
      };

      class _Dfs_posix_mixin
      {
      public:
	void _M_reset(_StateIdT __start, size_t __size)
	{
	  _M_context._M_state = __start;
	  _M_context._M_parens.clear();
	  _M_context._M_parens.resize(__size);
	  _M_result.clear();
	}

	_Regex_context&
	_M_get_context()
	{ return _M_context; }

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
	_Regex_context _M_context;
	_Captures _M_result;
      };

      template<typename _Traits, bool __is_ecma>
	class _Dfs_executer : private std::conditional<__is_ecma, _Dfs_ecma_mixin, _Dfs_posix_mixin>::type
	{
	private:
	  using _Runner = _Regex_runner<_Dfs_executer, _Traits>;

	public:
	  static constexpr bool _S_is_ecma = __is_ecma;

	  bool
	  _M_search_from_first(_Runner& __runner, _StateIdT __start, size_t __size, _Captures& __result)
	  {
	    this->_M_reset(__start, __size);
	    __runner._M_exec(this->_M_get_context());
	    if (this->_M_get_context()._M_found)
	      {
		this->_M_get_result(__result);
		return true;
	      }
	    return false;
	  }

	  bool
	  _M_visited(_StateIdT __state_id, _Regex_context& __context)
	  { return false; }

	  bool
	  _M_handle_match(_Runner& __runner, const _State<_Traits>& __state, _Regex_context& __context)
	  {
	    if (__state._M_matches(*__runner._M_exec_context._M_current))
	      {
		__runner._M_stack.template _M_push<typename _Regex_stack::_Saved_position>(__runner._M_exec_context._M_current);
		++__runner._M_exec_context._M_current;
		__context._M_state = __state._M_next;
		return true;
	      }
	    return false;
	  }

	  bool
	  _M_handle_backref(_Runner& __runner, const _State<_Traits>& __state, _Regex_context& __context)
	  {
	    _Bi_iter __current;
	    if (__runner._M_exec_context._M_match_backref(__state._M_backref_index, __context, __current))
	      {
		__runner._M_stack.template _M_push<typename _Regex_stack::_Saved_position>(std::move(__current));
		__context._M_state = __state._M_next;
		return true;
	      }
	    return false;
	  }

	  void
	  _M_handle_accept(const _Captures& __captures)
	  { this->_M_update(__captures); }
	};

      template<typename _Traits, bool __is_ecma>
	class _Bfs_executer
	{
	private:
	  using _Runner = _Regex_runner<_Bfs_executer, _Traits>;

	public:
	  static constexpr bool _S_is_ecma = __is_ecma;

	  bool
	  _M_visited(_StateIdT __state_id, _Regex_context& __context)
	  {
	    if (_M_is_visited[__state_id])
	      {
		if (__is_ecma)
		  return true;
		if (!_M_leftmost_longest(__context._M_parens, _M_current_positions[__state_id]))
		  return true;
	      }
	    _M_is_visited[__state_id] = true;
	    if (!__is_ecma)
	      _M_current_positions[__state_id] = __context._M_parens;
	    return false;
	  }

	  bool
	  _M_search_from_first(_Runner& __runner, _StateIdT __start, size_t __size, _Captures& __result)
	  {
	    _M_is_visited.resize(__runner._M_exec_context._M_nfa->size());
	    _M_current_positions.resize(__runner._M_exec_context._M_nfa->size());
	    _M_contexts.clear();
	    _M_contexts.emplace_back(__start, __size);
	    if (_M_search_from_first_impl(__runner))
	      {
		__result = std::move(_M_result);
		return true;
	      }
	    return false;
	  }

	  bool
	  _M_search_from_first_impl(_Runner& __runner)
	  {
	    bool __found = false;
	    while (!_M_contexts.empty())
	      {
		_M_found = false;
		std::fill_n(_M_is_visited.begin(), _M_is_visited.size(), false);
		auto __contexts = std::move(_M_contexts);
		for (auto& __context : __contexts)
		  __runner._M_exec(__context);
		__found = __found || _M_found;
		if (!__runner._M_exec_context._M_at_end())
		  ++__runner._M_exec_context._M_current;
	      }
	    return __found;
	  }

	  bool
	  _M_handle_match(_Runner& __runner, const _State<_Traits>& __state, const _Regex_context& __context)
	  {
	    if (!__state._M_matches(*__runner._M_exec_context._M_current))
	      return false;
	    if (__is_ecma && _M_found)
	      return false;
	    _M_contexts.emplace_back(__context);
	    _M_contexts.back()._M_state = __state._M_next;
	    return false;
	  }

	  bool
	  _M_handle_backref(_Runner& __runner, const _State<_Traits>& __state, _Regex_context& __context)
	  {
	    _GLIBCXX_DEBUG_ASSERT(false);
	    return false;
	  }

	  void
	  _M_handle_accept(const _Captures& __captures)
	  {
	    _M_found = true;
	    if (__is_ecma || (_M_result.empty() || _M_leftmost_longest(__captures, _M_result)))
	      _M_result = __captures;
	  }

	private:
	  std::vector<bool> _M_is_visited;
	  std::vector<_Captures> _M_current_positions;
	  std::vector<_Regex_context> _M_contexts;
	  _Captures _M_result;
	  bool _M_found;
	};

      template<typename _Executer, typename _Traits>
	class _Regex_runner
	{
	private:
	  static constexpr bool _S_is_ecma = _Executer::_S_is_ecma;

	public:
	  _Regex_runner(_Dynamic_stack& __dynamic_stack) : _M_stack(__dynamic_stack) { }

	  void
	  _M_init(_Bi_iter __begin, _Bi_iter __end, const _NFA<_Traits>& __nfa,
		  regex_constants::match_flag_type __flags,
		  _Regex_search_mode __search_mode)
	  {
	    _M_exec_context._M_begin = __begin;
	    _M_exec_context._M_end = __end;
	    _M_exec_context._M_flags = __flags;
	    _M_exec_context._M_nfa = &__nfa;
	    _M_exec_context._M_search_mode = __search_mode;
	  }

	  template<typename _Bp, typename _Alloc>
	    bool
	    _M_match(std::vector<sub_match<_Bp>, _Alloc>& __res)
	    {
	      _Captures __result;
	      if (_M_match_impl(_M_exec_context._M_nfa->_M_start(), _M_exec_context._M_nfa->_M_sub_count(), __result))
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
	  void
	  _M_exec(_Regex_context& __context)
	  { _M_stack._M_exec(__context, *this); }

	  void
	  _M_dfs(_Regex_context& __context);

	  bool
	  _M_search_from_first_helper(_StateIdT __start, _Captures& __result)
	  {
	    _M_exec_context._M_reset();
	    return _M_executer._M_search_from_first(*this, __start, _M_exec_context._M_nfa->_M_sub_count(), __result);
	  }

	  bool
	  _M_match_impl(_StateIdT __start, size_t __size, _Captures& __result)
	  {
	    if (_M_exec_context._M_search_mode == _Regex_search_mode::_Exact)
	      return _M_search_from_first_helper(__start, __result);
	    if (_M_search_from_first_helper(__start, __result))
	      return true;
	    if (_M_exec_context._M_flags & regex_constants::match_continuous)
	      return false;
	    _M_exec_context._M_flags |= regex_constants::match_prev_avail;
	    while (_M_exec_context._M_begin != _M_exec_context._M_end)
	      {
		++_M_exec_context._M_begin;
		if (_M_search_from_first_helper(__start, __result))
		  return true;
	      }
	    return false;
	  }

	private:
	  _Regex_stack _M_stack;
	  _Executer_context<_Traits> _M_exec_context;
	  _Executer _M_executer;

	  friend class _Regex_stack;
	  template<typename, bool> friend class _Dfs_executer;
	  template<typename, bool> friend class _Bfs_executer;
	};

      template<_RegexExecutorPolicy __policy, typename _Bp, typename _Alloc, typename _Traits>
	static bool
	__run(_Bi_iter __s, _Bi_iter __e,
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
#define _RUN(_EXECUTER_TYPE, __IS_ECMA) \
	    do \
	      { \
		_Regex_runner<_EXECUTER_TYPE<_Traits, __IS_ECMA>, _Traits> __runner(__get_dynamic_stack()); \
		__runner._M_init(__s, __e, __nfa, __flags, __search_mode); \
		return __runner._M_match(__res); \
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
	}
    };

 //@} regex-detail
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace __detail
} // namespace std

#include <bits/regex_executer.tcc>
