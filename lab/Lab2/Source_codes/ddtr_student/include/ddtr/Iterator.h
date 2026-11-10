#ifndef ITERATOR_H
#define ITERATOR_H

#include "DDTIncludes.h"
////////////////////////////////////////////////////////////////////////////////
// Begin namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
namespace DDTLibrary {
	template <class iteratorExtension, bool Contiguous = false> class ReverseIteratorAdapter;
	template <class iteratorExtension> class ConstIteratorAdapter;
	template <class DataStructureType> class Iterator_Base;

	template <class, class> class Vector;
	template <class> class List;
	template <class> class Tree;
	template <class, class, int> class BTree;

	///////////////////////////////////////////////////////////////
	// Iterator base: a position (_data) in a data structure, which
	// provides increment/decrement/getData for its element pointers.
	///////////////////////////////////////////////////////////////
	template <class DataStructureType>
	class Iterator_Base {
	public:
		typedef std::bidirectional_iterator_tag iterator_category;
		typedef DataStructureType IteratorDataStructureType;
		typedef typename DataStructureType::value_type value_type;
		typedef typename DataStructureType::pointer pointer;
		typedef typename DataStructureType::reference reference;
		typedef typename DataStructureType::element_pointer element_pointer;
		typedef typename DataStructureType::size_type size_type;
		typedef typename DataStructureType::difference_type difference_type;
		typedef typename DataStructureType::input_value_type input_value_type;

		// Constructors
		Iterator_Base () : _data(nullptr), _datastructure(nullptr) {}

		Iterator_Base (const DataStructureType* datastructure, element_pointer data)
			: _data(data), _datastructure(datastructure) {}

		Iterator_Base (const Iterator_Base& itr) = default;
		Iterator_Base& operator=(const Iterator_Base& other) = default;

		virtual ~Iterator_Base() {}

		virtual void increment() {
			_data = _datastructure->increment(_data);
		}
		virtual void decrement() {
			_data = _datastructure->decrement(_data);
		}

		Iterator_Base& operator++() {
			increment();
			return *this;
		}
		Iterator_Base operator++(int) {
			Iterator_Base tmp = *this;
			increment();
			return tmp;
		}
		Iterator_Base& operator--() {
			decrement();
			return *this;
		}
		Iterator_Base operator--(int) {
			Iterator_Base tmp = *this;
			decrement();
			return tmp;
		}

		Iterator_Base& operator+=(size_type n) {
			for (size_type i = 0; i < n; i++)
				increment();
			return *this;
		}
		Iterator_Base operator+(size_type n) const {
			Iterator_Base tmp(*this);
			tmp += n;
			return tmp;
		}
		Iterator_Base& operator-=(size_type n) {
			for (size_type i = 0; i < n; i++)
				decrement();
			return *this;
		}
		Iterator_Base operator-(size_type n) const {
			Iterator_Base tmp(*this);
			tmp -= n;
			return tmp;
		}

		// Distance from other to *this, walking forward: other must not be past *this.
		difference_type operator-(const Iterator_Base& other) const {
			difference_type diff = 0;
			for (Iterator_Base it(other); it != *this; ++it)
				diff++;
			return diff;
		}

		bool operator==(const Iterator_Base& other) const { return _data == other._data; }
		bool operator!=(const Iterator_Base& other) const { return _data != other._data; }
		bool operator<=(const Iterator_Base& other) const { return _data <= other._data; }
		bool operator>=(const Iterator_Base& other) const { return _data >= other._data; }
		bool operator<(const Iterator_Base& other) const { return _data < other._data; }
		bool operator>(const Iterator_Base& other) const { return _data > other._data; }

		input_value_type read() const {
			return *_datastructure->getData(_data);
		}
		void write(const input_value_type& value) {
			*_datastructure->getData(_data) = value;
		}

		reference operator*() const {
			return *_datastructure->getData(_data);
		}
		pointer operator->() const {
			return _datastructure->getData(_data);
		}

		element_pointer getData() const {
			return _data;
		}

	protected:
		element_pointer _data;                    // The item being inspected
		const DataStructureType* _datastructure;  // The data structure being inspected

		template <class, class> friend class Vector;
		template <class> friend class List;
		template <class> friend class Tree;
		template <class, class, int> friend class BTree;
	};

	///////////////////////////////////////////////////////////////
	// Iterator Adapters. These classes add functionality to
	// the iterator.
	///////////////////////////////////////////////////////////////
	template <class iteratorExtension, bool Contiguous>
	class ReverseIteratorAdapter : public iteratorExtension {
	public:
		typedef typename iteratorExtension::IteratorDataStructureType DataStructureType;
		typedef typename iteratorExtension::pointer pointer;
		typedef typename iteratorExtension::reference reference;
		typedef typename iteratorExtension::size_type size_type;
		typedef typename iteratorExtension::difference_type difference_type;
		typedef typename iteratorExtension::element_pointer element_pointer;
		typedef typename iteratorExtension::input_value_type input_value_type;

		// Constructors
		ReverseIteratorAdapter () {}
		ReverseIteratorAdapter(const ReverseIteratorAdapter&) = default;
		ReverseIteratorAdapter& operator=(const ReverseIteratorAdapter&) = default;

		ReverseIteratorAdapter (const iteratorExtension& itr)
			: iteratorExtension(itr) {
		}

		virtual void increment() {
			if constexpr (Contiguous) {
				this->_data = this->_datastructure->reverse_decrement(this->_data);
			} else {
				iteratorExtension::decrement();
			}
		}
		virtual void decrement() {
			if constexpr (Contiguous) {
				this->_data = this->_datastructure->reverse_increment(this->_data);
			} else {
				iteratorExtension::increment();
			}
		}

		ReverseIteratorAdapter& operator++() {
			increment();
			return *this;
		}
		ReverseIteratorAdapter operator++(int) {
			ReverseIteratorAdapter tmp = *this;
			increment();
			return tmp;
		}
		ReverseIteratorAdapter& operator--() {
			decrement();
			return *this;
		}
		ReverseIteratorAdapter operator--(int) {
			ReverseIteratorAdapter tmp = *this;
			decrement();
			return tmp;
		}

		ReverseIteratorAdapter& operator+=(size_type n) {
			for (size_type i = 0; i < n; i++)
				increment();
			return *this;
		}
		ReverseIteratorAdapter operator+(size_type n) const {
			ReverseIteratorAdapter tmp(*this);
			tmp += n;
			return tmp;
		}
		ReverseIteratorAdapter& operator-=(size_type n) {
			for (size_type i = 0; i < n; i++)
				decrement();
			return *this;
		}
		ReverseIteratorAdapter operator-(size_type n) const {
			ReverseIteratorAdapter tmp(*this);
			tmp -= n;
			return tmp;
		}

	protected:
		ReverseIteratorAdapter (const DataStructureType* datastructure, element_pointer data)
			: iteratorExtension(datastructure, data) {}

		template <class, class> friend class Vector;
		template <class> friend class List;
		template <class> friend class Tree;
		template <class, class, int> friend class BTree;
	};

	template <class iteratorExtension>
	class ConstIteratorAdapter : public iteratorExtension {
	public:
		typedef typename iteratorExtension::IteratorDataStructureType DataStructureType;
		typedef typename iteratorExtension::pointer pointer;
		typedef typename iteratorExtension::reference reference;
		typedef typename iteratorExtension::size_type size_type;
		typedef typename iteratorExtension::difference_type difference_type;
		typedef typename iteratorExtension::element_pointer element_pointer;

		// Constructors
		ConstIteratorAdapter () {}
		ConstIteratorAdapter(const ConstIteratorAdapter&) = default;
		ConstIteratorAdapter& operator=(const ConstIteratorAdapter&) = default;

		ConstIteratorAdapter (const Iterator_Base<DataStructureType>& itr)
			: iteratorExtension (itr) { }

		void write(const typename DataStructureType::input_value_type&) = delete;

	protected:
		ConstIteratorAdapter (const DataStructureType* datastructure, element_pointer data)
			: iteratorExtension(datastructure, data) {}

		template <class, class> friend class Vector;
		template <class> friend class List;
		template <class> friend class Tree;
		template <class, class, int> friend class BTree;
	};

}
////////////////////////////////////////////////////////////////////////////////
// End namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
#endif
