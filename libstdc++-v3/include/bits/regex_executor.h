// class template regex -*- C++ -*-

// Copyright (C) 2013-2015 Free Software Foundation, Inc.
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
 *  @file bits/regex_executor.h
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{regex}
 */

// FIXME convert comments to doxygen format.

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
namespace __regex
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION
  /**
   * @addtogroup regex-detail
   * @{
   */

  enum class _Search_mode : unsigned char { _Match, _Search };
  enum class _Style : unsigned char { _Ecma, _Posix };

  template<typename _Bi_iter, typename _Traits>
    struct _Context
    {
      using _Char_type = typename iterator_traits<_Bi_iter>::value_type;

      _Context(_Bi_iter __begin, _Bi_iter __end, const _NFA<_Traits>& __nfa,
	       regex_constants::match_flag_type __flags,
	       _Search_mode __search_mode)
      : _M_begin(__begin), _M_end(__end), _M_nfa(__nfa),
      _M_match_flags(__flags), _M_search_mode(__search_mode) { }

      bool
      _M_is_word(_Char_type __ch) const
      {
	static const _Char_type __s[2] = { 'w' };
	return _M_nfa._M_traits.isctype
	  (__ch, _M_nfa._M_traits.lookup_classname(__s, __s+1));
      }

      bool
      _M_at_begin() const
      {
	return _M_current == _M_begin
	  && !(_M_match_flags & (regex_constants::match_not_bol
				 | regex_constants::match_prev_avail));
      }

      bool
      _M_at_end() const
      {
	return _M_current == _M_end
	  && !(_M_match_flags & regex_constants::match_not_eol);
      }

      size_t
      _M_sub_count() const
      { return _M_nfa._M_sub_count(); }

      bool
      _M_word_boundary() const;

      _Bi_iter                          _M_current;
      _Bi_iter                          _M_begin;
      const _Bi_iter                    _M_end;
      const _NFA<_Traits>&              _M_nfa;
      regex_constants::match_flag_type  _M_match_flags;
      _Search_mode                      _M_search_mode;
    };

  template<typename _Bi_iter, typename _Executor>
    class _Executor_mixin
    {
      using _State_type =
	  _State<typename iterator_traits<_Bi_iter>::value_type>;
      using _Submatch = sub_match<_Bi_iter>;

    protected:
      template<_Search_mode __search_mode>
	bool
	_M_match_impl(_StateIdT __start);

      bool
      _M_dfs(_StateIdT __state_id, _Submatch* __captures);

      bool
      _M_handle_line_begin_assertion(const _State_type& __state,
				     _Submatch* __captures);

      bool
      _M_handle_line_end_assertion(const _State_type& __state,
				   _Submatch* __captures);

      bool
      _M_handle_word_boundary(const _State_type& __state,
			      _Submatch* __captures);

      bool
      _M_handle_subexpr_lookahead(const _State_type& __state,
				  _Submatch* __captures);

      bool
      _M_handle_alternative(const _State_type& __state, _Submatch* __captures);

    private:
      _Executor*
      _M_this()
      { return static_cast<_Executor*>(this); }
    };

  template<typename _Bi_iter>
    bool
    __leftmost_longest(const sub_match<_Bi_iter>* __lhs,
		       const sub_match<_Bi_iter>* __rhs, size_t __size);

  template<typename _Bi_iter, _Style __style>
    class _Dfs_mixin;

  template<typename _Bi_iter>
    class _Dfs_mixin<_Bi_iter, _Style::_Ecma>
    {
      using _Submatch = sub_match<_Bi_iter>;

    public:
      _Dfs_mixin(_Submatch* __results, size_t __sub_count)
      : _M_results(__results) { }

      _Submatch*
      _M_current_results()
      { return _M_results; }

      void
      _M_update() { }

    private:
      _Submatch* _M_results;
    };

  template<typename _Bi_iter>
    class _Dfs_mixin<_Bi_iter, _Style::_Posix>
    {
      using _Submatch = sub_match<_Bi_iter>;

    public:
      _Dfs_mixin(_Submatch* __results, size_t __sub_count)
      : _M_results(__results), _M_current_res(__sub_count) { }

      _Submatch*
      _M_current_results()
      { return _M_current_res.data(); }

      void
      _M_update()
      {
	if (__leftmost_longest(_M_current_res.data(), _M_results,
			       _M_current_res.size()))
	  std::copy(_M_current_res.begin(), _M_current_res.end(), _M_results);
      }

    private:
      _Submatch*		_M_results;
      vector<_Submatch>	_M_current_res;
    };

  /**
   * @brief Takes a regex and an input string and applies backtracking
   * algorithm.
   */
  template<typename _BiIter, typename _TraitsT, _Style __style>
    class _Dfs_executor
    : private _Context<_BiIter, _TraitsT>,
      private _Executor_mixin<_BiIter, _Dfs_executor<_BiIter, _TraitsT,
						     __style>>,
      private _Dfs_mixin<_BiIter, __style>
    {
      using _Context_type = _Context<_BiIter, _TraitsT>;
      using _State_type =
	  _State<typename iterator_traits<_BiIter>::value_type>;
      using _Submatch = sub_match<_BiIter>;

    public:
      _Dfs_executor(_BiIter __begin, _BiIter __end, const _NFA<_TraitsT>& __nfa,
		    regex_constants::match_flag_type __flags,
		    _Search_mode __search_mode, _Submatch* __results)
      : _Context_type(__begin, __end, __nfa, __flags, __search_mode),
      _Dfs_mixin<_BiIter, __style>(__results, this->_M_sub_count()),
      _M_last_rep_visit(_S_invalid_state_id, _BiIter())
      { }

      // __search_mode should be the same as this->_M_search_mode. It's
      // slightly faster to let caller specify __search_mode again
      // as a template argument.
      // Still, in other places we prefer using the dynamic
      // this->_M_search_mode,
      // because it doesn't deserve being passed around as a template
      // argument/function argument, since it's used in only a few places.
      template<_Search_mode __search_mode>
	bool
	_M_match()
	{
	  return this->template _M_match_impl<__search_mode>(
	      this->_M_nfa._M_start());
	}

    private:
      bool
      _M_search_from_first(_StateIdT __start);

      constexpr bool
      _M_is_ecma() const
      { return __style == _Style::_Ecma; }

      bool
      _M_handle_visit(_StateIdT __state_id)
      { return false; }

      bool
      _M_handle_subexpr_begin(const _State_type& __state,
			      _Submatch* __captures);

      bool
      _M_handle_subexpr_end(const _State_type& __state,
			    _Submatch* __captures);

      bool
      _M_handle_repeat(_StateIdT __state_id, _Submatch* __captures);

      bool
      _M_handle_match(const _State_type& __state, _Submatch* __captures);

      bool
      _M_handle_backref(const _State_type& __state, _Submatch* __captures);

      bool
      _M_handle_accept(const _State_type& __state, _Submatch* __captures);

      bool
      _M_nonreentrant_repeat(_StateIdT, _StateIdT, _Submatch* __captures);

    public:
      pair<_StateIdT, _BiIter>		_M_last_rep_visit;

      template<typename _Bp, typename _Ep>
	friend class _Executor_mixin;
    };

  /**
   * @brief Takes a regex and an input string and applies Thompson NFA
   * algorithm.
   */
  template<typename _Bi_iter, typename _TraitsT>
    class _Bfs_executor
    : private _Context<_Bi_iter, _TraitsT>,
      private _Executor_mixin<_Bi_iter, _Bfs_executor<_Bi_iter, _TraitsT>>
    {
      using _Context_type = _Context<_Bi_iter, _TraitsT>;
      using _State_type =
	_State<typename iterator_traits<_Bi_iter>::value_type>;
      using _Submatch = sub_match<_Bi_iter>;

    public:
      _Bfs_executor(_Bi_iter __begin, _Bi_iter __end,
		    const _NFA<_TraitsT>& __nfa,
		    regex_constants::match_flag_type __flags,
		    _Search_mode __search_mode, _Submatch* __results)
      : _Context_type(__begin, __end, __nfa, __flags, __search_mode),
      _M_results(__results), _M_visited_states(new bool[this->_M_nfa.size()])
      { }

      // __search_mode should be the same as this->_M_search_mode. It's
      // slightly faster to let caller specify __search_mode again
      // as a template argument.
      // Still, in other places we prefer using the dynamic
      // this->_M_search_mode,
      // because it doesn't deserve being passed around as a template
      // argument/function argument, since it's used in only a few places.
      template<_Search_mode __search_mode>
	bool
	_M_match()
	{
	  return this->template _M_match_impl<__search_mode>(
	      this->_M_nfa._M_start());
	}

    private:
      bool
      _M_search_from_first(_StateIdT __start);

      bool
      _M_is_ecma() const
      { return this->_M_nfa._M_options() & regex_constants::ECMAScript; }

      bool
      _M_handle_visit(_StateIdT __state_id)
      {
	// TODO: This is wrong for POSIX. If we already visited the state,
	// we may want to again do _M_dfs in this state, since POSIX uses
	// a leftmost longest algorithm for picking the final result.
	if (_M_visited_states[__state_id])
	  return true;
	_M_visited_states[__state_id] = true;
	return false;
      }

      bool
      _M_handle_subexpr_begin(const _State_type& __state,
			      _Submatch* __captures);

      bool
      _M_handle_subexpr_end(const _State_type& __state,
			    _Submatch* __captures);

      bool
      _M_handle_repeat(_StateIdT __state_id, _Submatch* __captures);

      bool
      _M_handle_match(const _State_type& __state, _Submatch* __captures);

      bool
      _M_handle_backref(const _State_type& __state, _Submatch* __captures)
      {
	__glibcxx_assert(false);
	return false;
      }

      bool
      _M_handle_accept(const _State_type& __state, _Submatch* __captures);

      void _M_queue(_StateIdT __i, const _Submatch* __captures, size_t __size)
      {
	_M_match_queue.emplace_back(
	  __i, vector<_Submatch>{__captures, __captures + __size});
      }

    public:
      _Submatch*      				_M_results;
      // Saves states that need to be considered for the next character.
      vector<pair<_StateIdT, vector<_Submatch>>>	_M_match_queue;
      // Indicates which states are already visited.
      unique_ptr<bool[]>			_M_visited_states;

      template<typename _Bp, typename _Ep>
	friend class _Executor_mixin;
    };

 //@} regex-detail
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace __regex
} // namespace __detail
} // namespace std

#include <bits/regex_executor.tcc>
