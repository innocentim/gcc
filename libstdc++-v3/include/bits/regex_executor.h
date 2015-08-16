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

  template<typename _Nfa_type, typename _Bi_iter>
    struct _Context
    {
      using _Char_type = typename iterator_traits<_Bi_iter>::value_type;
      using _State_type = typename _Nfa_type::_StateT;

      _Context(_Bi_iter __begin, _Bi_iter __end, const _Nfa_type& __nfa,
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

      _Bi_iter				_M_current;
      _Bi_iter				_M_begin;
      const _Bi_iter			_M_end;
      const _Nfa_type&			_M_nfa;
      regex_constants::match_flag_type	_M_match_flags;
      _Search_mode			_M_search_mode;
    };

  template<typename _Nfa_type, typename _Bi_iter, typename _Executor>
    class _Executor_mixin
    {
      using _State_type = typename _Nfa_type::_StateT;
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

    protected:
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

    protected:
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
  template<typename _Nfa_type, typename _BiIter, _Style __style>
    class _Dfs_executor
    : private _Context<_Nfa_type, _BiIter>,
      private _Executor_mixin<_Nfa_type, _BiIter,
			      _Dfs_executor<_Nfa_type, _BiIter, __style>>,
      private _Dfs_mixin<_BiIter, __style>
    {
      using _Context_type = _Context<_Nfa_type, _BiIter>;
      using _State_type = typename _Nfa_type::_StateT;
      using _Submatch = sub_match<_BiIter>;

    public:
      _Dfs_executor(_BiIter __begin, _BiIter __end, const _Nfa_type& __nfa,
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
      _M_handle_visit(_StateIdT __state_id, _Submatch* __results)
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
      _M_handle_repeated_match(const _State_type&, _Submatch*);

      bool
      _M_handle_match(_StateIdT __state_id, _Submatch* __captures);

      bool
      _M_handle_backref(const _State_type& __state, _Submatch* __captures);

      bool
      _M_handle_accept(const _State_type& __state, _Submatch* __captures);

      bool
      _M_nonreentrant_repeat(_StateIdT, _StateIdT, _Submatch* __captures);

    private:
      pair<_StateIdT, _BiIter>		_M_last_rep_visit;

      template<typename, typename, typename>
	friend class _Executor_mixin;
    };

  template<typename _Nfa_type, typename _Bi_iter, _Style __style>
    class _Bfs_executor;

  template<typename _Executor_type>
    class _Bfs_mixin;

  template<typename _Nfa_type, typename _Bi_iter>
    class _Bfs_mixin<_Bfs_executor<_Nfa_type, _Bi_iter, _Style::_Ecma>>
    {
      using _Submatch = sub_match<_Bi_iter>;
      using _State_type = typename _Nfa_type::_StateT;

    protected:
      _Bfs_mixin(size_t __size) : _M_visited_states(__size, false) { }

      bool
      _M_handle_visit(_StateIdT __state_id, _Submatch* __results)
      {
	if (_M_visited_states[__state_id])
	  return true;
	_M_visited_states[__state_id] = true;
	return false;
      }

      bool
      _M_handle_match(_StateIdT __state_id, _Submatch* __captures)
      {
	const auto& __state = _M_this()->_M_nfa[__state_id];
	if (_M_this()->_M_current != _M_this()->_M_end)
	  if (__state._M_matches(*_M_this()->_M_current))
	    _M_this()->_M_queue(__state._M_next, __captures);
	return false;
      }

      void
      _M_clear()
      { std::fill(_M_visited_states.begin(), _M_visited_states.end(), false); }

    private:
      _Bfs_executor<_Nfa_type, _Bi_iter, _Style::_Ecma>*
      _M_this()
      {
	return static_cast<
	    _Bfs_executor<_Nfa_type, _Bi_iter, _Style::_Ecma>*>(this);
      }

      // Indicates which states are already visited.
      vector<char> _M_visited_states;
    };

  template<typename _Nfa_type, typename _Bi_iter>
    class _Bfs_mixin<_Bfs_executor<_Nfa_type, _Bi_iter, _Style::_Posix>>
    {
      using _Submatch = sub_match<_Bi_iter>;
      using _State_type = typename _Nfa_type::_StateT;

    protected:
      _Bfs_mixin(size_t __size) : _M_visited_states(__size, nullptr) { }

      bool
      _M_handle_visit(_StateIdT __state_id, _Submatch* __results)
      {
	auto& __visited_state = _M_this()->_M_visited_states[__state_id];
	if (__visited_state
	    && !_M_leftmost_longest(__results, __visited_state,
				    _M_this()->_M_sub_count()))
	  return true;
	// We store different content in _M_visited_states for _S_opcode_match.
	// Let _M_handle_match handle it.
	if (_M_this()->_M_nfa[__state_id]._M_opcode() != _S_opcode_match)
	  __visited_state = __results;
	return false;
      }

      bool
      _M_handle_match(_StateIdT __state_id, _Submatch* __captures)
      {
	// For a state who's opcode is not _S_opcode_match, _M_visited_states
	// stores the captures address from __old_queue, just in case other
	// matching process wants to compete with it (so they can do
	// leftmost lonest competing);
	//
	// For a state who's opcode is _S_opcode_match, _M_visited_states
	// stores the captures address from _M_match_queue. When current state
	// wins, in theory we should delete the loser from _M_visited_states
	// and insert the new winner. To make it fast, we directly copy
	// winner's content to the loser's storage.
	//
	// If this state isn't in _M_match_queue, allocate a new one.
	const auto& __state = _M_this()->_M_nfa[__state_id];
	if (_M_this()->_M_current != _M_this()->_M_end)
	  if (__state._M_matches(*_M_this()->_M_current))
	    {
	      auto& __visited_state = _M_this()->_M_visited_states[__state_id];
	      if (__visited_state)
		{
		  std::copy(__captures, __captures + _M_this()->_M_sub_count(),
			    __visited_state);
		}
	      else
		{
		  __visited_state =
		    _M_this()->_M_queue(__state._M_next, __captures);
		}
	    }
	return false;
      }

      void
      _M_clear()
      {
	std::fill(_M_visited_states.begin(), _M_visited_states.end(), nullptr);
      }

    private:
      bool
      _M_half_matched(_Bi_iter __position)
      {
	return _M_this()->_M_begin <= __position
	    && __position < _M_this()->_M_end;
      }

      bool
      _M_leftmost_longest(const _Submatch* __lhs, const _Submatch* __rhs,
			  size_t __size);

      _Bfs_executor<_Nfa_type, _Bi_iter, _Style::_Posix>*
      _M_this()
      {
	return static_cast<
	    _Bfs_executor<_Nfa_type, _Bi_iter, _Style::_Posix>*>(this);
      }

      vector<_Submatch*> _M_visited_states;
    };

  /**
   * @brief Takes a regex and an input string and applies Thompson NFA
   * algorithm.
   */
  template<typename _Nfa_type, typename _Bi_iter, _Style __style>
    class _Bfs_executor
    : private _Context<_Nfa_type, _Bi_iter>,
      private _Executor_mixin<_Nfa_type, _Bi_iter,
			      _Bfs_executor<_Nfa_type, _Bi_iter, __style>>,
      private _Bfs_mixin<_Bfs_executor<_Nfa_type, _Bi_iter, __style>>
    {
      using _Context_type = _Context<_Nfa_type, _Bi_iter>;
      using _State_type = typename _Nfa_type::_StateT;
      using _Submatch = sub_match<_Bi_iter>;

    public:
      _Bfs_executor(_Bi_iter __begin, _Bi_iter __end, const _Nfa_type& __nfa,
		    regex_constants::match_flag_type __flags,
		    _Search_mode __search_mode, _Submatch* __results)
      : _Context_type(__begin, __end, __nfa, __flags, __search_mode),
      _Bfs_mixin<_Bfs_executor>(this->_M_nfa.size()), _M_results(__results)
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
      _M_handle_subexpr_begin(const _State_type& __state,
			      _Submatch* __captures);

      bool
      _M_handle_subexpr_end(const _State_type& __state,
			    _Submatch* __captures);

      bool
      _M_handle_repeat(_StateIdT __state_id, _Submatch* __captures);

      bool
      _M_handle_repeated_match(const _State_type&, _Submatch*)
      {
	__glibcxx_assert(false);
	return false;
      }

      bool
      _M_handle_backref(const _State_type& __state, _Submatch* __captures)
      {
	__glibcxx_assert(false);
	return false;
      }

      bool
      _M_handle_accept(const _State_type& __state, _Submatch* __captures);

      _Submatch* _M_queue(_StateIdT __i, const _Submatch* __captures)
      {
	_M_match_queue.emplace_back(
	  __i, vector<_Submatch>{__captures,
				 __captures + this->_M_sub_count()});
	return _M_match_queue.back().second.data();
      }

    private:
      _Submatch*      				_M_results;
      // Saves states that need to be considered for the next character.
      vector<pair<_StateIdT, vector<_Submatch>>>	_M_match_queue;

      template<typename, typename, typename>
	friend class _Executor_mixin;
      template<typename>
	friend class _Bfs_mixin;
    };

 //@} regex-detail
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace __regex
} // namespace __detail
} // namespace std

#include <bits/regex_executor.tcc>
