#include "detail/exceptions.hpp"
// Copyright 2018 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This file is a port of Abseil's B-tree, absl/container/internal/btree.h
// (https://github.com/abseil/abseil-cpp, commit 1a8a725), the implementation
// behind absl::btree_multiset, to the DDTR library. A copy of the Apache 2.0
// license is in LICENSE-abseil.txt at the root of this library.
//
// What was kept, as Abseil has it: the node layout and node-size computation
// (by default a 256-byte target and at least 4 values per node, and no child
// array in leaves), the
// leaf root that grows by doubling, the static empty node, the leftmost node
// stored as the root's parent and the rightmost pointer, linear in-node search
// for arithmetic keys and binary search otherwise, rebalancing with siblings
// before splitting on insert (biased by the insertion point), and merge/borrow
// on erase with its delete-from-the-front/back guards. The algorithms below keep
// Abseil's function names (emplace_value, rebalance_or_split, split,
// try_merge_or_rebalance, ...) so they can be compared side by side.
//
// What was changed, and why:
//  * Node getters and setters are member functions of the tree that take the
//    node as an argument (isLeaf(n), finishOf(n), set_finish(n, v), ...).
//  * Multiset insertion and single-descent insert_unique use operator<.
//    Unique insertion follows Abseil's boolean-comparator path. Hinted insertion,
//    erase_range, copy/move/swap, node handles, custom comparators, allocators
//    other than the DDTR one, key-compare-to search, and the sanitizer and
//    iterator-generation debugging aids were left out.
//  * The node size can also be given directly as the number of values per node
//    (the third template argument), including 3, which Abseil's own minimum of 4
//    rules out for its default sizing. Splitting needs 3 values per node.
//  * Indexing by sorted rank, which the DDTR containers provide and Abseil does
//    not, walks the iterators (Abseil's own increment_n_slow and distance_slow)
//    rather than adding bookkeeping to the nodes, so it costs what it costs
//    with Abseil: O(n / values-per-node).

#ifndef BTREE_H
#define BTREE_H

#include "DDTIncludes.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <stdexcept>
#include <type_traits>
#include <utility>

////////////////////////////////////////////////////////////////////////////////
// Begin namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
namespace DDTLibrary {

	// NodeSlots is the number of values per node; 0 sizes nodes as Abseil does (256 bytes).
	template <class T, class A = ddtr::malloc_allocator, int NodeSlots = 0> class BTree;

	template <class T, class A, int NodeSlots>
	class BTree : public allocated<A> {
		// ---- node layout ------------------------------------------------------
		// absl::container_internal::btree_node is laid out by Layout as
		//   parent, position, start, finish, max_count, slots[], children[]
		// with the four counters sharing one integral type big enough for the
		// number of values that fit a node.
		static constexpr std::size_t align_up(std::size_t n, std::size_t m) { return (n + m - 1) & ~(m - 1); }
		static constexpr std::size_t kTargetNodeSize = 256; // Abseil's default node size in bytes
		static constexpr std::size_t kNodeSlotSpace = kTargetNodeSize - (sizeof(void*) + 4);
		typedef typename std::conditional<
			(NodeSlots > 0 ? NodeSlots > 255 : kNodeSlotSpace / sizeof(T) > 255), std::uint16_t, std::uint8_t>::type field_type;
	public:
		struct Node {
			Node* parent;
			field_type position;  // index of the node in its parent
			field_type start;     // always 0
			field_type finish;    // one past the last value (the node's count)
			field_type max_count; // slot capacity of a leaf; kInternalNodeMaxCount marks an internal node
		};
	private:
		static constexpr std::size_t kSlotsOffset = align_up(sizeof(void*) + 4 * sizeof(field_type), alignof(T));
		static constexpr std::size_t LeafSize(std::size_t slots) {
			return align_up(kSlotsOffset + slots * sizeof(T), alignof(Node*));
		}
		// Number of values of a leaf that fits kTargetNodeSize bytes (binary search, as Abseil).
		static constexpr std::size_t NodeTargetSlots(std::size_t begin, std::size_t end) {
			return begin == end ? begin
				: LeafSize((begin + end) / 2 + 1) > kTargetNodeSize
					? NodeTargetSlots(begin, (begin + end) / 2)
					: NodeTargetSlots((begin + end) / 2 + 1, end);
		}
		// With Abseil's sizing there are at least 4 values per node: splitting needs 3, and
		// 3 gives a bad worst-case occupancy. An explicit NodeSlots may be as small as 3.
		static constexpr std::size_t kMinNodeSlots = 4;
		static constexpr std::size_t kNodeTargetSlots = NodeTargetSlots(0, kTargetNodeSize);
		static constexpr std::size_t kNodeSlots = NodeSlots > 0 ? static_cast<std::size_t>(NodeSlots)
			: (kNodeTargetSlots >= kMinNodeSlots ? kNodeTargetSlots : kMinNodeSlots);
		static_assert(NodeSlots == 0 || NodeSlots >= 3, "a B-tree node must hold at least 3 values to be split");
		static constexpr std::size_t kMinNodeValues = kNodeSlots / 2;
		static constexpr std::size_t kChildrenOffset = align_up(kSlotsOffset + kNodeSlots * sizeof(T), alignof(Node*));
		static constexpr std::size_t InternalSize() { return kChildrenOffset + sizeof(Node*) * (kNodeSlots + 1); }
		static constexpr field_type kInternalNodeMaxCount = 0;
		static_assert(kNodeSlots < (std::size_t(1) << (8 * sizeof(field_type))), "too many values per node");
		static_assert(alignof(T) <= alignof(std::max_align_t), "over-aligned values are not supported");
		// Abseil searches arithmetic keys ordered by std::less linearly.
		static constexpr bool use_linear_search = std::is_arithmetic<T>::value;

	public:
		typedef A Allocator;
		typedef T value_type;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;
		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef T input_value_type;
		typedef const input_value_type& const_input_reference;
		typedef std::size_t size_type;
		typedef std::ptrdiff_t difference_type;

		// A position in the tree (Abseil's btree_iterator): value `position` of `node`.
		// end() is the slot after the last value of the rightmost leaf.
		struct cursor {
			Node* node;
			int position;
			cursor() : node(nullptr), position(-1) {}
			cursor(std::nullptr_t) : node(nullptr), position(-1) {}
			cursor(Node* n, int p) : node(n), position(p) {}
			bool operator==(const cursor& other) const { return node == other.node && position == other.position; }
			bool operator!=(const cursor& other) const { return !(*this == other); }
		};
		typedef cursor element_pointer;
		typedef cursor& element_reference;

		typedef Iterator_Base<BTree> iterator;
		typedef ConstIteratorAdapter<iterator> const_iterator;

		typedef ReverseIteratorAdapter<iterator> reverse_iterator;
		typedef ReverseIteratorAdapter<const_iterator> const_reverse_iterator;

		// ---------------- Construction and destruction ---------------

		BTree() : root_(EmptyNode()), rightmost_(EmptyNode()), size_(0) {
		}
		// Copies the other tree's node structure in linear time, as Tree's copy does: each value
		// is copied once and nothing is compared or searched.
		BTree(const BTree& other) : BTree() {
			if (!other.is_empty()) {
				Node* first_leaf = nullptr;
				Node* last_leaf = nullptr;
				Node* copy = clone_node(other, other.root(), nullptr, 0, first_leaf, last_leaf);
				if (isInternal(copy)) {
					set_parent(copy, first_leaf); // the leftmost leaf is stored as the root's parent
				}
				mutable_root() = copy;
				mutable_rightmost() = last_leaf;
				size_ = other.getSize();
			}
		}
		// Takes the other tree's nodes; the other tree is left empty.
		BTree(BTree&& other) : BTree() {
			Node* other_root = other.root();
			Node* other_rightmost = other.rightmost();
			const size_type other_size = other.getSize();
			other.mutable_root() = other.mutable_rightmost() = EmptyNode();
			other.size_ = 0;
			mutable_root() = other_root;
			mutable_rightmost() = other_rightmost;
			size_ = other_size;
		}
		BTree& operator=(const BTree&) = delete;
		~BTree() {
			clear();
		}

		// Copies of elements; get(n), operator[] and front()/back() return references.
		input_value_type read(size_type n) { return get(n); }
		input_value_type read(size_type n) const { return get(n); }
		void write(size_type n, const_input_reference value) { (*this)[n] = value; }

		// Deletes all values and returns to the empty state.
		void clear() {
			if (!is_empty()) {
				clear_and_delete(root());
			}
			mutable_root() = mutable_rightmost() = EmptyNode();
			size_ = 0;
		}

		////////////////////////////////////////////////////////////////////////
		// Basic iterators of DDT
		////////////////////////////////////////////////////////////////////////
		const_iterator begin() const {
			const_iterator it(const_cast<BTree*>(this), getFirstElement());
			return it;
		}
		iterator begin() {
			iterator it(this, getFirstElement());
			return it;
		}
		const_iterator end() const {
			const_iterator it(const_cast<BTree*>(this), getEnd());
			return it;
		}
		iterator end() {
			iterator it(this, getEnd());
			return it;
		}
		// Forward iterator at the first key >= k (or end() if none).
		iterator lower_bound(const_input_reference k) {
			cursor c = internal_end(internal_lower_bound(k));
			return iterator(this, c);
		}
		// Forward iterator at the first key > k (or end() if none).
		iterator upper_bound(const_input_reference k) {
			cursor c = internal_end(internal_upper_bound(k));
			return iterator(this, c);
		}
		// Forward iterator at the last (largest) element.
		iterator last() {
			size_type n = size();
			iterator it(this, getLastElement());
			return it;
		}
		////////////////////////////////////////////////////////////////////////
		// Reverse iterators of DDT
		////////////////////////////////////////////////////////////////////////
		const_reverse_iterator rbegin() const {
			const_reverse_iterator it(const_cast<BTree*>(this), getLastElement());
			return it;
		}
		reverse_iterator rbegin() {
			reverse_iterator it(this, getLastElement());
			return it;
		}
		const_reverse_iterator rend() const {
			const_reverse_iterator it(const_cast<BTree*>(this), getREnd());
			return it;
		}
		reverse_iterator rend() {
			reverse_iterator it(this, getREnd());
			return it;
		}

		////////////////////////////////////////////////////////////////////////
		// Size information
		////////////////////////////////////////////////////////////////////////
		inline bool empty() const {
			return is_empty();
		}
		inline size_type size() const {
			return getSize();
		}
		// Abseil's bytes_used(): the tree object plus its nodes.
		size_t footprint() const {
			return bytes_used();
		}
		inline size_type capacity() const {
			return getSize();
		}

        // Ordered insertion already maintains stable in-order traversal.
        void sort() {
        }

		void reserve(size_type new_capacity) {
			(void)new_capacity;
		}

		reference operator[](size_type n) {
			return get(n);
		}

		const_reference operator[](size_type n) const {
			return get(n);
		}

		// Value with the given sorted rank, reached by advancing from begin().
		reference get(size_type n) {
			pointer payload = payload_at(n);
			return *payload;
		}

		const_reference get(size_type n) const {
			pointer payload = payload_at(n);
			return *payload;
		}

		const_reference front() const {
			pointer payload = payload_of(getFirstElementChecked());
			return *payload;
		}
		reference front() {
			pointer payload = payload_of(getFirstElementChecked());
			return *payload;
		}
		const_reference back() const {
			pointer payload = payload_of(getLastElementChecked());
			return *payload;
		}
		reference back() {
			pointer payload = payload_of(getLastElementChecked());
			return *payload;
		}

		iterator erase(const iterator& pos) {
			return iterator(this, erase(pos.getData()));
		}

		reverse_iterator erase(const reverse_iterator& pos) {
			// The predecessor of the erased value is the one before its successor.
			cursor next = erase(pos.getData());
			return reverse_iterator(this, decrement(next));
		}

		iterator insert(const_input_reference value) {
			T v(value); // a split may move the element `value` refers to
			cursor at = insert_multi(v, v);
			return iterator(this, at);
		}

		// Multiset insertion right before a valid hint, one whose predecessor is not greater
		// than the value and which is not less than it (Abseil's insert_hint_multi). Any other
		// hint falls back to ordinary (upper-bound) insertion, as in Tree.
		iterator insert(const iterator& hint, const_input_reference value) {
			T v(value); // a split may move the element `value` refers to
			cursor position = hint.getData();
			bool fits = false;
			if (!is_empty()) {
				if (position == getEnd() || !(keyAt(position.node, position.position) < v)) {
					if (position == getFirstElement()) {
						fits = true;
					} else {
						cursor before = decrement(position);
						fits = !(v < keyAt(before.node, before.position));
					}
				}
			}
			cursor at = fits ? internal_emplace(position, v) : insert_multi(v, v);
			return iterator(this, at);
		}

		// Erases [first, last), value by value (Abseil's erase_range).
		iterator erase(const iterator& first, const iterator& last) {
			cursor c = first.getData();
			const cursor stop = last.getData();
			size_type count = 0;
			for (cursor i = c; i != stop; i = increment(i)) {
				++count;
			}
			for (; count > 0; --count) {
				c = erase(c);
			}
			return iterator(this, c);
		}

		// Updates a value's payload in place, without copying it or changing its key.
		template<class Update> void update(const iterator& position, Update change) {
			pointer payload = getData(position.getData());
			change(*payload);
		}

        std::pair<iterator, bool> insert_unique(const_input_reference value) {
            auto result = internal_insert_unique(value);
            return {iterator(this, result.first), result.second};
        }

		void push_back(const_input_reference value) {
			T v(value); // a split may move the element `value` refers to
			cursor at = insert_multi(v, v);
			(void)at;
		}

		void push_front(const_input_reference value) {
			push_back(value);
		}

		void pop_back() {
			if (!is_empty())
				erase(decrement(getEnd()));
		}

		void pop_front() {
			if (!is_empty())
				erase(getFirstElement());
		}

		void add(size_type n, const_input_reference value) {
			(void)n;
			push_back(value);
		}

		void resize(size_type new_size) {
			while (size() > new_size)
				erase(decrement(getEnd()));
			while (size() < new_size) {
				T v = input_value_type();
				insert_multi(v, v);
			}
		}

		// Removes one value equal to `value` (the first of any run of equal values).
		void remove(const_input_reference value) {
			cursor found = internal_find(value);
			if (found.node != nullptr)
				erase(found);
		}

		void remove_value(const_input_reference value) {
			remove(value);
		}

		// Owning handle to a value taken out of the tree. The value lives in a
		// separate allocation that is released with the handle.
		struct extracted_deleter {
			void operator()(pointer p) const {
				if (!p) return;
				Destructor(p);
				Allocator::free(p);
			}
		};
		using extracted_value = std::unique_ptr<input_value_type, extracted_deleter>;

		extracted_value ExtractMin() {
			if (is_empty()) return extracted_value(nullptr, extracted_deleter{});
			cursor first = getFirstElement();
			pointer holder = static_cast<pointer>(Allocator::malloc(sizeof(input_value_type)));
			pointer payload = &slots(first.node)[first.position];
			::new (static_cast<void*>(holder)) input_value_type(std::move(*payload));
			erase(first);
			return extracted_value(holder, extracted_deleter{});
		}

		// STL-style lookup permits erase(iterator) without searching a second time.
		iterator find(const_input_reference item) {
			cursor found = internal_find(item);
			return iterator(this, found.node ? found : getEnd());
		}
		const_iterator find(const_input_reference item) const {
			cursor found = internal_find(item);
			return const_iterator(const_cast<BTree*>(this), found.node ? found : getEnd());
		}

		pointer Find(const_input_reference item) {
			cursor found = internal_find(item);
			return found.node != nullptr ? &slots(found.node)[found.position] : nullptr;
		}

		////////////////////////////////////////////////////////////////////////
		// Interface for iterators
		////////////////////////////////////////////////////////////////////////
		// btree_iterator::increment / increment_slow
		cursor increment(cursor c) const {
			if (isLeaf(c.node) && ++c.position < static_cast<int>(finishOf(c.node))) {
				return c;
			}
			return increment_slow(c);
		}
		// btree_iterator::decrement / decrement_slow
		cursor decrement(cursor c) const {
			if (isLeaf(c.node) && --c.position >= static_cast<int>(startOf(c.node))) {
				return c;
			}
			return decrement_slow(c);
		}
		inline pointer getData(cursor c) const {
			return &slots(c.node)[c.position];
		}
		// begin(): the leftmost node is stored as the parent of the root.
		inline cursor getFirstElement() const {
			Node* node = leftmost();
			return cursor(node, startOf(node));
		}
		// The last value, or the reverse end when empty (--end() in Abseil).
		inline cursor getLastElement() const {
			return decrement(getEnd());
		}
		// end(): one past the last value of the rightmost leaf.
		inline cursor getEnd() const {
			return cursor(rightmost(), finishOf(rightmost()));
		}
		// What decrementing begin() yields.
		inline cursor getREnd() const {
			return cursor(leftmost(), -1);
		}
		inline size_type getSize() const {
			return size_;
		}

		// The number of leaf, internal and total nodes used by the btree.
		size_type leaf_nodes() const { return internal_stats(root_).first; }
		size_type internal_nodes() const { return internal_stats(root_).second; }
		size_type nodes() const { auto s = internal_stats(root_); return s.first + s.second; }
		// The height of the btree. An empty tree has height 0.
		size_type height() const {
			size_type h = 0;
			if (size_ != 0) {
				const Node* n = root_;
				do {
					++h;
					n = n->parent;
				} while (n != root_);
			}
			return h;
		}
		// The total number of bytes used by the btree.
		size_type bytes_used() const {
			const auto stats = internal_stats(root_);
			if (stats.first == 1 && stats.second == 0) {
				return sizeof(*this) + LeafSize(root_->max_count);
			}
			return sizeof(*this) + stats.first * LeafSize(kNodeSlots) + stats.second * InternalSize();
		}
		// Values per full node, and the byte size of a full leaf / of an internal node.
		static constexpr size_type node_slots() { return kNodeSlots; }
		static constexpr size_type leaf_node_bytes() { return LeafSize(kNodeSlots); }
		static constexpr size_type internal_node_bytes() { return InternalSize(); }

		// Checks the structure of the tree without tracing (Abseil's verify()).
		bool validate() const {
			if (root_ == nullptr || rightmost_ == nullptr || root_->parent == nullptr) return false;
			if (size_ == 0) return root_ == EmptyNode() && rightmost_ == EmptyNode();
			size_type seen = 0;
			if (!internal_verify(root_, nullptr, nullptr, seen) || seen != size_) return false;
			// leftmost() is the first leaf, rightmost() the last.
			const Node* first = root_;
			while (is_internal_node(first)) first = children(first)[0];
			const Node* last = root_;
			while (is_internal_node(last)) last = children(last)[last->finish];
			return root_->parent == first && rightmost_ == last;
		}

		// Writes the shape of the tree: a node is "[...]" holding its values, and
		// an internal node shows the subtree between each pair of values, e.g. "[[1 2] 3 [4 5]]".
		template <class Stream>
		void print(Stream& out) const {
			if (size_ == 0) { out << "[]"; return; }
			print_node(out, root_);
		}

	private:
		// ---- raw node memory ---------------------------------------------------------
		static T* slots(Node* n) { return reinterpret_cast<T*>(reinterpret_cast<char*>(n) + kSlotsOffset); }
		static const T* slots(const Node* n) { return reinterpret_cast<const T*>(reinterpret_cast<const char*>(n) + kSlotsOffset); }
		static Node** children(Node* n) { return reinterpret_cast<Node**>(reinterpret_cast<char*>(n) + kChildrenOffset); }
		static Node* const* children(const Node* n) { return reinterpret_cast<Node* const*>(reinterpret_cast<const char*>(n) + kChildrenOffset); }
		static bool is_internal_node(const Node* n) { return n->max_count == kInternalNodeMaxCount; }

		// ---- node accessors (btree_node getters/setters) ---------------------
		bool isLeaf(Node* n) const { return n->max_count != kInternalNodeMaxCount; }
		bool isInternal(Node* n) const { return !isLeaf(n); }
		field_type positionOf(Node* n) const { return n->position; }
		field_type startOf(Node* n) const { (void)n; return 0; } // always 0: not stored, never read
		field_type finishOf(Node* n) const { return n->finish; }
		field_type countOf(Node* n) const { return static_cast<field_type>(finishOf(n) - startOf(n)); }
		field_type maxCountOf(Node* n) const {
			return n->max_count == kInternalNodeMaxCount ? static_cast<field_type>(kNodeSlots) : n->max_count;
		}
		Node* parentOf(Node* n) const { return n->parent; }
		// The parent of the root is the leftmost node, which is always a leaf.
		bool isRoot(Node* n) const { return isLeaf(parentOf(n)); }
		void set_parent(Node* n, Node* p) { n->parent = p; }
		void set_position(Node* n, field_type v) { n->position = v; }
		void set_start(Node* n, field_type v) { n->start = v; }
		void set_finish(Node* n, field_type v) { n->finish = v; }
		void set_max_count(Node* n, field_type v) { n->max_count = v; }
		void decrement_finish(Node* n) { --n->finish; }
		void make_root(Node* n) { set_parent(n, parentOf(parentOf(n))); }

		T* slotAt(Node* n, size_type i) const { return slots(n) + i; }
		const T& keyAt(Node* n, size_type i) const {
			T* s = slotAt(n, i);
			return *s;
		}
		// Child slots exist only in internal nodes, which are allocated with room for
		// them; after inlining GCC cannot tell that the leaf and internal paths are
		// exclusive and reports -Warray-bounds for a leaf-sized allocation.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
		Node* childAt(Node* n, size_type i) const {
			Node* const* slot = children(n) + i;
			return *slot;
		}
		Node* start_child(Node* n) const { return childAt(n, startOf(n)); }
		void set_child_noupdate_position(Node* n, size_type i, Node* c) {
			Node** slot = children(n) + i;
			*slot = c;
		}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
		void set_child(Node* n, size_type i, Node* c) {
			set_child_noupdate_position(n, i, c);
			set_position(c, static_cast<field_type>(i));
		}
		void init_child(Node* n, size_type i, Node* c) {
			set_child(n, i, c);
			set_parent(c, n);
		}

		// ---- value movement (slots in [start, finish) are live) ---------------------
		template <class U>
		void value_init(Node* n, size_type i, U&& v) {
			T* s = slotAt(n, i);
			::new (static_cast<void*>(s)) T(std::forward<U>(v));
		}
		void value_destroy(Node* n, size_type i) { slots(n)[i].~T(); }
		void value_destroy_n(Node* n, size_type i, size_type cnt) {
			for (size_type k = 0; k < cnt; ++k) slots(n)[i + k].~T();
		}
		// Moves one value from slot src_i of src_node to slot dest_i of dest_node.
		void transfer(Node* dest_node, size_type dest_i, Node* src_node, size_type src_i) {
			T* d = slotAt(dest_node, dest_i);
			T* s = slotAt(src_node, src_i);
			::new (static_cast<void*>(d)) T(std::move(*s));
			s->~T();
		}
		// Moves `n` values.
		void transfer_n(Node* dest_node, size_type n, size_type dest_i, size_type src_i, Node* src_node) {
			T* d = slotAt(dest_node, dest_i);
			T* s = slotAt(src_node, src_i);
			for (size_type k = 0; k < n; ++k) {
				::new (static_cast<void*>(d + k)) T(std::move(s[k]));
				s[k].~T();
			}
		}
		// Same, from the end towards the beginning (for overlapping ranges moving right).
		void transfer_n_backward(Node* dest_node, size_type n, size_type dest_i, size_type src_i, Node* src_node) {
			T* d = slotAt(dest_node, dest_i + n);
			T* s = slotAt(src_node, src_i + n);
			for (size_type k = 0; k < n; ++k) {
				::new (static_cast<void*>(d - 1 - k)) T(std::move(*(s - 1 - k)));
				(s - 1 - k)->~T();
			}
		}

		// ---- node allocation (btree_node::init_*, btree::new_*_node) -------------
		Node* allocate(size_type size) {
			return static_cast<Node*>(Allocator::malloc(size));
		}
		void deallocate(size_type /*size*/, Node* node) {
			Allocator::free(node);
		}
		void init_leaf(Node* n, field_type position, field_type max_count, Node* parent) {
			set_parent(n, parent);
			set_position(n, position);
			set_start(n, 0);
			set_finish(n, 0);
			set_max_count(n, max_count);
		}
		void init_internal(Node* n, field_type position, Node* parent) {
			init_leaf(n, position, static_cast<field_type>(kNodeSlots), parent);
			// `max_count` is a sentinel value that marks the node as internal.
			set_max_count(n, kInternalNodeMaxCount);
		}
		Node* new_internal_node(field_type position, Node* parent) {
			Node* n = allocate(InternalSize());
			init_internal(n, position, parent);
			return n;
		}
		Node* new_leaf_node(field_type position, Node* parent) {
			Node* n = allocate(LeafSize(kNodeSlots));
			init_leaf(n, position, static_cast<field_type>(kNodeSlots), parent);
			return n;
		}
		Node* new_leaf_root_node(field_type max_count) {
			Node* n = allocate(LeafSize(max_count));
			init_leaf(n, 0, max_count, /*parent=*/n);
			return n;
		}

		// ---- btree_node::emplace_value / remove_values -----------------------------------
		// Emplaces a value at position i, shifting all existing values and
		// children at positions >= i to the right by 1.
		template <class U>
		void emplace_value(Node* n, field_type i, U&& v) {
			// Shift old values to create space for new value and then construct it in place.
			if (i < finishOf(n)) {
				transfer_n_backward(n, finishOf(n) - i, i + 1, i, n);
			}
			value_init(n, i, std::forward<U>(v));
			set_finish(n, static_cast<field_type>(finishOf(n) + 1));

			if (isInternal(n) && finishOf(n) > i + 1) {
				for (field_type j = finishOf(n); j > i + 1; --j) {
					set_child(n, j, childAt(n, j - 1));
				}
			}
		}

		// Removes the values at positions [i, i + to_erase), shifting all existing
		// values and children after that range to the left by to_erase. Deletes the
		// children between [i, i + to_erase).
		void remove_values(Node* n, field_type i, field_type to_erase) {
			// Transfer values after the removed range into their new places.
			value_destroy_n(n, i, to_erase);
			const field_type orig_finish = finishOf(n);
			const field_type src_i = static_cast<field_type>(i + to_erase);
			transfer_n(n, orig_finish - src_i, i, src_i, n);

			if (isInternal(n)) {
				// Delete all children between begin and end.
				for (field_type j = 0; j < to_erase; ++j) {
					clear_and_delete(childAt(n, i + j + 1));
				}
				// Rotate children after end into new positions.
				for (field_type j = static_cast<field_type>(i + to_erase + 1); j <= orig_finish; ++j) {
					set_child(n, j - to_erase, childAt(n, j));
				}
			}
			set_finish(n, static_cast<field_type>(orig_finish - to_erase));
		}

		// Rebalances a node with its right sibling.
		void rebalance_right_to_left(Node* n, field_type to_move, Node* right) {
			// 1) Move the delimiting value in the parent to the left node.
			transfer(n, finishOf(n), parentOf(n), positionOf(n));

			// 2) Move the (to_move - 1) values from the right node to the left node.
			transfer_n(n, to_move - 1, finishOf(n) + 1, startOf(right), right);

			// 3) Move the new delimiting value to the parent from the right node.
			transfer(parentOf(n), positionOf(n), right, startOf(right) + to_move - 1);

			// 4) Shift the values in the right node to their correct positions.
			transfer_n(right, countOf(right) - to_move, startOf(right), startOf(right) + to_move, right);

			if (isInternal(n)) {
				// Move the child pointers from the right to the left node.
				for (field_type i = 0; i < to_move; ++i) {
					init_child(n, finishOf(n) + i + 1, childAt(right, i));
				}
				for (field_type i = startOf(right); i <= finishOf(right) - to_move; ++i) {
					init_child(right, i, childAt(right, i + to_move));
				}
			}

			// Fixup `finish` on the left and right nodes.
			set_finish(n, static_cast<field_type>(finishOf(n) + to_move));
			set_finish(right, static_cast<field_type>(finishOf(right) - to_move));
		}

		void rebalance_left_to_right(Node* n, field_type to_move, Node* right) {
			// Values in the right node are shifted to the right to make room for the
			// new to_move values. Then, the delimiting value in the parent and the
			// other (to_move - 1) values in the left node are moved into the right node.
			// Lastly, a new delimiting value is moved from the left node into the
			// parent, and the remaining empty left node entries are destroyed.

			// 1) Shift existing values in the right node to their correct positions.
			transfer_n_backward(right, countOf(right), startOf(right) + to_move, startOf(right), right);

			// 2) Move the delimiting value in the parent to the right node.
			transfer(right, startOf(right) + to_move - 1, parentOf(n), positionOf(n));

			// 3) Move the (to_move - 1) values from the left node to the right node.
			transfer_n(right, to_move - 1, startOf(right), finishOf(n) - (to_move - 1), n);

			// 4) Move the new delimiting value to the parent from the left node.
			transfer(parentOf(n), positionOf(n), n, finishOf(n) - to_move);

			if (isInternal(n)) {
				// Move the child pointers from the left to the right node.
				for (field_type i = static_cast<field_type>(finishOf(right) + 1); i > startOf(right); --i) {
					init_child(right, i - 1 + to_move, childAt(right, i - 1));
				}
				for (field_type i = 1; i <= to_move; ++i) {
					init_child(right, i - 1, childAt(n, finishOf(n) - to_move + i));
				}
			}

			// Fixup the counts on the left and right nodes.
			set_finish(n, static_cast<field_type>(finishOf(n) - to_move));
			set_finish(right, static_cast<field_type>(finishOf(right) + to_move));
		}

		// Splits a node, moving a portion of the node's values to its right sibling.
		void split(Node* n, int insert_position, Node* dest) {
			// We bias the split based on the position being inserted. If we're
			// inserting at the beginning of the left node then bias the split to put
			// more values on the right node. If we're inserting at the end of the
			// right node then bias the split to put more values on the left node.
			if (insert_position == static_cast<int>(startOf(n))) {
				set_finish(dest, static_cast<field_type>(startOf(dest) + finishOf(n) - 1));
			} else if (insert_position == static_cast<int>(kNodeSlots)) {
				set_finish(dest, startOf(dest));
			} else {
				set_finish(dest, static_cast<field_type>(startOf(dest) + countOf(n) / 2));
			}
			set_finish(n, static_cast<field_type>(finishOf(n) - countOf(dest)));

			// Move values from the left sibling to the right sibling.
			transfer_n(dest, countOf(dest), startOf(dest), finishOf(n), n);

			// The split key is the largest value in the left sibling.
			decrement_finish(n);
			transfer_to_new_slot(parentOf(n), positionOf(n), n, finishOf(n));
			set_child_noupdate_position(parentOf(n), positionOf(n) + 1, dest);

			if (isInternal(n)) {
				for (field_type i = startOf(dest), j = static_cast<field_type>(finishOf(n) + 1);
					 i <= finishOf(dest); ++i, ++j) {
					init_child(dest, i, childAt(n, j));
				}
			}
		}
		// parent()->emplace_value(position(), alloc, finish_slot()); value_destroy(finish()):
		// the split key moves up into the parent.
		void transfer_to_new_slot(Node* parent, field_type pos, Node* from, field_type from_i) {
			T* src = slotAt(from, from_i);
			emplace_value(parent, pos, std::move(*src));
			value_destroy(from, from_i);
		}

		// Merges a node with its right sibling, moving all of the values and the
		// delimiting key in the parent node onto itself, and deleting the src node.
		void merge(Node* n, Node* src) {
			// Move the delimiting value to the left node.
			T* delimiter = slotAt(parentOf(n), positionOf(n));
			value_init(n, finishOf(n), std::move(*delimiter));

			// Move the values from the right to the left node.
			transfer_n(n, countOf(src), finishOf(n) + 1, startOf(src), src);

			if (isInternal(n)) {
				// Move the child pointers from the right to the left node.
				for (field_type i = startOf(src), j = static_cast<field_type>(finishOf(n) + 1);
					 i <= finishOf(src); ++i, ++j) {
					init_child(n, j, childAt(src, i));
				}
			}

			// Fixup `finish` on the src and dest nodes.
			set_finish(n, static_cast<field_type>(startOf(n) + 1 + countOf(n) + countOf(src)));
			set_finish(src, startOf(src));

			// Remove the value on the parent node and delete the src node.
			remove_values(parentOf(n), positionOf(n), 1);
		}

		// Deletes a node and all of its children.
		// Copies `from`, a node of `other`, and its subtree, as child `position` of `parent`
		// (nullptr: the root). Every value is copied once. `first_leaf` and `last_leaf` receive
		// the copies of the leftmost and rightmost leaves. On an exception, what was built is freed.
		Node* clone_node(const BTree& other, Node* from, Node* parent, field_type position,
		                 Node*& first_leaf, Node*& last_leaf) {
			const bool internal = other.isInternal(from);
			const field_type count = other.finishOf(from);
			Node* n = internal ? new_internal_node(position, parent)
				: parent == nullptr ? new_leaf_root_node(other.maxCountOf(from))
				: new_leaf_node(position, parent);
			field_type copied = 0;
			field_type children_copied = 0;
			DDTR_TRY {
				for (; copied < count; ++copied) {
					value_init(n, copied, other.keyAt(from, copied));
				}
				set_finish(n, count);
				if (internal) {
					for (; children_copied <= count; ++children_copied) {
						Node* child = clone_node(other, other.childAt(from, children_copied), n,
						                         children_copied, first_leaf, last_leaf);
						set_child_noupdate_position(n, children_copied, child);
					}
				} else {
					if (first_leaf == nullptr) first_leaf = n;
					last_leaf = n;
				}
			} DDTR_CATCH_ALL {
				for (field_type i = 0; i < children_copied; ++i) {
					clear_and_delete(children(n)[i]);
				}
				value_destroy_n(n, 0, copied);
				deallocate(internal ? InternalSize() : LeafSize(maxCountOf(n)), n);
				DDTR_RETHROW();
			}
			return n;
		}

		void clear_and_delete(Node* node) {
			if (isLeaf(node)) {
				value_destroy_n(node, startOf(node), countOf(node));
				deallocate(LeafSize(maxCountOf(node)), node);
				return;
			}
			if (countOf(node) == 0) {
				deallocate(InternalSize(), node);
				return;
			}

			// The parent of the root of the subtree we are deleting.
			Node* delete_root_parent = parentOf(node);

			// Navigate to the leftmost leaf under node, and then delete upwards.
			while (isInternal(node)) node = start_child(node);
			// Use `size_type` because `pos` needs to be able to hold `kNodeSlots+1`,
			// which isn't guaranteed to be a valid `field_type`.
			size_type pos = positionOf(node);
			Node* parent = parentOf(node);
			for (;;) {
				// In each iteration of the next loop, we delete one leaf node and go right.
				do {
					node = childAt(parent, static_cast<field_type>(pos));
					if (isInternal(node)) {
						// Navigate to the leftmost leaf under node.
						while (isInternal(node)) node = start_child(node);
						pos = positionOf(node);
						parent = parentOf(node);
					}
					value_destroy_n(node, startOf(node), countOf(node));
					deallocate(LeafSize(maxCountOf(node)), node);
					++pos;
				} while (pos <= finishOf(parent));

				// Once we've deleted all children of parent, delete parent and go up/right.
				do {
					node = parent;
					pos = positionOf(node);
					parent = parentOf(node);
					value_destroy_n(node, startOf(node), countOf(node));
					deallocate(InternalSize(), node);
					if (parent == delete_root_parent) {
						return;
					}
					++pos;
				} while (pos > finishOf(parent));
			}
		}

		// ---- btree_iterator::increment_slow / decrement_slow --------------------------
		cursor increment_slow(cursor c) const {
			Node* node = c.node;
			int position = c.position;
			if (isLeaf(node)) {
				while (position == static_cast<int>(finishOf(node)) && !isRoot(node)) {
					position = positionOf(node);
					node = parentOf(node);
				}
				if (position == static_cast<int>(finishOf(node))) {
					return c;
				}
			} else {
				node = childAt(node, static_cast<field_type>(position + 1));
				while (isInternal(node)) {
					node = start_child(node);
				}
				position = startOf(node);
			}
			return cursor(node, position);
		}
		cursor decrement_slow(cursor c) const {
			Node* node = c.node;
			int position = c.position;
			if (isLeaf(node)) {
				while (position < static_cast<int>(startOf(node)) && !isRoot(node)) {
					position = positionOf(node) - 1;
					node = parentOf(node);
				}
				if (position < static_cast<int>(startOf(node))) {
					return c;
				}
			} else {
				node = childAt(node, static_cast<field_type>(position));
				while (isInternal(node)) {
					node = childAt(node, finishOf(node));
				}
				position = finishOf(node) - 1;
			}
			return cursor(node, position);
		}
		// btree_iterator::increment_n_slow (n > 0)
		cursor increment_n_slow(cursor c, difference_type n) const {
			Node* node = c.node;
			int position = c.position;
			while (n > 0) {
				if (isLeaf(node)) {
					if (position + n < static_cast<difference_type>(finishOf(node))) {
						position += static_cast<int>(n);
						break;
					} else {
						n -= finishOf(node) - position;
						position = finishOf(node);
						cursor save(node, position);
						while (position == static_cast<int>(finishOf(node)) && !isRoot(node)) {
							position = positionOf(node);
							node = parentOf(node);
						}
						if (position == static_cast<int>(finishOf(node))) {
							return save;
						}
					}
				} else {
					--n;
					node = childAt(node, static_cast<field_type>(position + 1));
					while (isInternal(node)) {
						node = start_child(node);
					}
					position = startOf(node);
				}
			}
			return cursor(node, position);
		}

		// ---- btree accessors -------------------------------------------------------------------
		bool is_empty() const { return size_ == 0; }
		Node* root() const { return root_; }
		Node*& mutable_root() { return root_; }
		Node* rightmost() const { return rightmost_; }
		Node*& mutable_rightmost() { return rightmost_; }
		// The leftmost node is stored as the parent of the root node.
		Node* leftmost() const { return parentOf(root()); }

		// We use a static empty node for the root/leftmost/rightmost of empty btrees
		// in order to avoid branching in begin()/end().
		static Node* EmptyNode() {
			static Node empty = { nullptr, 0, 0, 0, static_cast<field_type>(kInternalNodeMaxCount + 1) };
			static const bool linked = (empty.parent = &empty, true);
			(void)linked;
			return &empty;
		}

		cursor internal_end(cursor iter) const {
			return iter.node != nullptr ? iter : getEnd();
		}

		// ---- in-node search (btree_node::lower_bound / upper_bound) -----------------------
		// Position of the first value whose key is not less than k (Upper == false), or
		// greater than k (Upper == true). Linear for arithmetic keys, binary otherwise.
		template <bool Upper>
		bool node_comp(const T& key, const T& k) const {
			// lower: key < k; upper (upper_bound_adapter): !(k < key)
			return Upper ? !(k < key) : (key < k);
		}
		template <bool Upper>
		size_type node_search(Node* n, const T& k) const {
			size_type s = startOf(n);
			size_type e = finishOf(n);
			if (use_linear_search) {
				while (s < e) {
					if (!node_comp<Upper>(keyAt(n, s), k)) {
						break;
					}
					++s;
				}
				return s;
			}
			while (s != e) {
				const size_type mid = (s + e) >> 1;
				if (node_comp<Upper>(keyAt(n, mid), k)) {
					s = mid + 1;
				} else {
					e = mid;
				}
			}
			return s;
		}

		// ---- btree::insert_multi / internal_emplace / rebalance_or_split ------------------------
        // Abseil btree::insert_unique, without key-compare-to: locate once,
        // check the lower-bound candidate, then emplace at the saved leaf slot.
        std::pair<cursor, bool> internal_insert_unique(const T& value) {
            if (is_empty()) {
                mutable_root() = mutable_rightmost() = new_leaf_root_node(1);
            }
            cursor iter = internal_locate(value);
            cursor last = internal_last(iter);
            // lower_bound already established !(last.key < value).
            if (last.node != nullptr && !(value < keyAt(last.node, last.position))) {
                return {last, false};
            }
            T protected_value(value); // internal_emplace can relocate slots
            return {internal_emplace(iter, protected_value), true};
        }

		cursor insert_multi(const T& key, T& v) {
			if (is_empty()) {
				mutable_root() = mutable_rightmost() = new_leaf_root_node(1);
			}
			cursor iter = internal_upper_bound(key);
			if (iter.node == nullptr) {
				iter = getEnd();
			}
			return internal_emplace(iter, v);
		}

		cursor internal_emplace(cursor iter, T& v) {
			if (isInternal(iter.node)) {
				// We can't insert on an internal node. Instead, we'll insert after the
				// previous value which is guaranteed to be on a leaf node.
				iter = decrement(iter);
				++iter.position;
			}
			const field_type max_count = maxCountOf(iter.node);

			if (countOf(iter.node) == max_count) {
				// Make room in the leaf for the new item.
				if (max_count < kNodeSlots) {
					// Insertion into the root where the root is smaller than the full node
					// size. Simply grow the size of the root node.
					replace_leaf_root_node(iter, static_cast<field_type>((std::min)(static_cast<int>(kNodeSlots), 2 * max_count)));
				} else {
					rebalance_or_split(&iter);
				}
			}
			// v is the protected local copy, no longer needed after insertion.
			emplace_value(iter.node, static_cast<field_type>(iter.position), std::move_if_noexcept(v));
			++size_;
			return iter;
		}

		// Replaces the leaf root by a bigger one holding the same values.
		void replace_leaf_root_node(cursor& iter, field_type new_node_size) {
			Node* old_root = iter.node;
			Node* new_root = iter.node = new_leaf_root_node(new_node_size);
			// transfer_and_delete
			transfer_n(new_root, countOf(old_root), startOf(new_root), startOf(old_root), old_root);
			set_finish(new_root, finishOf(old_root));
			set_finish(old_root, startOf(old_root));
			clear_and_delete(old_root);
			mutable_root() = mutable_rightmost() = new_root;
		}

		// Rebalances or splits the node iter points to.
		void rebalance_or_split(cursor* iter) {
			Node*& node = iter->node;
			int& insert_position = iter->position;

			// First try to make room on the node by rebalancing.
			Node* parent = parentOf(node);
			if (node != root()) {
				if (positionOf(node) > startOf(parent)) {
					// Try rebalancing with our left sibling.
					Node* left = childAt(parent, positionOf(node) - 1);
					if (countOf(left) < kNodeSlots) {
						// We bias rebalancing based on the position being inserted. If we're
						// inserting at the end of the right node then we bias rebalancing to
						// fill up the left node.
						field_type to_move = static_cast<field_type>((kNodeSlots - countOf(left)) /
							(1 + (static_cast<field_type>(insert_position) < kNodeSlots)));
						to_move = (std::max)(field_type{1}, to_move);

						if (static_cast<field_type>(insert_position) - to_move >= startOf(node) ||
							countOf(left) + to_move < kNodeSlots) {
							rebalance_right_to_left(left, to_move, node);

							insert_position = static_cast<int>(static_cast<field_type>(insert_position) - to_move);
							if (insert_position < static_cast<int>(startOf(node))) {
								insert_position = insert_position + countOf(left) + 1;
								node = left;
							}
							return;
						}
					}
				}

				if (positionOf(node) < finishOf(parent)) {
					// Try rebalancing with our right sibling.
					Node* right = childAt(parent, positionOf(node) + 1);
					if (countOf(right) < kNodeSlots) {
						// We bias rebalancing based on the position being inserted. If we're
						// inserting at the beginning of the left node then we bias rebalancing
						// to fill up the right node.
						field_type to_move = static_cast<field_type>((kNodeSlots - countOf(right)) /
							(1 + (insert_position > static_cast<int>(startOf(node)))));
						to_move = (std::max)(field_type{1}, to_move);

						if (static_cast<field_type>(insert_position) <= finishOf(node) - to_move ||
							countOf(right) + to_move < kNodeSlots) {
							rebalance_left_to_right(node, to_move, right);

							if (insert_position > static_cast<int>(finishOf(node))) {
								insert_position = insert_position - countOf(node) - 1;
								node = right;
							}
							return;
						}
					}
				}

				// Rebalancing failed, make sure there is room on the parent node for a new value.
				if (countOf(parent) == kNodeSlots) {
					cursor parent_iter(parent, positionOf(node));
					rebalance_or_split(&parent_iter);
					parent = parentOf(node);
				}
			} else {
				// Rebalancing not possible because this is the root node.
				// Create a new root node and set the current root node as the child of the new root.
				parent = new_internal_node(/*position=*/0, parent);
				init_child(parent, startOf(parent), node);
				mutable_root() = parent;
			}

			// Split the node.
			Node* split_node;
			if (isLeaf(node)) {
				split_node = new_leaf_node(static_cast<field_type>(positionOf(node) + 1), parent);
				split(node, insert_position, split_node);
				if (rightmost() == node) mutable_rightmost() = split_node;
			} else {
				split_node = new_internal_node(static_cast<field_type>(positionOf(node) + 1), parent);
				split(node, insert_position, split_node);
			}

			if (insert_position > static_cast<int>(finishOf(node))) {
				insert_position = insert_position - countOf(node) - 1;
				node = split_node;
			}
		}

		// ---- btree::erase / rebalance_after_delete / try_merge_or_rebalance / try_shrink ------
		// Erases the value at `iter`; returns the one after it (or end()).
		cursor erase(cursor iter) {
			value_destroy(iter.node, iter.position);

			const bool internal_delete = isInternal(iter.node);
			if (internal_delete) {
				// Deletion of a value on an internal node. First, transfer the largest
				// value from our left child here, then erase/rebalance from that position.
				// We can get to the largest value from our left child by decrementing iter.
				cursor internal_iter(iter);
				iter = decrement(iter);
				transfer(internal_iter.node, internal_iter.position, iter.node, iter.position);
			} else {
				// Shift values after erased position in leaf. In the internal case, we
				// don't need to do this because the leaf position is the end of the node.
				const field_type transfer_from = static_cast<field_type>(iter.position + 1);
				const field_type num_to_transfer = static_cast<field_type>(finishOf(iter.node) - transfer_from);
				transfer_n(iter.node, num_to_transfer, iter.position, transfer_from, iter.node);
			}
			// Update node finish and container size.
			decrement_finish(iter.node);
			--size_;

			// We want to return the next value after the one we just erased. If we
			// erased from an internal node (internal_delete == true), then the next
			// value is ++(++iter). If we erased from a leaf node (internal_delete ==
			// false) then the next value is ++iter. Note that ++iter may point to an
			// internal node and the value in the internal node may move to a leaf node
			// (iter.node) when rebalancing is performed at the leaf level.
			cursor res = rebalance_after_delete(iter);

			// If we erased from an internal node, advance the iterator.
			if (internal_delete) {
				res = increment(res);
			}
			return res;
		}

		cursor rebalance_after_delete(cursor iter) {
			// Merge/rebalance as we walk back up the tree.
			cursor res(iter);
			bool first_iteration = true;
			for (;;) {
				if (iter.node == root()) {
					try_shrink();
					if (is_empty()) {
						return getEnd();
					}
					break;
				}
				if (countOf(iter.node) >= kMinNodeValues) {
					break;
				}
				bool merged = try_merge_or_rebalance(&iter);
				// On the first iteration, we should update `res` with `iter` because `res`
				// may have been invalidated.
				if (first_iteration) {
					res = iter;
					first_iteration = false;
				}
				if (!merged) {
					break;
				}
				iter.position = positionOf(iter.node);
				iter.node = parentOf(iter.node);
			}

			// Adjust our return value. If we're pointing at the end of a node, advance the iterator.
			if (res.position == static_cast<int>(finishOf(res.node))) {
				res.position = finishOf(res.node) - 1;
				res = increment(res);
			}
			return res;
		}

		// Merges the values of left, right and the delimiting key on their parent
		// onto left, removing the delimiting key and deleting right.
		void merge_nodes(Node* left, Node* right) {
			merge(left, right);
			if (rightmost() == right) mutable_rightmost() = left;
		}

		// Tries to merge node with its left or right sibling, and failing that,
		// rebalance with its left or right sibling. Returns true if a merge
		// occurred, at which point it is no longer valid to access node. Returns
		// false if no merging took place.
		bool try_merge_or_rebalance(cursor* iter) {
			Node* parent = parentOf(iter->node);
			if (positionOf(iter->node) > startOf(parent)) {
				// Try merging with our left sibling.
				Node* left = childAt(parent, positionOf(iter->node) - 1);
				if (1U + countOf(left) + countOf(iter->node) <= kNodeSlots) {
					iter->position += 1 + countOf(left);
					merge_nodes(left, iter->node);
					iter->node = left;
					return true;
				}
			}
			if (positionOf(iter->node) < finishOf(parent)) {
				// Try merging with our right sibling.
				Node* right = childAt(parent, positionOf(iter->node) + 1);
				if (1U + countOf(iter->node) + countOf(right) <= kNodeSlots) {
					merge_nodes(iter->node, right);
					return true;
				}
				// Try rebalancing with our right sibling. We don't perform rebalancing if
				// we deleted the first element from iter->node and the node is not
				// empty. This is a small optimization for the common pattern of deleting
				// from the front of the tree.
				if (countOf(right) > kMinNodeValues &&
					(countOf(iter->node) == 0 || iter->position > static_cast<int>(startOf(iter->node)))) {
					field_type to_move = static_cast<field_type>((countOf(right) - countOf(iter->node)) / 2);
					to_move = (std::min)(to_move, static_cast<field_type>(countOf(right) - 1));
					rebalance_right_to_left(iter->node, to_move, right);
					return false;
				}
			}
			if (positionOf(iter->node) > startOf(parent)) {
				// Try rebalancing with our left sibling. We don't perform rebalancing if
				// we deleted the last element from iter->node and the node is not
				// empty. This is a small optimization for the common pattern of deleting
				// from the back of the tree.
				Node* left = childAt(parent, positionOf(iter->node) - 1);
				if (countOf(left) > kMinNodeValues &&
					(countOf(iter->node) == 0 || iter->position < static_cast<int>(finishOf(iter->node)))) {
					field_type to_move = static_cast<field_type>((countOf(left) - countOf(iter->node)) / 2);
					to_move = (std::min)(to_move, static_cast<field_type>(countOf(left) - 1));
					rebalance_left_to_right(left, to_move, iter->node);
					iter->position += to_move;
					return false;
				}
			}
			return false;
		}

		// Tries to shrink the height of the tree by 1.
		void try_shrink() {
			Node* orig_root = root();
			if (countOf(orig_root) > 0) {
				return;
			}
			// Deleted the last item on the root node, shrink the height of the tree.
			if (isLeaf(orig_root)) {
				mutable_root() = mutable_rightmost() = EmptyNode();
			} else {
				Node* child = start_child(orig_root);
				make_root(child);
				mutable_root() = child;
			}
			clear_and_delete(orig_root);
		}

		// ---- btree::internal_* searches -----------------------------------------------------------
		// Returns an iterator pointing to the first value >= the value "iter" is
		// pointing at, moving up the tree as needed. The result's node is null when
		// there is no such value.
		cursor internal_last(cursor iter) const {
			while (iter.position == static_cast<int>(finishOf(iter.node))) {
				iter.position = positionOf(iter.node);
				iter.node = parentOf(iter.node);
				if (isLeaf(iter.node)) {
					iter.node = nullptr;
					break;
				}
			}
			return iter;
		}

		// First value whose key is not less than `key`, or a null node.
		cursor internal_lower_bound(const T& key) const {
			Node* root_node = root();
			cursor iter(root_node, startOf(root_node));
			for (;;) {
				iter.position = static_cast<int>(node_search<false>(iter.node, key));
				if (isLeaf(iter.node)) {
					break;
				}
				iter.node = childAt(iter.node, static_cast<field_type>(iter.position));
			}
			return internal_last(iter);
		}

		// First value whose key is greater than `key`, or a null node.
		cursor internal_upper_bound(const T& key) const {
			Node* root_node = root();
			cursor iter(root_node, startOf(root_node));
			for (;;) {
				iter.position = static_cast<int>(node_search<true>(iter.node, key));
				if (isLeaf(iter.node)) {
					break;
				}
				iter.node = childAt(iter.node, static_cast<field_type>(iter.position));
			}
			return internal_last(iter);
		}

		// Leaf position at which key would reside (btree::internal_locate).
		cursor internal_locate(const T& key) const {
			Node* root_node = root();
			cursor iter(root_node, startOf(root_node));
			for (;;) {
				iter.position = static_cast<int>(node_search<false>(iter.node, key));
				// Without a three-way comparator we cannot stop early on equal keys.
				if (isLeaf(iter.node)) {
					break;
				}
				iter.node = childAt(iter.node, static_cast<field_type>(iter.position));
			}
			return iter;
		}

		// A value equivalent to `key`, or a null node (btree::internal_find).
		cursor internal_find(const T& key) const {
			cursor iter = internal_last(internal_locate(key));
			if (iter.node != nullptr && !(key < keyAt(iter.node, iter.position))) {
				return iter;
			}
			return cursor(nullptr, 0);
		}

		// ---- rank access -----------------------------------------------------------------------------
		pointer payload_at(size_type n) const {
			if (n >= size_) {
				DDTR_THROW(std::out_of_range("ddtr b-tree: rank out of range"));
			}
			cursor c = increment_n_slow(getFirstElement(), static_cast<difference_type>(n));
			return &slots(c.node)[c.position];
		}
		cursor getFirstElementChecked() const {
			if (is_empty()) DDTR_THROW(std::out_of_range("ddtr b-tree: empty"));
			return getFirstElement();
		}
		cursor getLastElementChecked() const {
			if (is_empty()) DDTR_THROW(std::out_of_range("ddtr b-tree: empty"));
			return getLastElement();
		}
		pointer payload_of(cursor c) const {
			return &slots(c.node)[c.position];
		}

		// ---- diagnostics ---------------------------------------------------------------------------
		std::pair<size_type, size_type> internal_stats(const Node* node) const {
			// The root can be a static empty node.
			if (node == nullptr || (node == root_ && size_ == 0)) return {0, 0};
			if (!is_internal_node(node)) return {1, 0};
			std::pair<size_type, size_type> res(0, 1);
			for (size_type i = 0; i <= node->finish; ++i) {
				auto sub = internal_stats(children(node)[i]);
				res.first += sub.first;
				res.second += sub.second;
			}
			return res;
		}
		template <class Stream>
		void print_node(Stream& out, const Node* node) const {
			out << '[';
			for (size_type i = 0; i <= node->finish; ++i) {
				if (is_internal_node(node)) {
					if (i > 0) out << ' ';
					print_node(out, children(node)[i]);
				}
				if (i < node->finish) {
					if (i > 0 || is_internal_node(node)) out << ' ';
					out << slots(node)[i];
				}
			}
			out << ']';
		}
		// Abseil's internal_verify: counts, order, parent links and positions.
		bool internal_verify(const Node* node, const T* lo, const T* hi, size_type& count) const {
			if (node->finish == 0) return false;
			const size_type max = is_internal_node(node) ? kNodeSlots : node->max_count;
			if (node->finish > max) return false;
			if (lo != nullptr && slots(node)[0] < *lo) return false;
			if (hi != nullptr && *hi < slots(node)[node->finish - 1]) return false;
			for (size_type i = 1; i < node->finish; ++i)
				if (slots(node)[i] < slots(node)[i - 1]) return false;
			count += node->finish;
			if (is_internal_node(node)) {
				for (size_type i = 0; i <= node->finish; ++i) {
					const Node* c = children(node)[i];
					if (c == nullptr || c->parent != node || c->position != i) return false;
					if (!internal_verify(c, i == 0 ? lo : &slots(node)[i - 1],
										 i == node->finish ? hi : &slots(node)[i], count)) return false;
				}
			}
			return true;
		}

		Node* root_;
		// A pointer to the rightmost node. The leftmost node is stored as the parent of the root.
		Node* rightmost_;
		// Number of values.
		size_type size_;
	};
}
////////////////////////////////////////////////////////////////////////////////
// End namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
#endif
