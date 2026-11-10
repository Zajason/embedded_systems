#include "detail/exceptions.hpp"
#ifndef VECTOR_H
#define VECTOR_H

#include "DDTIncludes.h"
#include <optional>
#include <functional>
#include "detail/stable_vector_sort.hpp"

////////////////////////////////////////////////////////////////////////////////
// Begin namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
namespace DDTLibrary {

	//////////////////////////////////////////////////////////////////////////////
	// Predefinition of Vector for friendliness
	//////////////////////////////////////////////////////////////////////////////
template <class T, class A = ddtr::malloc_allocator> class Vector;

	//////////////////////////////////////////////////////////////////////////////
	// Class template Vector
	// This is a simple Vector DDT
	//////////////////////////////////////////////////////////////////////////////

	template <class T, class A>
	class Vector {
	public:
		typedef A Allocator;
		////////////////////////////////////////////////////////////////////////
		// Typing behaviour of DDT
		////////////////////////////////////////////////////////////////////////
		typedef T* element_pointer;
		typedef T& element_reference;
		typedef T value_type;

		typedef T elemenType;
		typedef value_type* pointer;

		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef T input_value_type;
		typedef const input_value_type& const_input_reference;

		typedef size_t size_type;
		typedef ptrdiff_t difference_type;

		typedef Iterator_Base<Vector> iterator;
		typedef ConstIteratorAdapter<iterator> const_iterator;

		typedef ReverseIteratorAdapter<iterator, true> reverse_iterator;
		typedef ReverseIteratorAdapter<const_iterator, true> const_reverse_iterator;

		////////////////////////////////////////////////////////////////////////
		// Constructors
		////////////////////////////////////////////////////////////////////////
        // Copies of elements; get(n), operator[] and front()/back() return references.
        input_value_type read(size_type n) { return get(n); }
        input_value_type read(size_type n) const { return get(n); }
        void write(size_type n, const_input_reference value) { (*this)[n] = value; }

		Vector()
		{
			begin_ = nullptr;
			last_ = end_ = begin_;
			finish_ = begin_;
		}

		Vector(size_type sz)
		{
			const size_type alloc_size = sz;
			begin_ = alloc_size ? (pointer)Allocator::malloc(alloc_size*sizeof(value_type)) : nullptr;
			end_ = sz ? begin_+sz : begin_;
			last_ = end_;
			finish_ = alloc_size ? begin_ + alloc_size : begin_;
			for (element_pointer p = begin_;p < end_; ++p) {
                Constructor(p);
			}
		}

		Vector(size_type sz, const_input_reference value)
		{
			const size_type alloc_size = sz;
			begin_ = alloc_size ? (pointer)Allocator::malloc(alloc_size*sizeof(value_type)) : nullptr;
			end_ = sz ? begin_+sz : begin_;
			last_ = end_;
			finish_ = alloc_size ? begin_ + alloc_size : begin_;
			for (element_pointer p = begin_;p < end_; ++p) {
                Constructor(p, value);
			}
		}
        
        Vector(iterator first, iterator last) {
			begin_ = nullptr;
            last_ = end_ = begin_;
            finish_ = begin_;
            
            for (;first!= last; first++) {
                push_back(*first);
            }
        }

		Vector(const Vector& other)
		{
			size_type other_size = other.size();
			size_type alloc_size = other_size;
			begin_ = alloc_size ? (pointer)Allocator::malloc(alloc_size*sizeof(value_type)) : nullptr;
			finish_ = alloc_size ? begin_ + alloc_size : begin_;
            element_pointer i;
			end_ = begin_;
            DDTR_TRY {
                for (i = other.begin_;i < other.end_; ++i) {
                Constructor(end_, *i);
                ++end_;
			}
            } DDTR_CATCH_ALL {
                Destructor(begin_, end_);
                if (begin_) Allocator::free(begin_);
                DDTR_RETHROW();
            }
			last_ = end_;
		}

		template <typename A2>
		Vector(const Vector<T, A2>& other)
		{
			size_type other_size = other.size();
			size_type alloc_size = other_size;
			begin_ = alloc_size ? (pointer)Allocator::malloc(alloc_size*sizeof(value_type)) : nullptr;
			finish_ = alloc_size ? begin_ + alloc_size : begin_;
			typename Vector<T, A2>::element_pointer i;
			end_ = begin_;
            DDTR_TRY {
                for (i = other.begin_;i < other.end_; ++i) {
                Constructor(end_, *i);
                ++end_;
			}
            } DDTR_CATCH_ALL {
                Destructor(begin_, end_);
                if (begin_) Allocator::free(begin_);
                DDTR_RETHROW();
            }
            last_ = end_;
		}

        // Standard range assignment. The input range must not belong to *this.
        // Consume input iterators once; do not build a staging container.
        template<class InputIterator>
        void assign(InputIterator first, InputIterator last) {
            clear();
            for (; first != last; ++first)
                push_back(*first);
        }

		Vector& operator=(const Vector& other) {
			if (this != &other) {
				Vector tmp(other);
				swap(tmp); 
			}
			return *this;
		}

		template <typename A2>
		Vector& operator=(const Vector<T, A2>& other) {
			Vector tmp(other);
			swap(tmp); 
			return *this;
		}

		~Vector() {
			clear();
            if (begin_) Allocator::free(begin_);
		}

		template <typename A2>
		void swap(Vector<T, A2>& other) {
            element_pointer tmp = other.begin_;
			other.begin_ = begin_;
			begin_ = tmp;
			tmp = other.end_;
			other.end_ = end_;
			end_ = tmp;
			tmp = other.last_;
			other.last_ = last_;
			last_ = tmp;
			tmp = other.finish_;
			other.finish_ = finish_;
			finish_ = tmp;
		}

		iterator begin() const {
            iterator it(this, getFirstElement());
			return it;
		}
		iterator end() const {
            iterator it(this, getEnd());
            return it;
		}
		////////////////////////////////////////////////////////////////////////
		// Reverse iterators of DDT
		////////////////////////////////////////////////////////////////////////
		reverse_iterator rbegin() {
            reverse_iterator it(this, getLastElement());
			return it;
		}
		const_reverse_iterator rbegin() const {
            const_reverse_iterator it(this, getLastElement());
			return it;
		}
		reverse_iterator rend() {
			reverse_iterator it(this, getREnd());
            return it;
		}
		const_reverse_iterator rend() const {
			const_reverse_iterator it(this, getREnd());
            return it;
		}			
		////////////////////////////////////////////////////////////////////////
		// iterator integration
		////////////////////////////////////////////////////////////////////////
        inline element_pointer decrement(element_pointer data) const {
			return --data;
		}
		inline element_pointer increment(element_pointer data) const {
			return ++data;
		}
		inline element_pointer getData(element_pointer data) const {
			return data;
		}
		////////////////////////////////////////////////////////////////////////
		// Element access used by the iterators
		////////////////////////////////////////////////////////////////////////
		inline element_pointer getFirstElement() const {
			return begin_;
		}
		inline element_pointer getLastElement() const {
			if (last_ == begin_) return nullptr;
            return last_ - 1;
		}
		inline element_pointer getEnd() const {
			return end_;
		}
        inline element_pointer getREnd() const { return nullptr; }

        inline element_pointer reverse_decrement(element_pointer data) const {
            if (data == begin_) return nullptr;
            return data - 1;
        }
        inline element_pointer reverse_increment(element_pointer data) const {
            if (!data) {
                return begin_;
            }
            return data + 1;
        }
		bool exists(element_pointer x) {
			if (!empty()) {
				iterator it1(this, begin_);
				iterator it2(this, end_);
				for (Iterator_Base<Vector> it = it1;it != it2; it++) {
					if (it.getData() == x){
						return true;
					}
				}
			}
			return false;
		}
		inline bool IsNil(element_pointer x) {
			return (x == end_) || (x == nullptr);
		}
		////////////////////////////////////////////////////////////////////////
		// Size information
		////////////////////////////////////////////////////////////////////////
		bool empty() const {
            return (begin_ == end_);
		}
		size_type size() const {
            return begin_ ? size_type(end_ - begin_) : 0;
		}
		inline size_type getSize() const {
			return size();
		}
		size_type capacity() {
            return begin_ ? finish_ - begin_ : 0;
		}

		size_t footprint() {
			return sizeof(*this) + (begin_ ? (size_t)(finish_ - begin_) : 0) * sizeof(value_type);
		}

        void sort() {
            const auto n=size();
            if(n<2) return;
            detail::StableVectorSort<input_value_type,Allocator> sorter(n);
            sorter(begin_,n);
        }

		void reserve(size_type new_capacity) {

			if (new_capacity > capacity()) {
				const size_type old_size = size();
				element_pointer new_storage = (pointer)Allocator::malloc(new_capacity * sizeof(value_type));
				element_pointer tmp = new_storage;
				DDTR_TRY {
                    for (element_pointer i = begin_;i != end_; ++i, ++tmp) {
                    Constructor(tmp, *i);
				}
                } DDTR_CATCH_ALL {
                    Destructor(new_storage, tmp);
                    Allocator::free(new_storage);
                    DDTR_RETHROW();
                }
				Destructor(begin_, end_);
				if (begin_) Allocator::free(begin_);
				begin_ = new_storage;
				end_ = begin_ + old_size;
				last_ = end_;
				finish_ = begin_ + new_capacity;
			}
        }

		////////////////////////////////////////////////////////////////////////
		// Basic access operations
		////////////////////////////////////////////////////////////////////////
		
		reference operator[](size_type n) const {
            return get(n);
        }

		reference get(size_type n) const {
			return *(begin_ + n); 
		}

		reference front() const { 
			return get(0); }
		
		reference back() const {
			return get(size() - 1); 
		}

		////////////////////////////////////////////////////////////////////////
		// Basic modification operations
		////////////////////////////////////////////////////////////////////////
		void clear() {
			if (!empty()) {
				Destructor(begin_, end_);
			}
            end_ = begin_;
            last_ = begin_;
        }

        // Same linear lower-bound walk as a sorted linked sequence.
        template<class Compare>
        void insert_sorted(const_input_reference value, Compare less) {
            auto position = begin_;
            const auto finish = end_;
            while (position != finish) {
                if (!less(*position, value)) break;
                ++position;
            }
            insert(iterator(this, position), value);
        }

		        iterator insert(iterator pos, const_input_reference value) {
            const auto index = begin_ ? pos.getData() - begin_ : 0;
            const auto old_size = size();
            std::optional<input_value_type> saved;
            const auto source = std::addressof(value);
            if ((!std::less<const input_value_type*>{}(source, begin_) && std::less<const input_value_type*>{}(source, end_))) {
                saved.emplace(value);
            }
            const auto& input = saved ? *saved : value;
            if (old_size == capacity()) reserve(capacity() ? capacity() * 2 : 1);
            auto destination = begin_ + index;
            if (destination == end_) {
                Constructor(end_, input);
            } else {
                Constructor(end_, *(end_ - 1));
                DDTR_TRY {
                    for (auto target = end_ - 1;target != destination; --target) *target = *(target - 1);
                    *destination = input;
                } DDTR_CATCH_ALL {
                    Destructor(end_);
                    DDTR_RETHROW();
                }
            }
            ++end_;
            last_ = end_;
            return iterator(this, destination);
        }
iterator erase(iterator pos) {
			if (pos._data + 1 != end_) {
				element_pointer o = pos._data;
				for (element_pointer i = o + 1;i != end_; ++i, ++o) {
					*o = *i;
				}
			}
			Destructor((--end_));
			last_--;
			return pos;
		}
        
        iterator copy_move(iterator first, iterator last, iterator result) {
			while (first != last) {
                *result++ = *first++;
            }
            return result;
        }

        iterator erase(iterator first, iterator last) {
            if (first != last) {
                auto old_end = end_;
                auto destination = first.getData();
                auto source = last.getData();
                if (source != old_end) {
                    while (source != old_end) *destination++ = *source++;
                }
                Destructor(destination, old_end);
                end_ = last_ = destination;
            }
            return first;
        }

        void push_back(const_input_reference value) {
            // Only growth invalidates an aliased input; protect it before reserve.
            std::optional<input_value_type> saved;
            if(size() == capacity()) {
                const auto source=std::addressof(value);
                if(!std::less<const input_value_type*>{}(source,begin_) &&
                    std::less<const input_value_type*>{}(source,end_)) {
                    saved.emplace(value);
                }
                reserve(capacity() ? capacity()*2 : 1);
            }
            Constructor(end_, saved ? *saved : value);
            ++end_;
            last_=end_;
        }

		void pop_back() {
			iterator it(this, end_ - 1);
			erase(it);
		}
		void pop_front() {
			iterator it(this, begin_);
			erase(it);
		}

		void resize(size_type new_size, const_input_reference value = input_value_type()) {
			const size_type old_size = size();
			if (new_size > old_size) {
				reserve(new_size);
				for (size_type i = old_size;i < new_size; i++) {
					push_back(value);
				}
			} else if (new_size < old_size) {
				for (size_type i = new_size;i < old_size; i++) {
					pop_back();
				}
			}
        }

		////////////////////////////////////////////////////////////////////////
		// Syntactic sugar
		////////////////////////////////////////////////////////////////////////
		void set(size_type n, const_input_reference value) {
			get(n) = value;
		}

		void add(size_type n, const_input_reference value) {
			insert(iterator(this, n ? begin_ + n : begin_), value);
		}

		void remove(size_type n) {
			erase(iterator(this, begin_ + n));
		}

		void remove_value(const_input_reference value) {
			for (element_pointer i = begin_;i != end_; ++i) {
				if (*i == value) {
                    erase(iterator(this, i));
					return;
				}
			}

		}

		void append(const_input_reference value) {
			push_back(value);
		}

		////////////////////////////////////////////////////////////////////////
		// Allocation related items
		////////////////////////////////////////////////////////////////////////
	
	private:

		element_pointer erase(element_pointer item) {
			if (item + 1 != end_) {
				element_pointer o = item;
				for (element_pointer i = o + 1;i != end_; ++i, ++o) {
					*o = *i;
				}
			}
			Destructor((--end_));
			last_--;
			return item;
		}

		element_pointer begin_;
		element_pointer last_;
		element_pointer end_;
		element_pointer finish_;

		template <class, class> friend class Vector;
	};

}
////////////////////////////////////////////////////////////////////////////////
// End namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
#endif
