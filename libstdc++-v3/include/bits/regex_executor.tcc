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
 *  @file bits/regex_executor.tcc
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{regex}
 */

namespace std _GLIBCXX_VISIBILITY(default)
{
namespace __detail
{
namespace __regex
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  // Return whether now is at some word boundary.
  template<typename _Bi_iter, typename _Traits>
    inline bool _Context<_Bi_iter, _Traits>::
    _M_word_boundary() const
    {
      bool __left_is_word = false;
      if (_M_current != _M_begin
	  || (_M_match_flags & regex_constants::match_prev_avail))
	{
	  auto __prev = _M_current;
	  if (_M_is_word(*std::prev(__prev)))
	    __left_is_word = true;
	}
      bool __right_is_word =
        _M_current != _M_end && _M_is_word(*_M_current);

      if (__left_is_word == __right_is_word)
	return false;
      if (__left_is_word && !(_M_match_flags & regex_constants::match_not_eow))
	return true;
      if (__right_is_word && !(_M_match_flags & regex_constants::match_not_bow))
	return true;
      return false;
    }

  template<typename _Bi_iter, typename _Executor_type>
  template<_Search_mode __search_mode>
    bool _Executor_mixin<_Bi_iter, _Executor_type>::
    _M_match_impl(_StateIdT __start)
    {
      if (__search_mode == _Search_mode::_Match)
	{
	  _M_this()->_M_current = _M_this()->_M_begin;
	  return _M_this()->_M_search_from_first(__start);
	}

      _M_this()->_M_current = _M_this()->_M_begin;
      if (_M_this()->_M_search_from_first(__start))
	return true;
      if (_M_this()->_M_match_flags & regex_constants::match_continuous)
	return false;
      _M_this()->_M_match_flags |= regex_constants::match_prev_avail;
      while (_M_this()->_M_begin != _M_this()->_M_end)
	{
	  ++_M_this()->_M_begin;
	  _M_this()->_M_current = _M_this()->_M_begin;
	  if (_M_this()->_M_search_from_first(__start))
	    return true;
	}
      return false;
    }

  template<typename _Bi_iter, typename _Executor_type>
    void _Executor_mixin<_Bi_iter, _Executor_type>::
    _M_dfs(_StateIdT __state_id, _Head_type& __head)
    {
      if (_M_this()->_M_handle_visit(__state_id))
	return;

      const auto& __state = _M_this()->_M_nfa[__state_id];
      switch (__state._M_opcode())
	{
	  case _S_opcode_repeat:
	    return _M_this()->_M_handle_repeat(__state_id, __head);
	  case _S_opcode_subexpr_begin:
	    return _M_this()->_M_handle_subexpr_begin(__state, __head);
	  case _S_opcode_subexpr_end:
	    return _M_this()->_M_handle_subexpr_end(__state, __head);
	  case _S_opcode_line_begin_assertion:
	    return _M_this()->_M_handle_line_begin_assertion(__state, __head);
	  case _S_opcode_line_end_assertion:
	    return _M_this()->_M_handle_line_end_assertion(__state, __head);
	  case _S_opcode_word_boundary:
	    return _M_this()->_M_handle_word_boundary(__state, __head);
	  case _S_opcode_subexpr_lookahead:
	    return _M_this()->_M_handle_subexpr_lookahead(__state, __head);
	  case _S_opcode_match:
	    return _M_this()->_M_handle_match(__state, __head);
	  case _S_opcode_backref:
	    return _M_this()->_M_handle_backref(__state, __head);
	  case _S_opcode_accept:
	    return _M_this()->_M_handle_accept(__state, __head);
	  case _S_opcode_alternative:
	    return _M_this()->_M_handle_alternative(__state, __head);
	  case _S_opcode_unknown:
	  case _S_opcode_dummy:
	    __glibcxx_assert(false);
	}
    }

  template<typename _Bi_iter, typename _Executor_type>
    void _Executor_mixin<_Bi_iter, _Executor_type>::
    _M_handle_subexpr_begin(const _State_type& __state, _Head_type& __head)
    {
      auto& __res = __head._M_captures[__state._M_subexpr];
      auto __back = __res.first;
      __res.first = _M_this()->_M_current;
      this->_M_dfs(__state._M_next, __head);
      __res.first = __back;
    }

  template<typename _Bi_iter, typename _Executor_type>
    void _Executor_mixin<_Bi_iter, _Executor_type>::
    _M_handle_subexpr_end(const _State_type& __state, _Head_type& __head)
    {
      auto& __res = __head._M_captures[__state._M_subexpr];
      auto __back = __res;
      __res.second = _M_this()->_M_current;
      __res.matched = true;
      this->_M_dfs(__state._M_next, __head);
      __res = __back;
    }

  template<typename _Bi_iter, typename _Executor_type>
    void _Executor_mixin<_Bi_iter, _Executor_type>::
    _M_handle_line_begin_assertion(const _State_type& __state,
				   _Head_type& __head)
    {
      if (_M_this()->_M_at_begin())
	this->_M_dfs(__state._M_next, __head);
    }

  template<typename _Bi_iter, typename _Executor_type>
    void _Executor_mixin<_Bi_iter, _Executor_type>::
    _M_handle_line_end_assertion(const _State_type& __state, _Head_type& __head)
    {
      if (_M_this()->_M_at_end())
	this->_M_dfs(__state._M_next, __head);
    }

  template<typename _Bi_iter, typename _Executor_type>
    void _Executor_mixin<_Bi_iter, _Executor_type>::
    _M_handle_word_boundary(const _State_type& __state, _Head_type& __head)
    {
      if (_M_this()->_M_word_boundary() == !__state._M_neg)
	this->_M_dfs(__state._M_next, __head);
    }

  template<typename _Bi_iter, typename _Executor_type>
    void _Executor_mixin<_Bi_iter, _Executor_type>::
    _M_handle_subexpr_lookahead(const _State_type& __state, _Head_type& __head)
    {
      // Return whether now match the given sub-NFA.
      const auto __lookahead = [this](_StateIdT __next, _Head_type& __head)
      {
	vector<sub_match<_Bi_iter>> __what(__head._M_captures.size());
	_Executor_type __sub(
	  _M_this()->_M_current, _M_this()->_M_end, _M_this()->_M_nfa,
	  _M_this()->_M_match_flags | regex_constants::match_continuous,
	  _Search_mode::_Search, __what.data());
	if (__sub.template _M_match_impl<_Search_mode::_Search>(__next))
	  {
	    for (size_t __i = 0; __i < __what.size(); __i++)
	      if (__what[__i].matched)
		__head._M_captures[__i] = __what[__i];
	    return true;
	  }
	return false;
      };

      // Here __state._M_alt offers a single start node for a sub-NFA.
      // We recursively invoke our algorithm to match the sub-NFA.
      if (__lookahead(__state._M_alt, __head) == !__state._M_neg)
	this->_M_dfs(__state._M_next, __head);
    }

  template<typename _Bi_iter, typename _Executor_type>
    void _Executor_mixin<_Bi_iter, _Executor_type>::
    _M_handle_alternative(const _State_type& __state, _Head_type& __head)
    {
      if (_M_this()->_M_nfa._M_options() & regex_constants::ECMAScript)
	{
	  // TODO: Fix BFS support. It is wrong.
	  this->_M_dfs(__state._M_alt, __head);
	  // Pick lhs if it matches. Only try rhs if it doesn't.
	  if (!__head._M_has_sol)
	    this->_M_dfs(__state._M_next, __head);
	}
      else
	{
	  // Try both and compare the result.
	  // See "case _S_opcode_accept:" handling above.
	  this->_M_dfs(__state._M_alt, __head);
	  this->_M_dfs(__state._M_next, __head);
	}
    }

  // DFS mode:
  //
  // It applies a Depth-First-Search (aka backtracking) on given NFA and input
  // string.
  // At the very beginning the executor stands in the start state, then it
  // tries every possible state transition in current state recursively. Some
  // state transitions consume input string, say, a single-char-matcher or a
  // back-reference matcher; some don't, like assertion or other anchor nodes.
  // When the input is exhausted and/or the current state is an accepting
  // state, the whole executor returns true.
  //
  // TODO: This approach is exponentially slow for certain input.
  //       Try to compile the NFA to a DFA.
  //
  // Time complexity: \Omega(match_length), O(2^(_M_nfa.size()))
  // Space complexity: \theta(match_results.size() + match_length)
  //
  template<typename _BiIter, typename _TraitsT>
    bool _Dfs_executor<_BiIter, _TraitsT>::
    _M_search_from_first(_StateIdT __start)
    {
      _M_head._M_has_sol = false;
      this->_M_dfs(__start, _M_head);
      return _M_head._M_has_sol;
    }

  // ECMAScript 262 [21.2.2.5.1] Note 4:
  // ...once the minimum number of repetitions has been satisfied, any more
  // expansions of Atom that match the empty character sequence are not
  // considered for further repetitions.
  //
  // POSIX doesn't specify this, so let's keep them consistent.
  template<typename _BiIter, typename _TraitsT>
    void _Dfs_executor<_BiIter, _TraitsT>::
    _M_nonreentrant_repeat(_StateIdT __i, _StateIdT __alt, _Head_type& __head)
    {
      auto __back = _M_last_rep_visit;
      _M_last_rep_visit = make_pair(__i, this->_M_current);
      this->_M_dfs(__alt, __head);
      _M_last_rep_visit = std::move(__back);
    };

  template<typename _BiIter, typename _TraitsT>
    void _Dfs_executor<_BiIter, _TraitsT>::
    _M_handle_repeat(_StateIdT __state_id, _Head_type& __head)
    {
      // The most recent repeated state visit is the same, and this->_M_current
      // doesn't change since then. Shouldn't continue dead looping.
      if (_M_last_rep_visit.first == __state_id
	  && _M_last_rep_visit.second == this->_M_current)
	return;
      const auto& __state = this->_M_nfa[__state_id];
      // Greedy.
      if (!__state._M_neg)
	{
	  _M_nonreentrant_repeat(__state_id, __state._M_alt, __head);
	  // If it's DFS executor and already accepted, we're done.
	  if (!__head._M_has_sol || !_M_is_ecma())
	    this->_M_dfs(__state._M_next, __head);
	}
      else // Non-greedy mode
	{
	  __glibcxx_assert(_M_is_ecma());
	  // vice-versa.
	  this->_M_dfs(__state._M_next, __head);
	  if (!__head._M_has_sol)
	    _M_nonreentrant_repeat(__state_id, __state._M_alt, __head);
	}
    }

  template<typename _BiIter, typename _TraitsT>
    void _Dfs_executor<_BiIter, _TraitsT>::
    _M_handle_match(const _State_type& __state, _Head_type& __head)
    {
      if (this->_M_current == this->_M_end)
	return;
      if (__state._M_matches(*this->_M_current))
	{
	  ++this->_M_current;
	  this->_M_dfs(__state._M_next, __head);
	  --this->_M_current;
	}
    }

  template<typename _BiIter, typename _TraitsT>
    void _Dfs_executor<_BiIter, _TraitsT>::
    _M_handle_backref(const _State_type& __state, _Head_type& __head)
    {
      auto& __submatch = __head._M_captures[__state._M_backref_index];
      if (__submatch.matched)
	{
	  auto __last = this->_M_current;
	  for (auto __tmp = __submatch.first;
	       __last != this->_M_end && __tmp != __submatch.second;
	       ++__tmp)
	    ++__last;
	  if (this->_M_nfa._M_options() & regex_constants::collate)
	    {
	      const auto& __traits = this->_M_nfa._M_traits;
	      if (__traits.transform(__submatch.first, __submatch.second)
		  != __traits.transform(this->_M_current, __last))
		return;
	    }
	  else
	    {
	      if (!std::equal(__submatch.first, __submatch.second,
			      this->_M_current))
		return;
	    }
	  auto __back = this->_M_current;
	  this->_M_current = __last;
	  this->_M_dfs(__state._M_next, __head);
	  this->_M_current = std::move(__back);
	}
      else
	{
	  // ECMAScript [21.2.2.9] Note:
	  // If the regular expression has n or more capturing parentheses
	  // but the nth one is undefined because it has not captured
	  // anything, then the backreference always succeeds.
	  this->_M_dfs(__state._M_next, __head);
	}
    }

  template<typename _BiIter, typename _TraitsT>
    void _Dfs_executor<_BiIter, _TraitsT>::
    _M_handle_accept(const _State_type& __state, _Head_type& __head)
    {
      if (!__head._M_has_sol)
	{
	  if (this->_M_search_mode == _Search_mode::_Match)
	    __head._M_has_sol = this->_M_current == this->_M_end;
	  else
	    __head._M_has_sol = true;
	  if (this->_M_current == this->_M_begin
	      && (this->_M_match_flags & regex_constants::match_not_null))
	    __head._M_has_sol = false;
	}
      if (__head._M_has_sol)
	{
	  if (_M_is_ecma()
	      || _M_leftmost_longest(__head._M_captures, _M_results))
	    {
	      std::copy(__head._M_captures.begin(),
			__head._M_captures.end(), _M_results);
	    }
	}
    }

  template<typename _BiIter, typename _TraitsT>
    bool _Dfs_executor<_BiIter, _TraitsT>::
    _M_leftmost_longest(const vector<sub_match<_BiIter>>& __current_match,
			const sub_match<_BiIter>* __rhs)
    {
      for (const auto& __lhs : __current_match)
	{
	  if (!__lhs.matched)
	    return false;
	  if (!__rhs->matched)
	    return true;
	  if (__lhs.first < __rhs->first)
	    return true;
	  if (__lhs.first > __rhs->first)
	    return false;
	  if (__lhs.second > __rhs->second)
	    return true;
	  if (__lhs.second < __rhs->second)
	    return false;
	  __rhs++;
	}
      return false;
    }

  // ------------------------------------------------------------
  //
  // BFS mode:
  //
  // Russ Cox's article (http://swtch.com/~rsc/regexp/regexp1.html)
  // explained this algorithm clearly.
  //
  // It first computes epsilon closure (states that can be achieved without
  // consuming characters) for every state that's still matching,
  // using the same DFS algorithm, but doesn't re-enter states (using
  // _M_states._M_visited to check), nor follow _S_opcode_match.
  //
  // Then apply DFS using every _S_opcode_match (in _M_states._M_match_queue)
  // as the start state.
  //
  // It significantly reduces potential duplicate states, so has a better
  // upper bound; but it requires more overhead.
  //
  // Time complexity: \Omega(match_length * match_results.size())
  //                  O(match_length * _M_nfa.size() * match_results.size())
  // Space complexity: \Omega(_M_nfa.size() + match_results.size())
  //                   O(_M_nfa.size() * match_results.size())
  template<typename _BiIter, typename _TraitsT>
    bool _Bfs_executor<_BiIter, _TraitsT>::
    _M_search_from_first(_StateIdT __start)
    {
      _M_states._M_queue(__start, _M_head._M_captures);
      bool __ret = false;
      while (1)
	{
	  _M_head._M_has_sol = false;
	  if (_M_states._M_match_queue.empty())
	    break;
	  std::fill_n(_M_states._M_visited_states.get(),
		      this->_M_nfa.size(), false);
	  auto __old_queue = std::move(_M_states._M_match_queue);
	  for (auto& __task : __old_queue)
	    {
	      _M_head._M_captures = std::move(__task.second);
	      this->_M_dfs(__task.first, _M_head);
	    }
	  if (this->_M_search_mode == _Search_mode::_Search)
	    __ret |= _M_head._M_has_sol;
	  if (this->_M_current == this->_M_end)
	    break;
	  ++this->_M_current;
	}
      if (this->_M_search_mode == _Search_mode::_Match)
	__ret = _M_head._M_has_sol;
      _M_states._M_match_queue.clear();
      return __ret;
    }

  template<typename _BiIter, typename _TraitsT>
    void _Bfs_executor<_BiIter, _TraitsT>::
    _M_handle_repeat(_StateIdT __state_id, _Head_type& __head)
    {
      const auto& __state = this->_M_nfa[__state_id];
      // Greedy.
      if (!__state._M_neg)
	{
	  this->_M_dfs(__state._M_alt, __head);
	  this->_M_dfs(__state._M_next, __head);
	}
      else // Non-greedy mode
	{
	  // DON'T attempt anything, because there's already another
	  // state with higher priority accepted. This state cannot
	  // be better by attempting its next node.
	  if (!__head._M_has_sol)
	    {
	      this->_M_dfs(__state._M_next, __head);
	      // DON'T attempt anything if it's already accepted. An
	      // accepted state *must* be better than a solution that
	      // matches a non-greedy quantifier one more time.
	      if (!__head._M_has_sol)
		this->_M_dfs(__state._M_alt, __head);
	    }
	}
    }

  template<typename _BiIter, typename _TraitsT>
    void _Bfs_executor<_BiIter, _TraitsT>::
    _M_handle_match(const _State_type& __state, _Head_type& __head)
    {
      if (this->_M_current == this->_M_end)
	return;
      if (__state._M_matches(*this->_M_current))
	_M_states._M_queue(__state._M_next, __head._M_captures);
    }

  template<typename _BiIter, typename _TraitsT>
    void _Bfs_executor<_BiIter, _TraitsT>::
    _M_handle_accept(const _State_type& __state, _Head_type& __head)
    {
      if (this->_M_current == this->_M_begin
	  && (this->_M_match_flags & regex_constants::match_not_null))
	return;
      if (this->_M_search_mode == _Search_mode::_Search
	  || this->_M_current == this->_M_end)
	if (!__head._M_has_sol)
	  {
	    __head._M_has_sol = true;
	    std::copy(__head._M_captures.begin(),
		      __head._M_captures.end(), _M_results);
	  }
    }

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace __regex
} // namespace __detail
} // namespace
