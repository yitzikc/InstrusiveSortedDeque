/*
 * InstrusiveSortedDeque.h
 *
 *  Created on: Dec 4, 2016
 *      Author: yitzikc
 */

#ifndef UTILS_INTRUSIVESORTEDDEQUE_H_
#define UTILS_INTRUSIVESORTEDDEQUE_H_

#include <algorithm>
#include <deque>

#include <boost/iterator/filter_iterator.hpp>

namespace Utils {

// InstrusiveSortedDeque: A deque containing sorted values, which should supply the following types and methods:
// * typedef KeyType
// * KeyType GetKey() const;
// * IsDeleted() const; - indicating that a value should be considered as removed
// * Remove()		    - Designates a value as deleted.

template <typename T>	// TODO: Add other template args allowing the allocator to be customized
class InstrusiveSortedDeque : public std::deque<T> {
private:

	typedef std::deque<T> StdDeque;

	static constexpr bool FilterPredicate(const T& value) { return ! value.IsDeleted(); }


	// A default-constructible type which wraps FilterPredicate
	struct FilterPredicateType {
		inline bool operator() (const T& value)
		{
			return FilterPredicate(value);
		}
	};

public:

	using typename StdDeque::allocator_type;
	using typename StdDeque::size_type;
	using typename StdDeque::pointer;
	using typename StdDeque::const_pointer;
	using typename StdDeque::reference;
	using typename StdDeque::const_reference;

	typedef typename T::KeyType key_type;
	typedef T value_type;

	typedef boost::filter_iterator<FilterPredicateType, typename StdDeque::iterator> iterator;
	typedef boost::filter_iterator<FilterPredicateType, typename StdDeque::const_iterator> const_iterator;
	typedef boost::filter_iterator<FilterPredicateType, typename StdDeque::reverse_iterator> reverse_iterator;
	typedef boost::filter_iterator<FilterPredicateType, typename StdDeque::const_reverse_iterator> const_reverse_iterator;

	InstrusiveSortedDeque( const_iterator first, const_iterator last, const allocator_type& alloc = allocator_type() )
		: StdDeque(first, last, alloc)
		, m_nMarkedAsErased(0)
	{
	}

	InstrusiveSortedDeque( iterator first, iterator last, const allocator_type& alloc = allocator_type() )
		: StdDeque(first, last, alloc)
		, m_nMarkedAsErased(0)
	{
	}

	InstrusiveSortedDeque(const InstrusiveSortedDeque<T>& other)
		: InstrusiveSortedDeque(other.cbegin(), other.cend(), other.get_allocator())
// FIXME: Aliasing the allocator might not be a good idea when we use custom allocators
	{
	}

	template< class InputIt >
	InstrusiveSortedDeque( InputIt first, InputIt last, const allocator_type& alloc = allocator_type() )
		: StdDeque(MakeFilteredIter(this, first), MakeFilteredIter(this, last), alloc)
		, m_nMarkedAsErased(0)
	{
	}

	InstrusiveSortedDeque()
		: StdDeque::deque()
		, m_nMarkedAsErased(0)
	{
	}

	using StdDeque::deque;

	InstrusiveSortedDeque<T>& operator=(const InstrusiveSortedDeque<T>& other)
	{
		assign(other.cbegin(), other.cend());
		return *this;
	}

	InstrusiveSortedDeque<T>& operator=(InstrusiveSortedDeque<T>&& other)
	{
		static_cast<StdDeque*>(this)->operator=(other);
		m_nMarkedAsErased = other.m_nMarkedAsErased;
		return *this;
	}

	// Methods for obtaining forward iterators

	iterator begin()
	{
		ValidateEdge(this->front());
		return MakeFilteredIter(this, StdDeque::begin());
	}

	const_iterator begin() const
	{
		ValidateEdge(this->front());
		return MakeFilteredIter(this, StdDeque::begin());
	}

	const_iterator cbegin() const
	{
		return begin();
	}

	iterator end()
	{
		ValidateEdge(this->back());
		return MakeFilteredIter(this, StdDeque::end());
	}

	const_iterator end() const
	{
		ValidateEdge(this->back());
		return MakeFilteredIter(this, StdDeque::end());
	}

	const_iterator cend() const
	{
		return end();
	}

	// Methods for obtaining reverse iterators

	reverse_iterator rbegin()
	{
		ValidateEdge(this->back());
		return MakeFilteredIter(this, StdDeque::rbegin());
	}

	const_reverse_iterator rbegin() const
	{
		ValidateEdge(this->back());
		return MakeFilteredIter(this, StdDeque::rbegin());
	}

	const_reverse_iterator crbegin() const
	{
		return rbegin();
	}

	reverse_iterator rend()
	{
		ValidateEdge(this->front());
		return MakeFilteredIter(this, StdDeque::rend());
	}

	const_reverse_iterator rend() const
	{
		ValidateEdge(this->front());
		return MakeFilteredIter(this, StdDeque::rend());
	}

	const_reverse_iterator crend() const
	{
		return end();
	}

	size_type size() const
	{
		const size_type baseSize = StdDeque::size();
		if (baseSize <= 1) {
			assert(0 == m_nMarkedAsErased);
			return baseSize;
		}

		const ssize_t netSize = baseSize - m_nMarkedAsErased;
		assert(netSize > 1);
		return netSize;
	}

	const_iterator find(key_type k) const
	{
		typename StdDeque::const_iterator it;
		DoFind(it, StdDeque::cbegin(), StdDeque::cend(), k);
		return MakeFilteredIter(this, std::move(it));
	}

	iterator find(key_type k)
	{
		typename StdDeque::iterator it;
		DoFind(it, StdDeque::begin(), StdDeque::end(), k);
		return MakeFilteredIter(this, std::move(it));
	}

	void erase(iterator& it)
	{
		erase(it.base());
		return;
	}

	// TODO: Also implement the overload using a range.

	bool erase(key_type k)
	{
		typename StdDeque::iterator it;
		if (DoFind(it, StdDeque::begin(), StdDeque::end(), k)) {
			erase(it);
			return true;
		}

		return false;
	}

	void pop_front()
	{
		assert(! this->empty() && ! this->front().IsDeleted());
		StdDeque::pop_front();
		TrimFront();
	}

	void pop_back()
	{
		assert(! this->empty() && ! this->back().IsDeleted());
		StdDeque::pop_back();
		TrimBack();
	}

	void clear()
	{
		StdDeque::clear();
		m_nMarkedAsErased = 0;
		return;
	}

	void assign(const_iterator first, const_iterator last)
	{
		AssignFiltered(first, last);
	}

	void assign(iterator first, iterator last)
	{
		AssignFiltered(first, last);
	}

	template< class InputIt >
	void assign( InputIt first, InputIt last )
	{
		AssignFiltered(MakeFilteredIter(this, first), MakeFilteredIter(this, last));
	}

	// TODO: Provide a more flexible kind of emplace_back which can handle slightly out-of-order orders

	template< typename... Args >
	void emplace_back(Args&&... args)
	{
		const_pointer const prevBack = this->empty() ? nullptr : & (this->back());
		StdDeque::emplace_back(std::forward<Args>(args)...);
		assert(! this->back().IsDeleted());
		assert( (nullptr == prevBack) ||
				((! prevBack->IsDeleted()) && (prevBack->GetKey() < this->back().GetKey())));

		return;
	}

	template< typename... Args >
	void emplace_front(Args&&... args)
	{
		const_pointer const prevFront = this->empty() ? nullptr : & (this->front());
		StdDeque::emplace_front(std::forward<Args>(args)...);
		assert(! this->front().IsDeleted());
		assert( (nullptr == prevFront) ||
				((! prevFront->IsDeleted()) && (this->front().GetKey() < prevFront->GetKey())));

		return;
	}

	// FIXME: Implement resize() to  to maintain the invariants for m_nMarkedAsErased if it shrinks
	// FIXME: Override operator [] and at() with a read-only versions, so that the invariants for m_nMarkedAsErased could be maintained
	// FIXME: Implement the other overloads of assign() to maintain the invariants for m_nMarkedAsErased

private:
	typename StdDeque::size_type m_nMarkedAsErased = 0;

	void TrimFront()
	{
		while (! this->empty() && this->front().IsDeleted()) {
			StdDeque::pop_front();
			--m_nMarkedAsErased;
		}
		return;
	}

	void TrimBack()
	{
		while (! this->empty() && this->back().IsDeleted()) {
			StdDeque::pop_back();
			--m_nMarkedAsErased;
		}
		return;
	}

	template <typename IterType>
	bool DoFind(IterType& result, IterType&& beginIter, IterType&& endIter, key_type k)
	{
		constexpr auto FindComp = [](const T& value, typename T::KeyType k)->bool { return value.GetKey() < k; };

		if (! this->empty() && (k <= this->back().GetKey())) {
			result = std::lower_bound(beginIter, endIter, k, FindComp);
			assert(result != endIter);
			if ((result->GetKey() == k) && (endIter != result)) {
				return true;
			}
		}

		result = this->StdDeque::end();
		return false;
	}

	// Filter iterator wrapping implementation
	template <typename ThisType, typename BaseIterType>
	inline static boost::filter_iterator<FilterPredicateType, BaseIterType> MakeFilteredIter(ThisType thisPtr, BaseIterType&& baseIter)
	{
		return boost::make_filter_iterator<FilterPredicateType>(baseIter, thisPtr->StdDeque::end());
	}

	template< class InputIt >
	void AssignFiltered(InputIt first, InputIt last)
	{
		StdDeque::assign(first, last);
		m_nMarkedAsErased = 0;
	}

	// Validate that a value is a valid fron or back value
	void ValidateEdge(const value_type& v) const
	{
		// Note that if the deque is empty, the reference to v might be garbage.
		assert(this->empty() || ! v.IsDeleted());
	}

	void erase(typename StdDeque::iterator it)
	{
		if (! it->IsDeleted()) {
			it->Remove();
			assert(it->IsDeleted());
			++m_nMarkedAsErased;
			TrimFront();
			TrimBack();
		}
		else {
			assert((StdDeque::size() > 1) && (m_nMarkedAsErased > 0));
			ValidateEdge(this->front());
			ValidateEdge(this->back());
		}

		return;
	}
};

}	// namespace Utils

#endif /* UTILS_INTRUSIVESORTEDDEQUE_H_ */
