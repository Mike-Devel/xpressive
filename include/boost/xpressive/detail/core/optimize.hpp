///////////////////////////////////////////////////////////////////////////////
// optimize.hpp
//
//  Copyright 2004 Eric Niebler. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_XPRESSIVE_DETAIL_CORE_OPTIMIZE_HPP_EAN_10_04_2005
#define BOOST_XPRESSIVE_DETAIL_CORE_OPTIMIZE_HPP_EAN_10_04_2005

#include <string>
#include <utility>
#include <boost/mpl/bool.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/xpressive/detail/core/finder.hpp>
#include <boost/xpressive/detail/core/linker.hpp>
#include <boost/xpressive/detail/core/peeker.hpp>
#include <boost/xpressive/detail/core/regex_impl.hpp>

namespace boost { namespace xpressive { namespace detail
{

///////////////////////////////////////////////////////////////////////////////
// optimize_regex
//
template<typename BidiIter, typename Traits>
intrusive_ptr<finder<BidiIter> > optimize_regex
(
    xpression_peeker<typename iterator_value<BidiIter>::type> const &peeker
  , Traits const &traits
  , mpl::false_
)
{
    if(peeker.line_start())
    {
        return intrusive_ptr<finder<BidiIter> >
        (
            new line_start_finder<BidiIter, Traits>(traits)
        );
    }
    else if(256 != peeker.bitset().count())
    {
        return intrusive_ptr<finder<BidiIter> >
        (
            new hash_peek_finder<BidiIter, Traits>(peeker.bitset())
        );
    }

    return intrusive_ptr<finder<BidiIter> >();
}

///////////////////////////////////////////////////////////////////////////////
// optimize_regex
//
template<typename BidiIter, typename Traits>
intrusive_ptr<finder<BidiIter> > optimize_regex
(
    xpression_peeker<typename iterator_value<BidiIter>::type> const &peeker
  , Traits const &traits
  , mpl::true_
)
{
    typedef typename iterator_value<BidiIter>::type char_type;

    // if we have a leading string literal, initialize a boyer-moore struct with it
    std::pair<std::basic_string<char_type> const *, bool> str = peeker.get_string();
    if(0 != str.first)
    {
        BOOST_ASSERT(1 == peeker.bitset().count());
        return intrusive_ptr<finder<BidiIter> >
        (
            new boyer_moore_finder<BidiIter, Traits>
            (
                str.first->data()
              , str.first->data() + str.first->size()
              , traits
              , str.second
            )
        );
    }

    return optimize_regex<BidiIter>(peeker, traits, mpl::false_());
}

///////////////////////////////////////////////////////////////////////////////
// common_compile
//
template<typename RegEx, typename Traits>
void common_compile
(
    intrusive_ptr<RegEx const> const &regex
  , regex_impl<typename RegEx::iterator_type> &impl
  , Traits const &traits
)
{
    typedef typename RegEx::iterator_type iterator_type;
    typedef typename iterator_value<iterator_type>::type char_type;

    // "link" the regex
    xpression_linker<char_type> linker(traits);
    regex->link(linker);

    // "peek" into the compiled regex to see if there are optimization opportunities
    hash_peek_bitset<char_type> bset;
    xpression_peeker<char_type> peeker(bset, traits);
    regex->peek(peeker);

    // optimization: get the peek chars OR the boyer-moore search string
    impl.finder_ = optimize_regex<iterator_type>(peeker, traits, is_random<iterator_type>());
    impl.xpr_ = regex;
}

}}} // namespace boost::xpressive

#endif
