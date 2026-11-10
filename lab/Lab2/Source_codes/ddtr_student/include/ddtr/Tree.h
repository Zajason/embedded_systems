#include "detail/exceptions.hpp"
#ifndef TREE_H
#define TREE_H

#include "DDTIncludes.h"
////////////////////////////////////////////////////////////////////////////////
// Begin namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
namespace DDTLibrary {

	template <class treeType> class Tree;

	template <class treeType> class RedBlackTreeExtension;
	template <class treeType> class AVLTreeExtension;
	template <class elementType> class BinaryTree;

	template <class Derived> struct TreeLinks;
	template <class Derived, class T, class A, class Links = TreeLinks<Derived>> class TreeNodeCore;
	template <class T, class A = ddtr::malloc_allocator> class BinaryTreeNode;
	template <class T, class A = ddtr::malloc_allocator> class RedBlackTreeNode;
	template <class T, class A = ddtr::malloc_allocator> class AVLTreeNode;

	///////////////////////////////////////////////////////////////
	// Tree Nodes
	///////////////////////////////////////////////////////////////
	// Links, rank and balance field precede _data, so the small fields share one padded
	// slot. The empty constructors make these non-POD: the Itanium ABI then lets the next
	// field reuse their tail padding. Do not default them.
	template <class Derived>
	struct TreeLinks {
		Derived *_left, *_right, *_parent;
		#ifdef DDTR_TREE_RANKS
			std::uint32_t _subtreeSize;
		#endif
		TreeLinks() {}
	};
	template <class Derived>
	struct RedBlackLinks : TreeLinks<Derived> {
		short _color;
		RedBlackLinks() {}
	};
	template <class Derived>
	struct AVLLinks : TreeLinks<Derived> {
		int _height;
		AVLLinks() {}
	};

	template <class Derived, class T, class A, class Links>
	class TreeNodeCore : public allocated<A>, public Links {
	public:
		typedef A Allocator;
		typedef T value_type;
		typedef value_type* pointer;
		typedef const value_type* const_pointer;
		typedef value_type& reference;
		typedef const value_type& const_reference;
		typedef T input_value_type;
		typedef const input_value_type& const_input_reference;
		typedef size_t size_type;
		typedef ptrdiff_t difference_type;
		typedef Derived* element_pointer;
		typedef Derived& element_reference;
		typedef T* value_pointer;

		using Links::_left;
		using Links::_right;
		using Links::_parent;
		#ifdef DDTR_TREE_RANKS
			using Links::_subtreeSize;
		#endif
		#ifndef OUT_OF_LINE_STORAGE
			union { T _data; };
		#else
			pointer _data;
		#endif

		TreeNodeCore () {
			_left = static_cast<Derived*>(this);
			_right = static_cast<Derived*>(this);
			_parent = static_cast<Derived*>(this);
			#ifdef DDTR_TREE_RANKS
				_subtreeSize = 0;
			#endif
			#ifdef OUT_OF_LINE_STORAGE
				_data = NULL;
            #else
                ::new(static_cast<void*>(std::addressof(_data))) input_value_type;
			#endif
		}
		TreeNodeCore (const_input_reference data, element_pointer sentinel, element_pointer parent) {
			#ifdef OUT_OF_LINE_STORAGE
                _data = (pointer) Allocator::malloc(sizeof(input_value_type));
            #endif
			_left = sentinel;
			_right = sentinel;
			_parent = parent;
			#ifdef DDTR_TREE_RANKS
				_subtreeSize = 1;
			#endif
			#ifndef OUT_OF_LINE_STORAGE
                Constructor(std::addressof(_data), data);
			#else
                Constructor(_data, data);
			#endif
		}
		~TreeNodeCore() {
#ifdef OUT_OF_LINE_STORAGE
            if (_data != nullptr) {
                Destructor(_data);
                Allocator::free(_data);
            }
#else
            Destructor(std::addressof(_data));
#endif
        }

		inline pointer getDataPointer() {
			#ifndef OUT_OF_LINE_STORAGE
				return &_data;
			#else
				return _data;
			#endif
		}
		inline const_reference getData() {
			#ifndef OUT_OF_LINE_STORAGE
				return _data;
			#else
                return *_data;
			#endif
		}
	};

	template <class T, class A>
	class BinaryTreeNode : public TreeNodeCore<BinaryTreeNode<T, A>, T, A> {
	public:
		typedef TreeNodeCore<BinaryTreeNode<T, A>, T, A> core;
		typedef typename core::const_input_reference const_input_reference;
		typedef typename core::element_pointer element_pointer;
		BinaryTreeNode () : core() {}
		BinaryTreeNode (const_input_reference data, element_pointer sentinel, element_pointer parent)
			: core(data, sentinel, parent) {}
	};

	template <class T, class A>
	class RedBlackTreeNode : public TreeNodeCore<RedBlackTreeNode<T, A>, T, A, RedBlackLinks<RedBlackTreeNode<T, A>>> {
	public:
		typedef TreeNodeCore<RedBlackTreeNode<T, A>, T, A, RedBlackLinks<RedBlackTreeNode<T, A>>> core;
		typedef typename core::const_input_reference const_input_reference;
		typedef typename core::element_pointer element_pointer;
		using core::_color;
		RedBlackTreeNode () : core() {
			_color = BLACK;
		}
		RedBlackTreeNode (const_input_reference data, element_pointer sentinel, element_pointer parent)
			: core(data, sentinel, parent) {
			_color = RED;
		}
	};

	template <class T, class A>
	class AVLTreeNode : public TreeNodeCore<AVLTreeNode<T, A>, T, A, AVLLinks<AVLTreeNode<T, A>>> {
	public:
		typedef TreeNodeCore<AVLTreeNode<T, A>, T, A, AVLLinks<AVLTreeNode<T, A>>> core;
		typedef typename core::const_input_reference const_input_reference;
		typedef typename core::element_pointer element_pointer;
		using core::_height;
		AVLTreeNode () : core() {
			_height = 0;
		}
		AVLTreeNode (const_input_reference data, element_pointer sentinel, element_pointer parent)
			: core(data, sentinel, parent) {
			_height = 1;
		}
	};

	///////////////////////////////////////////////////////////////
	// Tree Bases
	///////////////////////////////////////////////////////////////
	template <class elementType> 
	class BinaryTree :public allocated<typename elementType::Allocator> {
	public:
		////////////////////////////////////////////////////////////////////////
		typedef typename elementType::Allocator Allocator;
		////////////////////////////////////////////////////////////////////////
		typedef typename elementType::value_type value_type;
		typedef typename elementType::pointer pointer;
		typedef typename elementType::reference reference;
		typedef typename elementType::const_reference const_reference;

		typedef elementType BTelementType;

		typedef typename elementType::input_value_type input_value_type;
		typedef typename elementType::const_input_reference const_input_reference;

		typedef typename elementType::element_pointer element_pointer;
		typedef typename elementType::element_reference element_reference;

		typedef typename elementType::size_type size_type;
		typedef typename elementType::difference_type difference_type;

		// ---------------- Construction and destruction ---------------

		BinaryTree () {
			_sentinel = new elementType();
			_root = _sentinel;
            _leftmost = _sentinel;
            _rightmost = _sentinel;
		}
        // Transfer ownership; give the moved-from tree a fresh empty sentinel,
        // without scanning elements.
        BinaryTree(BinaryTree&& other) {
            auto fresh_nil = new elementType();
            _root = other._root;
            _sentinel = other._sentinel;
            _size = other._size;
            _leftmost = other._leftmost;
            _rightmost = other._rightmost;
            other._size = 0;
            other._root = fresh_nil;
            other._sentinel = fresh_nil;
            other._leftmost = fresh_nil;
            other._rightmost = fresh_nil;
        }
		BinaryTree (const BinaryTree &) = delete;
		BinaryTree & operator=(const BinaryTree &) = delete;
		~BinaryTree() {
			delete _sentinel;
		}
		void initialize() {
			_root = _sentinel;
			_size = 0;
            _leftmost = _sentinel;
            _rightmost = _sentinel;
		}
		inline bool IsNil(element_pointer x) const {
			return x == _sentinel;
		}
		void LeftRotate(element_pointer x) {
			element_pointer y = x->_right;
			#ifdef DDTR_TREE_RANKS
				long a = x->_left->_subtreeSize;
				long b = y->_left->_subtreeSize;
				long c = y->_right->_subtreeSize;
			#endif
			x->_right = y->_left;
			if (!IsNil(y->_left)){
				y->_left->_parent = x;
			}
			y->_parent = x->_parent;
			if (IsNil(x->_parent)) {
				_root = y;
			} else {
				if (x == x->_parent->_left) {
					x->_parent->_left = y;
				} else {
					x->_parent->_right = y;
				}
			}
			y->_left = x;
			x->_parent = y;
			#ifdef DDTR_TREE_RANKS
				x->_subtreeSize = a + b + 1;
				y->_subtreeSize = a + b + c + 2;
			#endif
		}
		void RightRotate(element_pointer x) {
			element_pointer y = x->_left;
			#ifdef DDTR_TREE_RANKS
				long c = x->_right->_subtreeSize;
				long a = y->_left->_subtreeSize;
				long b = y->_right->_subtreeSize;
			#endif
			x->_left = y->_right;
			if (!IsNil(y->_right)){
				y->_right->_parent = x;
			}
			y->_parent = x->_parent;
			if (IsNil(x->_parent)) {
				_root = y;
			} else {
				if (x == x->_parent->_right) {
					x->_parent->_right = y;
				} else {
					x->_parent->_left = y;
				}
			}
			y->_right = x;
			x->_parent = y;
			#ifdef DDTR_TREE_RANKS
				x->_subtreeSize = b + c + 1;
				y->_subtreeSize = a + b + c + 2;
			#endif
		}
		virtual element_pointer Insert(const_input_reference k){
			element_pointer y = _sentinel;
			element_pointer x = _root;
			while (!IsNil(x)) {
				y = x;
				#ifdef DDTR_TREE_RANKS
					x->_subtreeSize += 1;
				#endif
				if (k < x->getData())
					x = x->_left;
				else
					x = x->_right;
			}
			element_pointer z = new elementType(k, _sentinel, y);
			if (IsNil(y)) {
				_root = z;
			} else {
				if (k < y->getData())
					y->_left = z;
				else
					y->_right = z;
			}
            this->cacheInsertedNode(z, y);
            incrementSize();
			return z;
		}

        // Multiset insertion immediately before a valid hint. Invalid hints fall
        // back to ordinary stable (upper-bound) insertion.
        virtual void FinishHintInsert(element_pointer) {}
        element_pointer InsertHint(element_pointer hint,
                                             const_input_reference value) {
            const auto nil = _sentinel;
            auto before = IsNil(hint) ? Maximum() : Predecessor(hint);
            if ((!IsNil(hint) && hint->getData() < value) ||
                (!IsNil(before) && value < before->getData()))
                return Insert(value);
            auto parent = nil;
            bool left = false;
            if (!IsNil(hint) && IsNil(hint->_left)) {
                parent = hint;
                left = true;
            } else if (!IsNil(before)) parent = before;
            auto node = new BTelementType(value, nil, parent);
            if (IsNil(parent)) _root = node;
            else if (left) parent->_left = node;
            else parent->_right = node;
            cacheInsertedNode(node, parent);
#ifdef DDTR_TREE_RANKS
            for (auto a = parent; !IsNil(a); a = a->_parent) {
                a->_subtreeSize += 1;
            }
#endif
            incrementSize();
            FinishHintInsert(node);
            return node;
        }

        // One search descent. Do not change ancestor sizes until absence is known:
        // a duplicate is a read-only operation. Existing Insert remains a multiset.
        virtual std::pair<element_pointer, bool> InsertUnique(const_input_reference k) {
            const auto nil = _sentinel;
            auto parent = nil;
            auto node = _root;
            bool left = false;
            while (node != nil) {
                parent = node;
                const auto& key = node->getData();
                if (k < key) {
                    left = true;
                    node = node->_left;
                } else if (key < k) {
                    left = false;
                    node = node->_right;
                } else {
                    // Equivalent under operator<, irrespective of operator!=.
                    return {node, false};
                }
            }
            auto inserted = new elementType(k, nil, parent);
            if (parent == nil) {
                _root = inserted;
            } else if (left) {
                parent->_left = inserted;
            } else {
                parent->_right = inserted;
            }
            cacheInsertedNode(inserted, parent);
#ifdef DDTR_TREE_RANKS
            // This is an ancestor update, not another key-search descent.
            for (auto ancestor = parent; ancestor != nil;
                 ancestor = ancestor->_parent) {
                ancestor->_subtreeSize += 1;
            }
#endif
            incrementSize();
            return {inserted, true};
        }
		void transplantSuccessorNode(element_pointer del, element_pointer succ, element_pointer x) {
			bool directChild = (succ->_parent == del);
			succ->_left = del->_left;
			succ->_right = del->_right;
			succ->_parent = del->_parent;
			#ifdef DDTR_TREE_RANKS
				succ->_subtreeSize = del->_subtreeSize;
			#endif
			if (!IsNil(del->_left)) {
				del->_left->_parent = succ;
			}
			if (!IsNil(del->_right)) {
				del->_right->_parent = succ;
			}
			if (IsNil(del->_parent))
				_root = succ;
			else if (del == del->_parent->_left)
				del->_parent->_left = succ;
			else
				del->_parent->_right = succ;
			if (directChild) {
				x->_parent = succ;
			}
		}
		virtual element_pointer Delete(element_pointer z) {
            this->cacheErasedNode(z);
			element_pointer x, y, removed;
			if((IsNil(z->_left) || IsNil(z->_right)))
				y = z;
			else
				y = Successor(z);
			#ifdef DDTR_TREE_RANKS
				element_pointer tmp = y;
				while (!IsNil(tmp)) {
					tmp->_subtreeSize += -1;
					tmp = tmp->_parent;
				}
			#endif
			decrementSize();
			if (!IsNil(y->_left))
				x = y->_left;
			else{
				x = y->_right;
			}
			x->_parent = y->_parent;
			if (IsNil(y->_parent)){
				_root = x;
			}
			else {
				if (y == y->_parent->_left)
					y->_parent->_left = x;
				else 
					y->_parent->_right = x;
			}
			if (y != z){
				#ifndef OUT_OF_LINE_STORAGE
					this->transplantSuccessorNode(z, y, x);
					removed = z;
				#else
					auto _data_tmp = z->_data;
					z->_data = y->_data;
					y->_data = _data_tmp;
                    if (this->_rightmost == y)
                        this->_rightmost = z;
					removed = y;
				#endif
			}
			else
				removed = y;
			return removed;
		}
		element_pointer Search(const_input_reference k) {
			return TreeSearch (_root, k);
		}
		// First node whose key is >= k (in-order), or _sentinel if none.
		// Mirrors std::lower_bound / SkipList::Seek semantics. Works for
		// BST/RB/AVL (reads only _left/_right/getData).
		element_pointer LowerBoundNode(const_input_reference k) {
			element_pointer x = _root;
			element_pointer res = _sentinel;
			while (!IsNil(x)) {
				if (!(x->getData() < k)) {   // x->key >= k
					res = x;
					x = x->_left;
				} else {
					x = x->_right;
				}
			}
			return res;
		}
		inline element_pointer Minimum() const {
            return _leftmost;
		}
		inline element_pointer Maximum() const{
            return _rightmost;
		}
		pointer Smallest() const {
			if (!IsNil(_root))
				return Minimum()->getDataPointer();
			else
				return NULL;
		}
		pointer Largest() const {
			if (!IsNil(_root))
				return Maximum()->getDataPointer();
			else
				return NULL;
		};
		inline element_pointer ExtractMin() {
			return Delete(Minimum());

		}
		pointer Find(const_input_reference item) {
			element_pointer tmp = TreeSearch(_root, item);
			if (!IsNil(tmp))
				return tmp->getDataPointer();
			else
				return NULL;
		}
		element_pointer Remove(const_input_reference item) {
			element_pointer tmp = TreeSearch(_root, item);
			if (!IsNil(tmp)) {
				return Delete(tmp);
			}
			return tmp;
		}
		element_pointer Successor(element_pointer x) {
            auto child = x->_right;
            if (!IsNil(child)) return TreeMinimum(child);
            auto parent = x->_parent;
            while (!IsNil(parent)) {
                if (x != parent->_right) break;
                x = parent;
                parent = x->_parent;
            }
            return parent;
        }
		element_pointer Predecessor(element_pointer x) {
            auto child = x->_left;
            if (!IsNil(child)) return TreeMaximum(child);
            auto parent = x->_parent;
            while (!IsNil(parent)) {
                if (x != parent->_left) break;
                x = parent;
                parent = x->_parent;
            }
            return parent;
        }
		pointer ItemWithRank(size_type rank) const {
#ifdef DDTR_TREE_RANKS
            element_pointer tmp = Select(_root, rank + 1);
#else
            element_pointer tmp = Minimum();
            for (size_type i = 0; i < rank && !IsNil(tmp); ++i)
                tmp = increment(tmp);
#endif

			if (!IsNil(tmp))
				return tmp->getDataPointer();
			else
				return NULL;
		}
#ifdef DDTR_TREE_RANKS
		element_pointer Select (element_pointer x, size_type i) const {
			element_pointer left = x->_left;
			long r = left->_subtreeSize + 1;
			if (i > x->_subtreeSize)
				return _sentinel;
			else if (i == r)
				return x;
			else if (i < r)
				return Select(left, i);
			else
				return Select(x->_right, i-r);
		}
#endif
		void Print() {
			std::cout << "Printing Tree Nodes :" << std::endl;
			InorderTreeWalk(_root);
			std::cout << std::endl;
		}
		inline element_pointer Root() {
			return _root;
		}
		inline element_pointer Sentinel() {
			return _sentinel;
		}
		inline element_pointer getFirstElement() const {
            return _leftmost;
		}
		inline element_pointer getLastElement() const {
            return _rightmost;
		}
		inline element_pointer getEnd() const {
			return _sentinel;
		}
		////////////////////////////////////////////////////////////////////////
		// Element access used by the iterators
		////////////////////////////////////////////////////////////////////////
		inline element_pointer getREnd() {
			return _sentinel;
		}
		inline size_type getSize() const {
			return _size;
		}
		inline bool empty() const {
			return (_size == 0);
		}
		//interface for iterators
		element_pointer increment(element_pointer data) const {
            auto child = data->_right;
            if (!IsNil(child)) return TreeMinimum(child);
            auto parent = data->_parent;
            while (!IsNil(parent)) {
                if (data != parent->_right) break;
                data = parent;
                parent = data->_parent;
            }
            return parent;
        }
		element_pointer decrement(element_pointer data) const {
            // A bidirectional tree iterator may decrement end() to the maximum.
            if (IsNil(data)) return Maximum();
            auto child = data->_left;
            if (!IsNil(child)) return TreeMaximum(child);
            auto parent = data->_parent;
            while (!IsNil(parent)) {
                if (data != parent->_left) break;
                data = parent;
                parent = data->_parent;
            }
            return parent;
        }

		inline pointer getData(element_pointer data) const {
			return data->getDataPointer();
		}

	protected:
        // Update only on insertion/removal. Rotations preserve in-order extrema.
        void cacheInsertedNode(element_pointer node, element_pointer parent) {
            if (IsNil(parent)) {
                _leftmost = node;
                _rightmost = node;
            } else {
                if (parent == _leftmost &&
                    parent->_left == node)
                    _leftmost = node;
                if (parent == _rightmost &&
                    parent->_right == node)
                    _rightmost = node;
            }
        }
        void cacheErasedNode(element_pointer node) {
            if (node == _leftmost)
                _leftmost = Successor(node);
            if (node == _rightmost)
                _rightmost = Predecessor(node);
        }

        void incrementSize() {
            ++_size;
        }

        void decrementSize() {
            --_size;
        }

		element_pointer TreeSearch(element_pointer x, const_input_reference k) {
            while (!IsNil(x)) {
                const auto& key = x->getData();
                if (k < key) {
                    x = x->_left;
                } else if (key < k) {
                    x = x->_right;
                } else {
                    return x;
                }
            }

			return x;
		}
		inline element_pointer TreeMinimum(element_pointer x) const {
            while (true) {
                auto next = x->_left;
                if (IsNil(next)) return x;
                x = next;
            }
        }
		inline element_pointer TreeMaximum(element_pointer x) const {
            while (true) {
                auto next = x->_right;
                if (IsNil(next)) return x;
                x = next;
            }
        }
		void InorderTreeWalk(element_pointer x) {
			if (!IsNil(x)) {
				InorderTreeWalk(x->_left);
				//std::cout << x->getData() << " ";
				InorderTreeWalk(x->_right);
			}
		}
		void destroySubtree(element_pointer x) {
			if (!IsNil(x)) {
				destroySubtree(x->_left);
				destroySubtree(x->_right);
				delete x;
			}
		}

		element_pointer _root;
        element_pointer _leftmost, _rightmost;
		element_pointer _sentinel;
		size_type _size = 0;

	};

	///////////////////////////////////////////////////////////////
	// Tree Base Extensions
	///////////////////////////////////////////////////////////////
	template <class treeType> 
	class RedBlackTreeExtension : public treeType {
	public:
		////////////////////////////////////////////////////////////////////////
		typedef typename treeType::Allocator Allocator;
		typedef typename treeType::value_type value_type;
		////////////////////////////////////////////////////////////////////////
		typedef typename treeType::pointer pointer;
		typedef typename treeType::reference reference;
		typedef typename treeType::const_reference const_reference;

		typedef typename treeType::BTelementType elementType;

		typedef typename treeType::input_value_type input_value_type;
		typedef typename treeType::const_input_reference const_input_reference;

		typedef typename treeType::element_pointer element_pointer;

		typedef typename treeType::element_reference element_reference;

		typedef typename treeType::size_type size_type;
		typedef typename treeType::difference_type difference_type;

		using treeType::_sentinel;
		using treeType::_root;

		// ---------------- Construction and destruction ---------------

		RedBlackTreeExtension () {}
		//RedBlackTreeExtension (const RedBlackTreeExtension & other) {
		//	_root = other._root;
		//	_sentinel = other._sentinel;
		virtual element_pointer Insert(const_input_reference k){
			element_pointer y = _sentinel;
			element_pointer x = _root;
			while (!(this->IsNil(x))) {
				y = x;
				#ifdef DDTR_TREE_RANKS
					x->_subtreeSize += 1;
				#endif
				if (k < x->getData())
					x = x->_left;
				else
					x = x->_right;
			}
			element_pointer z = new elementType(k, _sentinel, y);
			this->incrementSize();
			if (this->IsNil(y)){
				_root = z;
			}
			else {
				if (k < y->getData())
					y->_left = z;
				else
					y->_right = z;
			}
            this->cacheInsertedNode(z, y);
			RBInsertFixup(z);
			return z;
		}
        void FinishHintInsert(element_pointer node) override {
            RBInsertFixup(node);
        }

        std::pair<element_pointer, bool> InsertUnique(const_input_reference k) override {
            auto result = treeType::InsertUnique(k);
            if (result.second) {
                RBInsertFixup(result.first);
            }
            return result;
        }

		virtual element_pointer Delete(element_pointer z) {
            this->cacheErasedNode(z);
			element_pointer x, y, removed;
			if((this->IsNil(z->_left) || this->IsNil(z->_right)))
				y = z;
			else
				y = this->Successor(z);
			#ifdef DDTR_TREE_RANKS
				element_pointer tmp = y;
				while (!(this->IsNil(tmp))) {
					tmp->_subtreeSize += -1;
					tmp = tmp->_parent;
				}
			#endif
			this->decrementSize();
			if (!(this->IsNil(y->_left)))
				x = y->_left;
			else{
				x = y->_right;
			}
			x->_parent = y->_parent;
			if (this->IsNil(y->_parent))
				_root = x;
			else {
				if (y == y->_parent->_left)
					y->_parent->_left = x;
				else 
					y->_parent->_right = x;
			}
			auto yColor = y->_color;
			if (y != z){
				#ifndef OUT_OF_LINE_STORAGE
					this->transplantSuccessorNode(z, y, x);
					y->_color = z->_color;
					removed = z;
				#else
					auto _data_tmp = z->_data;
					z->_data = y->_data;
					y->_data = _data_tmp;
                    if (this->_rightmost == y)
                        this->_rightmost = z;
					removed = y;
				#endif
			}
			else
				removed = y;
			if (yColor == BLACK)
				RBDeleteFixup(x);

			return removed;
		}

	protected:
		void RBInsertFixup(element_pointer z) {
			element_pointer y;
            while (!this->IsNil(z)) {
                auto parent = z->_parent;
                if ((this->IsNil(parent) || parent->_color != RED)) break;
				if (z->_parent == z->_parent->_parent->_left) {
					y = z->_parent->_parent->_right;
					if (y->_color == RED) {
						z->_parent->_color = BLACK;
						y->_color = BLACK;
						z->_parent->_parent->_color = RED;
						z = z->_parent->_parent;
					} else {
						if (z == z->_parent->_right) {
							z = z->_parent;
							this->LeftRotate(z);
						}
						z->_parent->_color = BLACK;
						z->_parent->_parent->_color = RED;
						this->RightRotate(z->_parent->_parent);
					}
				} else {
					y = z->_parent->_parent->_left;
					if (y->_color == RED) {
						z->_parent->_color = BLACK;
						y->_color = BLACK;
						z->_parent->_parent->_color = RED;
						z = z->_parent->_parent;
					} else {
						if (z == z->_parent->_left) {
							z = z->_parent;
							this->RightRotate(z);
						}
						z->_parent->_color = BLACK;
						z->_parent->_parent->_color = RED;
						this->LeftRotate(z->_parent->_parent);
					}
				}
			}
			_root->_color = BLACK;
		}
		void RBDeleteFixup(element_pointer x) {
			element_pointer w;
            while ((x != _root && x->_color == BLACK)) {
				if (x == x->_parent->_left) {
					w = x->_parent->_right;
					if (w->_color == RED) {
						w->_color = BLACK;
						x->_parent->_color = RED;
						this->LeftRotate(x->_parent);
						w = x->_parent->_right;
					}

					if ((w->_left->_color == BLACK && w->_right->_color == BLACK)) {
						w->_color = RED;
						x = x->_parent;
					} else {
						if (w->_right->_color == BLACK) {
							w->_left->_color = BLACK;
							w->_color = RED;
							this->RightRotate(w);
							w = x->_parent->_right;
						}
						w->_color = x->_parent->_color;
						x->_parent->_color = BLACK;
						w->_right->_color = BLACK;
						this->LeftRotate(x->_parent);
						x = _root;
					}
				} else {
					w = x->_parent->_left;
					if (w->_color == RED) {
						w->_color = BLACK;
						x->_parent->_color = RED;
						this->RightRotate(x->_parent);
						w = x->_parent->_left;
					}

					if ((w->_right->_color == BLACK && w->_left->_color == BLACK)) {
						w->_color = RED;
						x = x->_parent;
					} else {
						if (w->_left->_color == BLACK) {
							w->_right->_color = BLACK;
							w->_color = RED;
							this->LeftRotate(w);
							w = x->_parent->_left;
						}
						w->_color = x->_parent->_color;
						x->_parent->_color = BLACK;
						w->_left->_color = BLACK;
						this->RightRotate(x->_parent);
						x = _root;
					}
				}
			}
			x->_color = BLACK;
		}

	};

	template <class treeType>
	class AVLTreeExtension : public treeType {
	public:
		////////////////////////////////////////////////////////////////////////
		typedef typename treeType::Allocator Allocator;
		typedef typename treeType::value_type value_type;
		////////////////////////////////////////////////////////////////////////
		typedef typename treeType::pointer pointer;
		typedef typename treeType::reference reference;
		typedef typename treeType::const_reference const_reference;

		typedef typename treeType::BTelementType elementType;

		typedef typename treeType::input_value_type input_value_type;
		typedef typename treeType::const_input_reference const_input_reference;

		typedef typename treeType::element_pointer element_pointer;

		typedef typename treeType::element_reference element_reference;

		typedef typename treeType::size_type size_type;
		typedef typename treeType::difference_type difference_type;

		using treeType::_sentinel;
		using treeType::_root;

		AVLTreeExtension () {}

		virtual element_pointer Insert(const_input_reference k) {
			element_pointer y = _sentinel;
			element_pointer x = _root;
			while (!(this->IsNil(x))) {
				y = x;
				#ifdef DDTR_TREE_RANKS
					x->_subtreeSize += 1;
				#endif
				if (k < x->getData())
					x = x->_left;
				else
					x = x->_right;
			}
			element_pointer z = new elementType(k, _sentinel, y);
			this->incrementSize();
			if (this->IsNil(y)){
				_root = z;
			}else {
				if (k < y->getData())
					y->_left = z;
				else
					y->_right = z;
			}
            this->cacheInsertedNode(z, y);
			AVLInsertRebalance(z->_parent);
			return z;
		}

        void FinishHintInsert(element_pointer node) override {
            AVLInsertRebalance(node->_parent);
        }

        std::pair<element_pointer, bool> InsertUnique(const_input_reference k) override {
            auto result = treeType::InsertUnique(k);
            if (result.second) {
                AVLInsertRebalance(result.first->_parent);
            }
            return result;
        }

		virtual element_pointer Delete(element_pointer z) {
            this->cacheErasedNode(z);
			element_pointer x, y, removed;
			if ((this->IsNil(z->_left) || this->IsNil(z->_right)))
				y = z;
			else
				y = this->Successor(z);
			#ifdef DDTR_TREE_RANKS
				element_pointer tmp = y;
				while (!(this->IsNil(tmp))) {
					tmp->_subtreeSize += -1;
					tmp = tmp->_parent;
				}
			#endif
			this->decrementSize();
			if (!(this->IsNil(y->_left)))
				x = y->_left;
			else{
				x = y->_right;
			}
			x->_parent = y->_parent;
			if (this->IsNil(y->_parent)){
				_root = x;
			}else {
				if (y == y->_parent->_left)
					y->_parent->_left = x;
				else
					y->_parent->_right = x;
			}
			if (y != z){
				#ifndef OUT_OF_LINE_STORAGE
					this->transplantSuccessorNode(z, y, x);
					y->_height = z->_height;
					removed = z;
				#else
					auto _data_tmp = z->_data;
					z->_data = y->_data;
					y->_data = _data_tmp;
                    if (this->_rightmost == y)
                        this->_rightmost = z;
					removed = y;
				#endif
			}
			else
				removed = y;
			AVLDeleteRebalance(x->_parent);
			return removed;
		}

	protected:
		int nodeHeight(element_pointer n) {

			if (this->IsNil(n)) return 0;

			return n->_height;
		}

		int balanceFactor(element_pointer n) {
			return nodeHeight(n->_left) - nodeHeight(n->_right);
		}

		void updateHeight(element_pointer n) {
			int lh = nodeHeight(n->_left);
			int rh = nodeHeight(n->_right);
			n->_height = 1 + (lh > rh ? lh : rh);
		}

		int avlRefresh(element_pointer n, int& balance) {
			int lh = nodeHeight(n->_left);
			int rh = nodeHeight(n->_right);
			balance = lh - rh;
			int newH = 1 + (lh > rh ? lh : rh);
			if (n->_height != newH) {
				n->_height = newH;
				return 1;
			}
			return 0;
		}

		void AVLInsertRebalance(element_pointer n) {
			while (!(this->IsNil(n))) {
				int balance;
				int changed = avlRefresh(n, balance);
				if (balance > 1) {
					if (balanceFactor(n->_left) < 0) {
						element_pointer L = n->_left;
						this->LeftRotate(L);
						updateHeight(L);
						updateHeight(n->_left);
					}
					this->RightRotate(n);
					updateHeight(n);
					updateHeight(n->_parent);
					return;
				} else if (balance < -1) {
					if (balanceFactor(n->_right) > 0) {
						element_pointer R = n->_right;
						this->RightRotate(R);
						updateHeight(R);
						updateHeight(n->_right);
					}
					this->LeftRotate(n);
					updateHeight(n);
					updateHeight(n->_parent);
					return;
				}
				if (!changed) return;
				n = n->_parent;
			}
		}

		void AVLDeleteRebalance(element_pointer n) {
			while (!(this->IsNil(n))) {
				int balance;
				int changed = avlRefresh(n, balance);
				if (balance > 1) {
					int bf = balanceFactor(n->_left);
					if (bf < 0) {
						element_pointer L = n->_left;
						this->LeftRotate(L);
						updateHeight(L);
						updateHeight(n->_left);
					}
					element_pointer P = n->_parent;
					this->RightRotate(n);
					updateHeight(n);
					updateHeight(n->_parent);
					if (bf == 0) return;
					n = P;
				} else if (balance < -1) {
					int bf = balanceFactor(n->_right);
					if (bf > 0) {
						element_pointer R = n->_right;
						this->RightRotate(R);
						updateHeight(R);
						updateHeight(n->_right);
					}
					element_pointer P = n->_parent;
					this->LeftRotate(n);
					updateHeight(n);
					updateHeight(n->_parent);
					if (bf == 0) return;
					n = P;
				} else {
					if (!changed) return;
					n = n->_parent;
				}
			}
		}
	};

	///////////////////////////////////////////////////////////////
	// This is the main handler for the tree abstract data type
	///////////////////////////////////////////////////////////////
	template <class treeType>
	class Tree : public treeType{
	public:
		////////////////////////////////////////////////////////////////////////
		typedef typename treeType::Allocator Allocator;
		typedef typename treeType::value_type value_type;
		////////////////////////////////////////////////////////////////////////
		typedef typename treeType::pointer pointer;
		typedef typename treeType::reference reference;
		typedef typename treeType::const_reference const_reference;

		typedef typename treeType::element_pointer element_pointer;
		typedef typename treeType::element_reference element_reference;
		typedef typename treeType::BTelementType elementType;

		typedef typename treeType::input_value_type input_value_type;
		typedef typename treeType::const_input_reference const_input_reference;

		typedef typename treeType::size_type size_type;
		typedef typename treeType::difference_type difference_type;

		typedef Iterator_Base<Tree> iterator;
		typedef ConstIteratorAdapter<iterator> const_iterator;

		typedef ReverseIteratorAdapter<iterator> reverse_iterator;
		typedef ReverseIteratorAdapter<const_iterator> const_reverse_iterator;

        // Copies of elements; get(n), operator[] and front()/back() return references.
        input_value_type read(size_type n) { return get(n); }
        input_value_type read(size_type n) const { return get(n); }
        void write(size_type n, const_input_reference value) { (*this)[n] = value; }

		Tree() {}
        // No scope: the move is the base's work, held until SWAP names it.
        Tree(Tree&& other) : treeType(std::move(other)) {
        }

        // Clone the topology in linear time: no insertions, searches or rotations.
        Tree(const Tree& other) : treeType() {
            this->_root = cloneNode(other, other._root, this->_sentinel);
            this->_leftmost = this->TreeMinimum(this->_root);
            this->_rightmost = this->TreeMaximum(this->_root);
            this->_size = other._size;
        }

    private:
        template<class Node, class = void> struct has_color : std::false_type {};
        template<class Node> struct has_color<Node, std::void_t<decltype(std::declval<Node>()._color)>> : std::true_type {};
        template<class Node, class = void> struct has_height : std::false_type {};
        template<class Node> struct has_height<Node, std::void_t<decltype(std::declval<Node>()._height)>> : std::true_type {};
        element_pointer cloneNode(const Tree& other, element_pointer from,
                                            element_pointer parent) {
            if (other.IsNil(from)) return this->_sentinel;
            auto node = new elementType(from->getData(), this->_sentinel, parent);
            DDTR_TRY {
#ifdef DDTR_TREE_RANKS
                node->_subtreeSize = from->_subtreeSize;
#endif
                if constexpr (has_color<elementType>::value) node->_color = from->_color;
                if constexpr (has_height<elementType>::value) node->_height = from->_height;
                node->_left = cloneNode(other, from->_left, node);
                node->_right = cloneNode(other, from->_right, node);
            } DDTR_CATCH_ALL {
                this->destroySubtree(node);
                DDTR_RETHROW();
            }
            return node;
        }
    public:

		void clear() {
			if (!treeType::empty()) {
				treeType::destroySubtree(treeType::Root());
			}

			treeType::initialize();

		}

		~Tree() {
			clear();
		}

		////////////////////////////////////////////////////////////////////////
		// Basic iterators of DDT
		////////////////////////////////////////////////////////////////////////
		const_iterator begin() const {
			const_iterator it(const_cast<Tree*>(this), treeType::getFirstElement());
			return it;
		}
		iterator begin() {
			iterator it(this, treeType::getFirstElement());
			return it;
		}
		const_iterator end() const {
            const_iterator it(const_cast<Tree*>(this), treeType::getEnd());
            return it;
		}
		iterator end() {
            iterator it(this, treeType::getEnd());
            return it;
		}
		// Forward iterator at the first key >= k (or end() if none).
		iterator lower_bound(const_input_reference k) {
			return iterator(this, treeType::LowerBoundNode(k));
		}
        iterator upper_bound(const_input_reference key) {
            auto node = this->_root;
            auto found = this->_sentinel;
            while (!this->IsNil(node)) {
                if (key < node->getData()) {
                    found = node;
                    node = node->_left;
                } else node = node->_right;
            }
            return iterator(this, found);
        }

		// Forward iterator at the last (largest) element (++ -> end, -- -> previous).
		iterator last() {
			size_type n = size();
			iterator it(this, treeType::getLastElement());
			return it;
		}
		////////////////////////////////////////////////////////////////////////
		// Reverse iterators of DDT
		////////////////////////////////////////////////////////////////////////
		const_reverse_iterator rbegin() const {
			const_reverse_iterator it(const_cast<Tree*>(this), treeType::getLastElement());
			return it;
		}
		reverse_iterator rbegin() {
			reverse_iterator it(this, treeType::getLastElement());
			return it;
		}
		const_reverse_iterator rend() const {
            const_reverse_iterator it(const_cast<Tree*>(this), treeType::getEnd());
            return it;
		}
		reverse_iterator rend() {
            reverse_iterator it(this, treeType::getEnd());
            return it;
		}

		////////////////////////////////////////////////////////////////////////
		// Size information
		////////////////////////////////////////////////////////////////////////
		inline bool empty() const {
			return treeType::empty();
		}
		inline size_type size() const {
			return treeType::getSize();
		}

		size_t footprint() {
			const size_t node_bytes = sizeof(elementType);
			const size_t data_bytes = sizeof(input_value_type);
			const auto n = treeType::getSize();
            size_t total = sizeof(*this) + (n + 1) * node_bytes;
#ifdef OUT_OF_LINE_STORAGE
            total += n * data_bytes; // The sentinel has no value buffer.
#endif
            return total;
		}
		inline size_type capacity() const {
			return treeType::getSize();
		}

        // Ordered insertion already maintains stable in-order traversal.
        void sort() {
        }

		void reserve(size_type new_capacity) {
        }

		reference operator[](size_type n) {
			return get(n);
		}

		const_reference operator[](size_type n) const {
			return get(n);
		}

		reference get(size_type n) {
            auto payload = treeType::ItemWithRank(n);
            return *payload;
		}

		const_reference get(size_type n) const {
            auto payload = treeType::ItemWithRank(n);
            return *payload;
		}

		const_reference front() const {
            auto payload = treeType::Smallest();
            return *payload;
		}
		reference front() {
            auto payload = treeType::Smallest();
            return *payload;
		}
		const_reference back() const {
            auto payload = treeType::Largest();
            return *payload;
		}
		reference back() {
            auto payload = treeType::Largest();
            return *payload;
		}

		iterator erase(const iterator& pos) {
			element_pointer curr = pos.getData();
			element_pointer next = treeType::Successor(curr);
			element_pointer tmp = treeType::Delete(curr);
			if (tmp == next)
				next = curr;
			if (!(this->IsNil(tmp)))
				delete tmp;
			return iterator(this, next);
		}

		reverse_iterator erase(const reverse_iterator& pos) {
			element_pointer curr = pos.getData();
			element_pointer prev = treeType::Predecessor(curr);
			element_pointer tmp = treeType::Delete(curr);
			if (!(this->IsNil(tmp)))
				delete tmp;
			return reverse_iterator(this, prev);
		}

        iterator insert(const iterator& hint, const_input_reference value) {
            auto node = this->InsertHint(hint.getData(), value);
            return iterator(this, node);
        }

        iterator erase(const iterator& first, const iterator& last) {
            auto node = first.getData();
            const auto stop = last.getData();
            while (node != stop) {
                auto next = this->Successor(node);
                auto removed = this->Delete(node);
                if (removed == next) next = node;
                delete removed;
                node = next;
            }
            return iterator(this, node);
        }

        // Update a mapped payload in place, without copying it or changing its key.
        template<class Update> void update(const iterator& position, Update change) {
            auto payload = this->getData(position.getData());
            change(*payload);
        }

		iterator insert(const_input_reference value) {
			element_pointer tmp = treeType::Insert(value);
			return iterator(this, tmp);
		}

        // Inserts only if no equivalent key exists; otherwise returns the existing one.
        std::pair<iterator, bool> insert_unique(const_input_reference value) {
            auto result = treeType::InsertUnique(value);
            return {iterator(this, result.first), result.second};
        }

		void push_back(const_input_reference value) {
			treeType::Insert(value);
		}

		void push_front(const_input_reference value) {
			push_back(value);
		}

		void pop_back() {
			element_pointer max_node = treeType::Maximum();
			if (!this->IsNil(max_node)) {
				element_pointer tmp = treeType::Delete(max_node);
				if (!this->IsNil(tmp))
					delete tmp;
			}
		}

		void pop_front() {
			element_pointer min_node = treeType::Minimum();
			if (!this->IsNil(min_node)) {
				element_pointer tmp = treeType::Delete(min_node);
				if (!this->IsNil(tmp))
					delete tmp;
			}
		}

		void add(size_type n, const_input_reference value) {
			push_back(value);
		}

		void resize(size_type new_size) {
			while (size() > new_size) {
				element_pointer max_node = treeType::Maximum();
				if (!this->IsNil(max_node)) {
					element_pointer tmp = treeType::Delete(max_node);
					if (!this->IsNil(tmp)) delete tmp;
				}
			}
			while (size() < new_size) {
				treeType::Insert(input_value_type());
			}
        }

		void remove(const_input_reference value) {
			element_pointer tmp = treeType::Remove(value);
			if (!this->IsNil(tmp))
				delete tmp;
		}

		void remove_value(const_input_reference value) {
			remove(value);
		}

        struct extracted_deleter {
            element_pointer node = nullptr;
            void operator()(pointer) const { delete node; }
        };
        using extracted_value = std::unique_ptr<input_value_type, extracted_deleter>;

		extracted_value ExtractMin () {
			auto node = treeType::Minimum();
            if (this->IsNil(node)) return extracted_value(nullptr, extracted_deleter{});
            auto detached = treeType::Delete(node);
            return extracted_value(detached->getDataPointer(), extracted_deleter{detached});
		}

        const_iterator find(const_input_reference item) const {
            return const_iterator(const_cast<Tree*>(this)->find(item));
        }

        // Comparator equivalence, first matching rank, just like BTree::find.
        iterator find(const_input_reference item) {
            auto node = this->_root;
            auto found = this->_sentinel;
            while(!this->IsNil(node)) {
                if(!(node->getData() < item)) {
                    found = node;
                    node = node->_left;
                } else node = node->_right;
            }
            if(!this->IsNil(found) &&
               item < found->getData())
                found = this->_sentinel;
            return iterator(this, found);
        }

		pointer Find(const_input_reference item) {
			return treeType::Find(item);
		}

		////////////////////////////////////////////////////////////////////////
		// Element access used by the iterators
		////////////////////////////////////////////////////////////////////////
		bool exists(element_pointer x) {

			if (!treeType::empty()) {
				iterator it1(this, treeType::getFirstElement());
				iterator it2(this, treeType::getEnd());
				for (iterator it = it1;it != it2; it++) {
					if (it.getData() == x) {
                        return true;
                    }
				}
			}
            return false;
		}

	};
}
////////////////////////////////////////////////////////////////////////////////
// End namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
#endif
