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

      bool
      _M_word_boundary() const;

      _Bi_iter                          _M_current;
      _Bi_iter                          _M_begin;
      const _Bi_iter                    _M_end;
      const _NFA<_Traits>&              _M_nfa;
      regex_constants::match_flag_type  _M_match_flags;
      _Search_mode                      _M_search_mode;
    };

  template<typename _Executor>
    class _Executor_mixin
    {
    protected:
      void
      _M_dfs(_StateIdT __state_id);

    private:
      _Executor*
      _M_this()
      { return static_cast<_Executor*>(this); }
    };

  /**
   * @brief Takes a regex and an input string and does the matching.
   *
   * The %_Executor class has two modes: DFS mode and BFS mode, controlled
   * by the template parameter %__dfs_mode.
   */
  template<typename _BiIter, typename _Alloc, typename _TraitsT,
	   bool __dfs_mode>
    class _Executor
    : private _Context<_BiIter, _TraitsT>,
      private _Executor_mixin<_Executor<_BiIter, _Alloc, _TraitsT, __dfs_mode>>
    {
      using __algorithm = integral_constant<bool, __dfs_mode>;
      using __dfs = true_type;
      using __bfs = false_type;
      using _Context_type = _Context<_BiIter, _TraitsT>;
      using _State_type = _State<typename iterator_traits<_BiIter>::value_type>;

    public:
      typedef std::vector<sub_match<_BiIter>, _Alloc>       _ResultsVec;

    public:
      _Executor(_BiIter __begin, _BiIter __end, const _NFA<_TraitsT>& __nfa,
		regex_constants::match_flag_type __flags,
		_Search_mode __search_mode, _ResultsVec& __results)
      : _Context_type(__begin, __end, __nfa, __flags, __search_mode),
      _M_results(__results), _M_last_rep_visit(_S_invalid_state_id, _BiIter()),
      _M_states(this->_M_nfa.size())
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
	{ return _M_match_impl<__search_mode>(this->_M_nfa._M_start()); }

    private:
      template<_Search_mode __search_mode>
	bool
	_M_match_impl(_StateIdT __start);

      bool
      _M_handle_visit(_StateIdT __state_id)
      { return _M_states._M_visited(__state_id); }

      void
      _M_handle_repeat(_StateIdT __state_id);

      void
      _M_handle_subexpr_begin(const _State_type& __state);

      void
      _M_handle_subexpr_end(const _State_type& __state);

      void
      _M_handle_line_begin_assertion(const _State_type& __state);

      void
      _M_handle_line_end_assertion(const _State_type& __state);

      void
      _M_handle_word_boundary(const _State_type& __state);

      void
      _M_handle_subexpr_lookahead(const _State_type& __state);

      void
      _M_handle_match(const _State_type& __state);

      void
      _M_handle_backref(const _State_type& __state);

      void
      _M_handle_accept(const _State_type& __state);

      void
      _M_handle_alternative(const _State_type& __state);

      void
      _M_nonreentrant_repeat(_StateIdT, _StateIdT);

      bool
      _M_main_dispatch(_StateIdT __start, __dfs);

      bool
      _M_main_dispatch(_StateIdT __start, __bfs);

       // Holds additional information used in BFS-mode.
      template<typename _Algorithm, typename _ResultsVec>
	struct _State_info;

      template<typename _ResultsVec>
	struct _State_info<__bfs, _ResultsVec>
	{
	  explicit
	  _State_info(size_t __n) : _M_visited_states(new bool[__n]()) { }

	  bool _M_visited(_StateIdT __i)
	  {
	    if (_M_visited_states[__i])
	      return true;
	    _M_visited_states[__i] = true;
	    return false;
	  }

	  void _M_queue(_StateIdT __i, const _ResultsVec& __res)
	  { _M_match_queue.emplace_back(__i, __res); }

	  // Dummy implementations for BFS mode.
	  _BiIter* _M_get_sol_pos() { return nullptr; }

	  // Saves states that need to be considered for the next character.
	  vector<pair<_StateIdT, _ResultsVec>>	_M_match_queue;
	  // Indicates which states are already visited.
	  unique_ptr<bool[]>			_M_visited_states;
	};

      template<typename _ResultsVec>
	struct _State_info<__dfs, _ResultsVec>
	{
	  explicit
	  _State_info(size_t) { }

	  // Dummy implementations for DFS mode.
	  bool _M_visited(_StateIdT) const { return false; }
	  void _M_queue(_StateIdT, const _ResultsVec&) { }

	  _BiIter* _M_get_sol_pos() { return &_M_sol_pos; }

	  _BiIter   _M_sol_pos;
	};

    public:
      _ResultsVec                                           _M_cur_results;
      _ResultsVec&                                          _M_results;
      pair<_StateIdT, _BiIter>                              _M_last_rep_visit;
      _State_info<__algorithm, _ResultsVec>		    _M_states;
      // Do we have a solution so far?
      bool                                                  _M_has_sol;

      template<typename _Ep>
	friend class _Executor_mixin;
    };

 //@} regex-detail
_GLIBCXX_END_NAMESPACE_VERSION
} // namespace __regex
} // namespace __detail
} // namespace std

#include <bits/regex_executor.tcc>
