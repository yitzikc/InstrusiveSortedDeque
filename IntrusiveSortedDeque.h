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

// InstrusiveSortedDeque: A deque containing sorted values, which should be default constructible and supply the following types and methods:
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

	// A user-supplied key type
	typedef typename T::KeyType key_type;
	typedef T value_type;

	// A key-type supporting quick access. Essentially a thin wrapper around indexes of the underlying deque class
	class quick_key_type {
		int m_index;

		enum { INVALID_INDEX = -1, MIN_VALID_INDEX = 0 };

		friend class InstrusiveSortedDeque;

		explicit constexpr quick_key_type(int index = INVALID_INDEX)
		{
			m_index = index;
		}

	public:
		constexpr quick_key_type(const quick_key_type&) = default;
		constexpr quick_key_type& operator=(const quick_key_type&) = default;
		constexpr bool is_valid() const { return m_index >= MIN_VALID_INDEX; }
		constexpr bool is_front() const { return m_index == MIN_VALID_INDEX; }
		constexpr bool operator==(quick_key_type other) { return other.m_index == m_index; }
		constexpr bool operator< (quick_key_type other) { return other.m_index <  m_index; }
		~quick_key_type() = default;
	};

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
		Clone(other);
		return *this;
	}

	InstrusiveSortedDeque<T>& operator=(InstrusiveSortedDeque<T>&& other)
	{
		static_cast<StdDeque*>(this)->operator=(other);
		m_nMarkedAsErased = other.m_nMarkedAsErased;
		return *this;
	}

	// Accessors by quick_key
	reference at(quick_key_type key)
	{
		return GetByQuickKey<reference>(this, key);
	}

	const_reference at(quick_key_type key) const
	{
		return GetByQuickKey<const_reference>(this, key);
	}

	reference operator[](quick_key_type key)
	{
		return GetByQuickKey<reference>(this, key);
	}

	const_reference operator[](quick_key_type key) const
	{
		return GetByQuickKey<const_reference>(this, key);
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

	iterator quick_key_to_iterator(quick_key_type qk)
	{
		return QuickKeyToIterator(this, qk);
	}

	const_iterator quick_key_to_iterator(quick_key_type qk) const
	{
		return QuickKeyToIterator(this, qk);
	}

	size_type size() const
	{
		const size_type baseSize = capacity();
		if (baseSize <= 1) {
			assert(0 == m_nMarkedAsErased);
			return baseSize;
		}

		const ssize_t netSize = baseSize - m_nMarkedAsErased;
		assert(netSize > 1);
		return netSize;
	}

	// Size of the underlying deque
	size_type capacity() const
	{
		return StdDeque::size();
	}

	// Find methods which return an iterator to the specified key using a binary search
	const_iterator find(key_type k) const
	{
		typename StdDeque::const_iterator it;
		DoFind(this, it, StdDeque::cbegin(), StdDeque::cend(), k);
		return MakeFilteredIter(this, std::move(it));
	}

	// Find methods which return an iterator to the specified key using a binary search
	iterator find(key_type k)
	{
		typename StdDeque::iterator it;
		DoFind(this, it, StdDeque::begin(), StdDeque::end(), k);
		return MakeFilteredIter(this, std::move(it));
	}

	// An alternate find, starts by searching at the front of the deque before trying the usual search,
	// and returns a 'quick key' instead of an iterator
	quick_key_type find_front(key_type userKey) const
	{
		if (! this->empty()) {
			if (this->front().GetKey() == userKey) {
				return quick_key_type(quick_key_type::MIN_VALID_INDEX);
			}
			else {
				// TODO: cache the result that we find here, and try searching from the cached value.
				typename StdDeque::const_iterator it;
				if (DoFind(this, it, StdDeque::cbegin(), StdDeque::cend(), userKey)) {
					return quick_key_type(it - StdDeque::cbegin());
				}
			}
		}

		return quick_key_type();
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
		if (DoFind(this, it, StdDeque::begin(), StdDeque::end(), k)) {
			return erase(it);
		}

		return false;
	}

	bool erase(quick_key_type k)
	{
		return erase(StdDeque::begin() + k.m_index);
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

	// Wrappers for emplace_back() and emplace_front(), which return references to the newly created values
	// Note that this is incompatible with the C++ 17 interface, where emplace...() methods return iterators

	// A more flexible emplace_back which will attempt to emplace at the back but will insert at the correct position
	// if the new value is not in fact greater than the last value
	template< typename... Args >
	reference emplace_back(Args&&... args)
	{
		const_pointer const prevBack = this->empty() ? nullptr : & (this->back());
		StdDeque::emplace_back(std::forward<Args>(args)...);
		reference& back = this->back();
		assert(! back.IsDeleted());
		if (nullptr != prevBack) {
			assert(! prevBack->IsDeleted());
			if (BOOST_UNLIKELY(back.GetKey() <= prevBack->GetKey())) {
				assert(back.GetKey() < prevBack->GetKey());
				auto it = DoFindUnchecked(StdDeque::begin(), StdDeque::end() - 1, back.GetKey());
				assert((it->GetKey() > back.GetKey()) && (& *it != &back));
				auto newIt = StdDeque::emplace(it, std::move(back));
				StdDeque::pop_back();
				ValidateEdge(this->back());
				return *newIt;
			}
		}

		return back;
	}

	// TODO: Handle out-of-place values in emplace_front like in emplace_back
	template< typename... Args >
	reference emplace_front(Args&&... args)
	{
		const_pointer const prevFront = this->empty() ? nullptr : & (this->front());
		StdDeque::emplace_front(std::forward<Args>(args)...);
		assert(! this->front().IsDeleted());
		assert( (nullptr == prevFront) ||
				((! prevFront->IsDeleted()) && (this->front().GetKey() < prevFront->GetKey())));

		return this->front();
	}

	// FIXME: Implement resize() to  to maintain the invariants for m_nMarkedAsErased if it shrinks
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

	template <typename ThisType, typename IterType>
	static bool DoFind(ThisType thisPtr, IterType& result, IterType&& beginIter, IterType&& endIter, key_type k)
	{
		if (! thisPtr->empty() && (k <= thisPtr->back().GetKey())) {
			result = DoFindUnchecked(beginIter, endIter, k);
			// FIXME: We should handle cases when endIter != StdDeque::end()
			assert(result != thisPtr->StdDeque::end());
			if ((result->GetKey() == k) && (endIter != result)) {
				return true;
			}
		}

		result = thisPtr->StdDeque::end();
		return false;
	}

	template <typename IterType>
	static inline auto DoFindUnchecked(IterType&& beginIter, IterType&& endIter, key_type k)
	{
		constexpr auto FindComp = [](const T& value, typename T::KeyType k)->bool { return value.GetKey() < k; };
		return std::lower_bound(beginIter, endIter, k, FindComp);
	}

	void Clone(const InstrusiveSortedDeque& other)
	{
		StdDeque::resize(other.size());		// Pre-allocate space if necessary
		constexpr auto pred = [](const value_type& r) { return ! r.IsDeleted(); };
		std::copy_if(other.StdDeque::begin(), other.StdDeque::end(), StdDeque::begin(), pred);
		m_nMarkedAsErased = 0;
	}

	template <typename RefType, typename ThisType>
	static inline RefType GetByQuickKey(ThisType thisPtr, quick_key_type key)
	{
		return thisPtr->StdDeque::at(key.m_index);
	}

	template <typename ThisType>
	static auto QuickKeyToIterator(ThisType thisPtr, const quick_key_type qk)
	{
		if (qk.is_valid()) {
			auto it = thisPtr->StdDeque::begin() + qk.m_index;
			if (! it->IsDeleted()) {
				return MakeFilteredIter(thisPtr, std::move(it));
			}
		}

		return MakeFilteredIter(thisPtr, thisPtr->StdDeque::end());
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

	bool erase(typename StdDeque::iterator it)
	{
		if (! it->IsDeleted()) {
			it->Remove();
			assert(it->IsDeleted());
			++m_nMarkedAsErased;
			TrimFront();
			TrimBack();
			return true;
		}
		else {
			assert((capacity() > 1) && (m_nMarkedAsErased > 0));
			ValidateEdge(this->front());
			ValidateEdge(this->back());
			return false;
		}
	}
};

}	// namespace Utils

#endif /* UTILS_INTRUSIVESORTEDDEQUE_H_ */
