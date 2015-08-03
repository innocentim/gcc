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
    _Dynamic_stack(const _Dynamic_stack&) = delete;

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
    _Dynamic_stack() : _M_blocks(1), _M_base(_M_end()), _M_top_ptr(_M_end()) { }

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

    friend _Dynamic_stack&
    __get_dynamic_stack();

    std::forward_list<_Block> _M_blocks;
    void* _M_base;
    void* _M_top_ptr;
  };

  inline _Dynamic_stack&
  __get_dynamic_stack()
  {
    static _Dynamic_stack __stack;
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
      using _Char_type = typename iterator_traits<_Bi_iter>::value_type;

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
	  _M_captures.resize(__size);
	}

	_StateIdT _M_state;
	_Captures _M_captures;
	bool _M_found;
      };

      struct _Saved_tag
      {
	enum _Type
	  {
	    _S_saved_capture,
	    _S_saved_dfs_repeat,
	    _S_saved_dfs_neg_repeat,
	    _S_saved_last,
	    _S_saved_state,
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

      struct _Saved_capture : public _Saved_tag
      {
	explicit
	_Saved_capture(unsigned int __index, _Capture __capture)
	: _Saved_tag(_Saved_tag::_S_saved_capture), _M_index(__index), _M_capture(std::move(__capture)) { }

	unsigned int _M_index;
	_Capture _M_capture;
      };

      struct _Saved_dfs_repeat : public _Saved_tag
      {
	_Saved_dfs_repeat(_StateIdT __next, std::pair<_StateIdT, _Bi_iter> __last, _Bi_iter __current)
	: _Saved_tag(_Saved_tag::_S_saved_dfs_repeat), _M_next(__next), _M_last(std::move(__last)), _M_current(std::move(__current)) { }

	_StateIdT _M_next;
	std::pair<_StateIdT, _Bi_iter> _M_last;
	_Bi_iter _M_current;
      };

      struct _Saved_dfs_neg_repeat : public _Saved_tag
      {
	_Saved_dfs_neg_repeat(_StateIdT __original, _StateIdT __next)
	: _Saved_tag(_Saved_tag::_S_saved_dfs_neg_repeat), _M_original(__original), _M_next(__next) { }

	_StateIdT _M_original;
	_StateIdT _M_next;
      };

      struct _Saved_last : public _Saved_tag
      {
	explicit
	_Saved_last(std::pair<_StateIdT, _Bi_iter> __last)
	: _Saved_tag(_Saved_tag::_S_saved_last), _M_last(std::move(__last)) { }

	std::pair<_StateIdT, _Bi_iter> _M_last;
      };

      template<typename _Traits>
	class _Context
	{
	public:
	  _Context() : _M_stack(__get_dynamic_stack()) { }

	  void
	  _M_init(_Bi_iter __begin, _Bi_iter __end, const _NFA<_Traits>& __nfa,
		  regex_constants::match_flag_type __flags,
		  _Regex_search_mode __search_mode);

	  const _Traits&
	  _M_traits() const
	  { return _M_nfa->_M_traits; }

	  const _State<_Char_type>&
	  _M_get_state(_StateIdT __state_id) const
	  { return (*_M_nfa)[__state_id]; }

	  bool
	  _M_is_word(_Char_type __ch) const
	  {
	    static const _Char_type __s[2] = { 'w' };
	    return _M_traits().isctype
	      (__ch, _M_traits().lookup_classname(__s, __s+1));
	  }

	  bool
	  _M_word_boundary() const;

	  _Bi_iter _M_begin;
	  _Bi_iter _M_end;
	  _Bi_iter _M_current;
	  const _NFA<_Traits>* _M_nfa;
	  regex_constants::match_flag_type _M_flags;
	  _Regex_search_mode _M_search_mode;
	  _Dynamic_stack& _M_stack;
	};

      template<typename _Executer>
	static void
	_M_dfs(_Executer& __executer, _Match_head& __head);

      template<typename _Traits>
	static bool
	_M_handle_subexpr_begin_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head);

      template<typename _Traits>
	static bool
	_M_handle_subexpr_end_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head);

      template<typename _Traits>
	static bool
	_M_handle_alternative_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head);

      template<typename _Traits>
	static bool
	_M_handle_line_begin_assertion_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head);

      template<typename _Traits>
	static bool
	_M_handle_line_end_assertion_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head);

      template<typename _Traits>
	static bool
	_M_handle_word_boundary_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head);

      template<typename _Executer, typename _Traits>
	static bool
	_M_handle_subexpr_lookahead_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head);

      template<typename _Traits>
	static void
	_M_handle_accept_common(_Context<_Traits>& __context, const _State<_Char_type>& __state, _Match_head& __head);

      class _Dfs_ecma_mixin
      {
      public:
	void _M_reset(_StateIdT __start, size_t __size)
	{
	  _M_head._M_state = __start;
	  _M_head._M_captures.clear();
	  _M_head._M_captures.resize(__size);
	}

	_Match_head&
	_M_get_head()
	{ return _M_head; }

	void
	_M_get_result(_Captures& __result)
	{ __result = std::move(_M_head._M_captures); }

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
	  _M_head._M_captures.clear();
	  _M_head._M_captures.resize(__size);
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
	public:
	  _Context<_Traits>&
	  _M_get_context()
	  { return _M_context; }

	  bool
	  _M_search_from_first(_StateIdT __start, _Captures& __result);

	private:
	  void
	  _M_exec(_Match_head& __head);

	  template<typename _Tp, typename... _Args>
	    void
	    _M_push(_Args&&... __args)
	    { _M_context._M_stack._M_push<_Tp>(_Tp(std::forward<_Args>(__args)...)); }

	  bool
	  _M_visited(_StateIdT __state_id, _Match_head& __head)
	  { return false; }

	  void
	  _M_visit(_StateIdT __state_id, _Match_head& __head)
	  { }

	  bool
	  _M_handle_repeat(const _State<_Char_type>& __state, _Match_head& __head);

	  bool
	  _M_handle_subexpr_begin(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_subexpr_begin_common(_M_context, __state, __head); }

	  bool
	  _M_handle_subexpr_end(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_subexpr_end_common(_M_context, __state, __head); }

	  bool
	  _M_handle_alternative(const _State<_Char_type>& __state, _Match_head& __head);

	  bool
	  _M_handle_line_begin_assertion(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_line_begin_assertion_common(_M_context, __state, __head); }

	  bool
	  _M_handle_line_end_assertion(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_line_end_assertion_common(_M_context, __state, __head); }

	  bool
	  _M_handle_word_boundary(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_word_boundary_common(_M_context, __state, __head); }

	  bool
	  _M_handle_subexpr_lookahead(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_subexpr_lookahead_common<_Dfs_executer>(_M_context, __state, __head); }

	  bool
	  _M_handle_match(const _State<_Char_type>& __state, _Match_head& __head);

	  bool
	  _M_handle_backref(const _State<_Char_type>& __state, _Match_head& __head);

	  bool
	  _M_handle_accept(const _State<_Char_type>& __state, _Match_head& __head);

	  void
	  _M_restore_state(_Dynamic_stack& __stack, _Match_head& __head)
	  {
	    __head._M_state = __stack.template _M_top_item<_Saved_state>()._M_state;
	    __stack.template _M_pop<_Saved_state>();
	    _M_dfs<_Dfs_executer>(*this, __head);
	  }

	  void
	  _M_restore_capture(_Dynamic_stack& __stack, _Match_head& __head)
	  {
	    auto& __save = __stack.template _M_top_item<_Saved_capture>();
	    __head._M_captures[__save._M_index] = std::move(__save._M_capture);
	    __stack.template _M_pop<_Saved_capture>();
	  }

	  void
	  _M_restore_dfs_repeat(_Dynamic_stack& __stack, _Match_head& __head)
	  {
	    auto& __save = __stack.template _M_top_item<_Saved_dfs_repeat>();
	    _M_last = __save._M_last;
	    _M_context._M_current = __save._M_current;
	    __head._M_state = __save._M_next;
	    __stack.template _M_pop<_Saved_dfs_repeat>();
	    _M_dfs<_Dfs_executer>(*this, __head);
	  }

	  void
	  _M_restore_dfs_neg_repeat(_Dynamic_stack& __stack, _Match_head& __head)
	  {
	    auto& __save = __stack.template _M_top_item<_Saved_dfs_neg_repeat>();
	    const auto& __current = _M_context._M_current;
	    const auto& __state_id = __save._M_original;
	    if (_M_last.first == __state_id && _M_last.second == __current)
	      {
		__stack.template _M_pop<_Saved_dfs_neg_repeat>();
		return;
	      }
	    _M_push<_Saved_last>(_M_last);
	    _M_last = make_pair(__state_id, __current);
	    __head._M_state = __save._M_next;
	    __stack.template _M_pop<_Saved_dfs_neg_repeat>();
	    _M_dfs<_Dfs_executer>(*this, __head);
	  }

	  void
	  _M_restore_last(_Dynamic_stack& __stack, _Match_head& __head)
	  {
	    _M_last = std::move(__stack.template _M_top_item<_Saved_last>()._M_last);
	    __stack.template _M_pop<_Saved_last>();
	  }

	private:
	  _Context<_Traits> _M_context;
	  std::pair<_StateIdT, _Bi_iter> _M_last;

	  template<typename _Executer>
	    friend void
	    _Regex_scope<_Bi_iter>::_M_dfs(_Executer& __executer, _Match_head& __head);
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
	_M_visited_impl(_StateIdT __state_id, _Match_head& __head)
	{ return _M_is_visited[__state_id]; }

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
	_M_visited_impl(_StateIdT __state_id, _Match_head& __head)
	{
	  if (_M_is_visited[__state_id])
	    if (!_M_leftmost_longest(__head._M_captures, _M_current_positions[__state_id]))
	      return true;
	  return false;
	}

	void
	_M_visit_impl(_StateIdT __state_id, _Match_head& __head)
	{
	  _M_is_visited[__state_id] = true;
	  _M_current_positions[__state_id] = __head._M_captures;
	}

      private:
	std::vector<bool> _M_is_visited;
	std::vector<_Captures> _M_current_positions;
      };

      template<typename _Traits, bool __is_ecma>
	class _Bfs_executer : private std::conditional<__is_ecma, _Bfs_ecma_mixin, _Bfs_posix_mixin>::type
	{
	public:
	  _Context<_Traits>&
	  _M_get_context()
	  { return _M_context; }

	  bool
	  _M_search_from_first(_StateIdT __start, _Captures& __result);

	private:
	  void
	  _M_exec(_Match_head& __head);

	  template<typename _Tp, typename... _Args>
	    void
	    _M_push(_Args&&... __args)
	    { _M_context._M_stack._M_push<_Tp>(_Tp(std::forward<_Args>(__args)...)); }

	  bool
	  _M_visited(_StateIdT __state_id, _Match_head& __head)
	  { return this->_M_visited_impl(__state_id, __head); }

	  void
	  _M_visit(_StateIdT __state_id, _Match_head& __head)
	  { return this->_M_visit_impl(__state_id, __head); }

	  bool
	  _M_handle_repeat(const _State<_Char_type>& __state, _Match_head& __head);

	  bool
	  _M_handle_subexpr_begin(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_subexpr_begin_common(_M_context, __state, __head); }

	  bool
	  _M_handle_subexpr_end(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_subexpr_end_common(_M_context, __state, __head); }

	  bool
	  _M_handle_alternative(const _State<_Char_type>& __state, _Match_head& __head);

	  bool
	  _M_handle_line_begin_assertion(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_line_begin_assertion_common(_M_context, __state, __head); }

	  bool
	  _M_handle_line_end_assertion(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_line_end_assertion_common(_M_context, __state, __head); }

	  bool
	  _M_handle_word_boundary(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_word_boundary_common(_M_context, __state, __head); }

	  bool
	  _M_handle_subexpr_lookahead(const _State<_Char_type>& __state, _Match_head& __head)
	  { return _M_handle_subexpr_lookahead_common<_Bfs_executer>(_M_context, __state, __head); }

	  bool
	  _M_handle_match(const _State<_Char_type>& __state, const _Match_head& __head);

	  bool
	  _M_handle_backref(const _State<_Char_type>& __state, _Match_head& __head)
	  {
	    _GLIBCXX_DEBUG_ASSERT(false);
	    return false;
	  }

	  bool
	  _M_handle_accept(const _State<_Char_type>& __state, _Match_head& __head);

	private:
	  bool
	  _M_search_from_first_impl();

	  void
	  _M_restore(_Saved_state __save, _Match_head& __head)
	  {
	    __head._M_state = __save._M_state;
	    _M_dfs<_Bfs_executer>(*this, __head);
	  }

	  void
	  _M_restore(_Saved_capture __save, _Match_head& __head)
	  { __head._M_captures[__save._M_index] = std::move(__save._M_capture); }

	  _Context<_Traits> _M_context;
	  std::vector<_Match_head> _M_heads;
	  _Captures _M_result;
	  bool _M_found;

	  template<typename _Executer>
	    friend void
	    _Regex_scope<_Bi_iter>::_M_dfs(_Executer& __executer, _Match_head& __head);
	};

      template<typename _Executer, typename _Bp, typename _Alloc>
	static bool
	__match(_Executer& __executer, std::vector<sub_match<_Bp>, _Alloc>& __res);

      template<typename _Executer>
	static bool
	__match_impl(_Executer& __executer, _StateIdT __start, _Captures& __result);

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
