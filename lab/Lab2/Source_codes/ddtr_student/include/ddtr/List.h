#ifndef LIST_H
#define LIST_H

#include "DDTIncludes.h"
#include <type_traits>
////////////////////////////////////////////////////////////////////////////////
// Begin namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
namespace DDTLibrary {

    template <class listType> class List;

    template<class listType> class sllRovingExtension;
    template<class listType> class dllRovingExtension;

    template<class listBase> class dll;
    template<class listBase> class sll;

    template<class elementType> class listBase;

template<class T, class A = ddtr::malloc_allocator> class dllListElement;
template<class T, class A = ddtr::malloc_allocator> class sllListElement;

    ///////////////////////////////////////////////////////////////
    // List Elements
    ///////////////////////////////////////////////////////////////
    template<class T, class A>
    class dllListElement {
    public:
        ////////////////////////////////////////////////////////////////////////
        typedef A Allocator;
        typedef T value_type;
        typedef T* data_pointer;

        typedef dllListElement* element_pointer;
        typedef dllListElement& element_reference;

        ////////////////////////////////////////////////////////////////////////
        typedef T input_value_type;
        typedef const input_value_type& const_input_reference;

        typedef size_t size_type;
        typedef ptrdiff_t difference_type;

        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef value_type& reference;
        typedef const value_type& const_reference;

        dllListElement()
        {
            next_ = element_pointer(NULL);
            prev_ = element_pointer(NULL);
            #ifdef OUT_OF_LINE_STORAGE
                data_ = NULL;
            #else
                ::new(static_cast<void*>(std::addressof(data_))) input_value_type;
            #endif
        }
        dllListElement(const_input_reference data)
        {
            #ifdef OUT_OF_LINE_STORAGE
                data_ = (pointer) Allocator::malloc(sizeof(input_value_type));
            #endif
            next_ = element_pointer(NULL);
            prev_ = element_pointer(NULL);
            #ifndef OUT_OF_LINE_STORAGE
                Constructor(std::addressof(data_), data);
            #else
                Constructor(data_, data);
            #endif
        }

        ~dllListElement() {
#ifdef OUT_OF_LINE_STORAGE
            if (data_ != nullptr) {
                Destructor(data_);
                Allocator::free(data_);
            }
#else
            Destructor(std::addressof(data_));
#endif
        }

        inline void initialize (element_pointer node) {
            next_ = node;
            prev_ = node;
        }
        inline reference getData() {
            #ifndef OUT_OF_LINE_STORAGE
                return data_;
            #else
                return *data_;
            #endif
        }
        inline const_reference getData() const {
            #ifndef OUT_OF_LINE_STORAGE
                return data_;
            #else
                return *data_;
            #endif
        }

        inline pointer getDataPointer() {
            #ifndef OUT_OF_LINE_STORAGE
                return &data_;
            #else
                return data_;
            #endif
        }
        inline void makeLink(element_pointer prev, element_pointer next) {
            if (prev)
            {
                prev->next_ = this;
                this->prev_ = prev;
            }
            else
            {
                this->prev_ = element_pointer(NULL);
            }
            if (next)
            {
                this->next_ = next;
                next->prev_ = this;
            }
            else
            {
                this->next_ = element_pointer(NULL);
            }
        }
        inline void removeLink(element_pointer prev, element_pointer next) {
            if (prev)
            {
                prev->next_ = next;
            }
            if (next)
            {
                next->prev_ = prev;
            }
        }

        void * operator new(const size_t sz)   { return Allocator::malloc(sz); }
        void operator delete(void * p)         { return Allocator::free(p); }
        void * operator new[](const size_t sz) { return Allocator::malloc(sz); }
        void operator delete[](void * p)       { return Allocator::free(p); }

        element_pointer next_;
        element_pointer prev_;
        #ifndef OUT_OF_LINE_STORAGE
            union { input_value_type data_; };
        #else
            pointer data_;
        #endif

    };

    template<class T, class A>
    class sllListElement {
    public:
        ////////////////////////////////////////////////////////////////////////
        typedef A Allocator;
        typedef T value_type;
        typedef T* data_pointer;
        typedef sllListElement* element_pointer;
        typedef sllListElement& element_reference;

        ////////////////////////////////////////////////////////////////////////

        typedef T input_value_type;
        typedef const input_value_type& const_input_reference;

        typedef size_t size_type;
        typedef ptrdiff_t difference_type;

        typedef value_type* pointer;
        typedef const value_type* const_pointer;
        typedef const data_pointer const_data_pointer;
        typedef value_type& reference;
        typedef const value_type& const_reference;

        sllListElement()
        {
            next_ = element_pointer(NULL);
            #ifdef OUT_OF_LINE_STORAGE
                data_ = NULL;
            #else
                ::new(static_cast<void*>(std::addressof(data_))) input_value_type;
            #endif
        }

        sllListElement(const_input_reference data) {

            #ifdef OUT_OF_LINE_STORAGE
                data_ = (pointer) Allocator::malloc(sizeof(input_value_type));
            #endif

            next_ = element_pointer(NULL);
            #ifndef OUT_OF_LINE_STORAGE
                Constructor(std::addressof(data_), data);
            #else
                Constructor(data_, data);
            #endif
        }
        ~sllListElement() {
#ifdef OUT_OF_LINE_STORAGE
            if (data_ != nullptr) {
                Destructor(data_);
                Allocator::free(data_);
            }
#else
            Destructor(std::addressof(data_));
#endif
        }

        inline void initialize (element_pointer node) {
            next_ = node;
        }

        inline void makeLink(element_pointer prev, element_pointer next) {
            if (prev) {
                prev->next_ = this;
            }
            this->next_ = next;
        }
        inline void removeLink(element_pointer prev, element_pointer next) {
            if (prev) {
                prev->next_ = next;
            }
        }
        inline reference getData() {
            #ifndef OUT_OF_LINE_STORAGE
                return data_;
            #else
                return *data_;
            #endif
        }
        inline const_reference getData() const {
            #ifndef OUT_OF_LINE_STORAGE
                return data_;
            #else
                return *data_;
            #endif
        }

        inline pointer getDataPointer() {
            #ifndef OUT_OF_LINE_STORAGE
                return &data_;
            #else
                return data_;
            #endif
        }

        void * operator new(const size_t sz)   { return Allocator::malloc(sz); }
        void operator delete(void * p)         { return Allocator::free(p); }
        void * operator new[](const size_t sz) { return Allocator::malloc(sz); }
        void operator delete[](void * p)       { return Allocator::free(p); }

        element_pointer next_;
        #ifndef OUT_OF_LINE_STORAGE
            union { input_value_type data_; };
        #else
            pointer data_;
        #endif

    };
    
    template<class T, class A>
    void swap(sllListElement<T, A>& lhs, sllListElement<T, A>& rhs)
    {
        std::cout << "swap(A&, A&)" << std::endl;
        using std::swap;
        swap(lhs.ptr_, rhs.ptr_);
    }

    // listBase without sentinel. Last element points to null
    template<class elementType>
    class listBaseNoSentinel {
    public:
        ////////////////////////////////////////////////////////////////////////
        enum { HAS_SENTINEL = 0 };
        typedef typename elementType::Allocator Allocator;
        typedef typename elementType::value_type value_type;
        ////////////////////////////////////////////////////////////////////////
        typedef typename elementType::pointer pointer;
        typedef typename elementType::reference reference;
        typedef const typename elementType::const_reference const_reference;

        typedef elementType listElementType;

        typedef typename elementType::input_value_type input_value_type;
        typedef typename elementType::const_input_reference const_input_reference;

        typedef typename elementType::element_pointer element_pointer;

        typedef typename elementType::element_reference element_reference;

        typedef typename elementType::size_type size_type;
        typedef typename elementType::difference_type difference_type;

        listBaseNoSentinel () {
            _size = 0;
            head_ = element_pointer(NULL);
            tail_ = element_pointer(NULL);
        }
        listBaseNoSentinel(const listBaseNoSentinel&) = delete;
        listBaseNoSentinel& operator=(const listBaseNoSentinel&) = delete;
        ~listBaseNoSentinel() {
        }
        void reset_roving() {}
        void initialize() {
            _size = 0;
            head_ = tail_ = nullptr;
        }
        inline element_pointer getFirstElement() const {
            return head_;
        }
        inline element_pointer getLastElement() const {
            return tail_;
        }
        inline element_pointer getNextElement(element_pointer data) const {
            return data->next_;
        }
        inline element_pointer increment(element_pointer data) const {
            return data ? data->next_ : head_;
        }
        inline element_pointer getEnd() const {
            return element_pointer(NULL);
        }
        element_pointer push_back(const_input_reference value) {
            element_pointer tmp = new elementType(value);
            if (head_ == element_pointer(NULL)) {
                head_ = tmp;
            }
            else {
                tmp->makeLink(tail_, element_pointer(NULL));
            }
            tail_ = tmp;

            _size++;
            return tmp;
        }
        void insert_after(const_input_reference value, element_pointer prev) {
            element_pointer tmp = new elementType(value);
            tmp->makeLink(prev, prev->next_);
            if (tail_ == prev) {
                tail_ = tmp;
            }

            _size++;
        }
        void erase(element_pointer prev, element_pointer curr, element_pointer next) {

            if (next == nullptr){
                tail_ = prev;
            }
            if (curr == head_){
                head_ = next;
            }
            curr->removeLink(prev, next);
            _size--;
        }
        //to be removed
        inline pointer getData(element_pointer data) const {
            #ifdef OUT_OF_LINE_STORAGE
            #endif
            return data->getDataPointer();
        }
        inline bool empty() const {
            return (_size == 0);
        }
        inline size_type getSize() const {
            return _size;
        }
        inline bool IsNil(element_pointer x) const {
            return x == NULL;
        }
        
        void resize(size_type n) {

            if (n > _size) { //increase size => append empty elements
                for (size_type i = 0;i < n - _size; i++) {
                    element_pointer tmp = new elementType();
                    if (head_ == NULL) {
                        head_ = tmp;
                    }
                    else {
                        tmp->makeLink(tail_, NULL);
                    }
                    tail_ = tmp;
                }
            }
            else { //decrease size => remove elements from the end
                element_pointer prev = head_;

                for (size_type i = 0;i < n - 1; i++)
                    prev = getNextElement(prev);

                element_pointer curr = getNextElement(prev);
                element_pointer next = getNextElement(curr);

                for (size_type i = 0;i < _size - n; i++) {
                    erase(prev, curr, next);
                    curr = getNextElement(prev);
                    next = getNextElement(curr);
                }
            }
            _size = n;
        }

        void swap(listBaseNoSentinel& other) {
            size_type tmpSize = _size; _size = other._size; other._size = tmpSize;
            element_pointer tmpHead = head_; head_ = other.head_; other.head_ = tmpHead;
            element_pointer tmpTail = tail_; tail_ = other.tail_; other.tail_ = tmpTail;
        }
    protected:
        size_type _size;
        element_pointer tail_, head_;
    };
    
    ///////////////////////////////////////////////////////////////
    // listBase defines a type of list. This type uses a sentinel.
    // Other types can also be defined and used.
    ///////////////////////////////////////////////////////////////
    template<class elementType>
    class listBase {
    public:
        ////////////////////////////////////////////////////////////////////////
        enum { HAS_SENTINEL = 1 };
        typedef typename elementType::Allocator Allocator;
        typedef typename elementType::value_type value_type;
        ////////////////////////////////////////////////////////////////////////
        typedef typename elementType::pointer pointer;
        typedef typename elementType::reference reference;
        typedef const typename elementType::const_reference const_reference;

        typedef elementType listElementType;

        typedef typename elementType::input_value_type input_value_type;
        typedef typename elementType::const_input_reference const_input_reference;

        typedef typename elementType::element_pointer element_pointer;

        typedef typename elementType::element_reference element_reference;

        typedef typename elementType::size_type size_type;
        typedef typename elementType::difference_type difference_type;

        listBase (short = 0) {
            _size = 0;
            sentinel = new elementType();
            sentinel->initialize(sentinel);
            head_ = sentinel;
            tail_ = sentinel;
        }
        ~listBase() {
            delete sentinel;
        }
        void reset_roving() {}
        void initialize() {
            _size = 0;
            sentinel->initialize(sentinel);
            head_ = sentinel;
            tail_ = sentinel;
        }
        inline element_pointer getFirstElement() const {
            return head_;
        }
        inline element_pointer getLastElement() const {
            return tail_;
        }
        inline element_pointer getNextElement(element_pointer data) const {
            return data->next_;
        }
        inline element_pointer increment(element_pointer data) const {
            return data->next_;
        }
        inline element_pointer getEnd() const {
            return sentinel;
        }
        
        element_pointer push_back(const_input_reference value) {
            element_pointer tmp = new elementType(value);
            if (sentinel->next_ == sentinel) {
                head_ = tmp;
                tmp->makeLink(sentinel, sentinel->next_);
            }
            else {
                tmp->makeLink(tail_, sentinel);
            }
            tail_ = tmp;

            _size++;
            return tmp;
        }
        void insert_after(const_input_reference value, element_pointer prev) {
            element_pointer tmp = new elementType(value);
            tmp->makeLink(prev, prev->next_);
            if (tail_ == prev) {
                tail_ = tmp;
            }

            _size++;
        }
        void erase(element_pointer prev, element_pointer curr, element_pointer next) {
            if (next == sentinel) {
                tail_ = prev;
            }
            if (curr == head_) {
                head_ = next;
            }
            curr->removeLink(prev, next);
            _size--;
        }
        //to be removed
        inline pointer getData(element_pointer data) const {
            #ifdef OUT_OF_LINE_STORAGE
            #endif
            return data->getDataPointer();
        }
        inline bool empty() const {
            return (_size == 0);
        }
        inline size_type getSize() const {
            return _size;
        }
        inline bool IsNil(element_pointer x) const {
            return x == sentinel;
        }

        void resize(size_type n) {

            if (n > _size) { //increase size => append empty elements
                for (size_type i = 0;i < n - _size; i++) {
                    element_pointer tmp = new elementType();
                    if (sentinel->next_ == sentinel) {
                        head_ = tmp;
                        tmp->makeLink(sentinel, sentinel->next_);
                    }
                    else {
                        tmp->makeLink(tail_, sentinel);
                    }
                    tail_ = tmp;
                }
            }
            else { //decrease size => remove elements from the end
                element_pointer prev = head_;

                for (size_type i = 0;i < n - 1; i++)
                    prev = getNextElement(prev);

                element_pointer curr = getNextElement(prev);
                element_pointer next = getNextElement(curr);

                for (size_type i = 0;i < _size - n; i++) {
                    erase(prev, curr, next);
                    curr = getNextElement(prev);
                    next = getNextElement(curr);
                }
            }
            _size = n;
        }
        ////////////////////////////////////////////////////////////////////////
        // Element access used by the iterators
        ////////////////////////////////////////////////////////////////////////
        inline element_pointer getREnd() {
            return sentinel;
        }
        listBase(const listBase&) = delete;
        listBase& operator=(const listBase&) = delete;
        void swap(listBase& other) {
            size_type tmpSize = _size; _size = other._size; other._size = tmpSize;
            element_pointer tmpHead = head_; head_ = other.head_; other.head_ = tmpHead;
            element_pointer tmpTail = tail_; tail_ = other.tail_; other.tail_ = tmpTail;
            element_pointer tmpSent = sentinel; sentinel = other.sentinel; other.sentinel = tmpSent;
        }
    protected:
        size_type _size;
        element_pointer tail_, head_;
        element_pointer sentinel;
    };
    //////////////////////////////////////////////////////////////////
    // sll and dll are list interfaces that define getPreviousElement()
    // and get() functions
    //////////////////////////////////////////////////////////////////
    template<class listBase>
    class sll: public listBase {
    public:
        ////////////////////////////////////////////////////////////////////////
        typedef typename listBase::Allocator Allocator;
        typedef typename listBase::value_type value_type;
        ////////////////////////////////////////////////////////////////////////
        typedef typename listBase::input_value_type input_value_type;
        typedef typename listBase::pointer pointer;
        typedef typename listBase::reference reference;
        typedef typename listBase::const_reference const_reference;
        typedef typename listBase::const_input_reference const_input_reference;

        typedef typename listBase::element_pointer element_pointer;
        typedef typename listBase::element_reference element_reference;

        typedef typename listBase::size_type size_type;
        typedef typename listBase::difference_type difference_type;
        
        using listBase::head_;

        sll (){}
        element_pointer getPreviousElement(element_pointer curr) {
            element_pointer tmp = listBase::head_;
            while (!this->IsNil(tmp)) {
                auto next = tmp->next_;
                if (next == curr) break;
                tmp = next;
            }
            return tmp;
        }
        element_pointer decrement(element_pointer data) const {
            element_pointer tmp = head_;
            while (!this->IsNil(tmp)) {
                auto next = tmp->next_;
                if (next == data) break;
                tmp = next;
            }
            return tmp;
        }
        reference get(size_type n) {
            size_type i = 0;
            element_pointer tmp = head_;
            while (i < n) {
                tmp = tmp->next_;
                i++;
            }

            return tmp->getData();
        }
        const_reference get(size_type n) const {
            size_type i = 0;
            element_pointer tmp = head_;
            while (i < n) {
                tmp = tmp->next_;
                i++;
            }

            return tmp->getData();
        }
        element_pointer getElement(size_type n) {
            size_type i = 0;
            element_pointer tmp = head_;
            while (i < n) {
                tmp = tmp->next_;
                i++;
            }
            return tmp;
        }
    };

    template<class listBase>
    class dll: public listBase {
    public:
        ////////////////////////////////////////////////////////////////////////
        typedef typename listBase::Allocator Allocator;
        typedef typename listBase::value_type value_type;
        ////////////////////////////////////////////////////////////////////////
        typedef typename listBase::pointer pointer;
        typedef typename listBase::reference reference;
        typedef typename listBase::const_reference const_reference;

        typedef typename listBase::element_pointer element_pointer;
        typedef typename listBase::element_reference element_reference;

        typedef typename listBase::input_value_type input_value_type;

        typedef typename listBase::size_type size_type;
        typedef typename listBase::difference_type difference_type;

        using listBase::_size;
        using listBase::head_;
        using listBase::tail_;

        dll () {}

        element_pointer getPreviousElement(element_pointer curr) {
            if constexpr (!listBase::HAS_SENTINEL) {
                if (!curr) return tail_;
            }
            return curr->prev_;
        }
        element_pointer decrement(element_pointer data) const {
            if constexpr (!listBase::HAS_SENTINEL) {
                if (!data) return tail_;
            }
            return data->prev_;
        }
        reference get(size_type n) {
            element_pointer tmp;
            size_type i = 0;

            if (n <= _size/2) {
                tmp = head_;
                while (i < n) {
                    tmp = tmp->next_;
                    ++i;
                }
            }
            else {
                tmp = tail_;
                size_type newOffset = _size - n - 1;
                while (i < newOffset) {
                    tmp = tmp->prev_;
                    ++i;
                }
            }

            return tmp->getData();
        }
        const_reference get(size_type n) const {
            element_pointer tmp;
            size_type i = 0;

            if (n <= _size/2) {
                tmp = head_;
                while (i < n) {
                    tmp = tmp->next_;
                    ++i;
                }
            }
            else {
                tmp = tail_;
                size_type newOffset = _size - n - 1;
                while (i < newOffset) {
                    tmp = tmp->prev_;
                    ++i;
                }
            }

            return tmp->getData();
        }

        element_pointer getElement(size_type n) {
            element_pointer tmp;
            size_type i = 0;

            if (n <= _size/2) {
                tmp = head_;
                while (i < n) {
                    tmp = tmp->next_;
                    ++i;
                }
            }
            else {
                tmp = tail_;
                size_type newOffset = _size - n - 1;
                while (i < newOffset) {
                    tmp = tmp->prev_;
                    ++i;
                }
            }
            return tmp;
        }
    };

    ///////////////////////////////////////////////////////////////
    // Extensions. These classes add functionality to the list. Some
    // of them, like sllBiIteratorAdapter are used to handle the new
    // functionality introduced in specific iterators like sllBiIterator.
    ///////////////////////////////////////////////////////////////
    template<class listType>
    class sllRovingExtension: public listType {
    public:
        ////////////////////////////////////////////////////////////////////////
        typedef typename listType::Allocator Allocator;
        typedef typename listType::value_type value_type;
        ////////////////////////////////////////////////////////////////////////
        typedef typename listType::pointer pointer;
        typedef typename listType::reference reference;
        typedef typename listType::const_reference const_reference;

        typedef typename listType::element_pointer element_pointer;
        typedef typename listType::element_reference element_reference;

        typedef typename listType::input_value_type input_value_type;

        typedef typename listType::size_type size_type;
        typedef typename listType::difference_type difference_type;

        sllRovingExtension()
            : roving(listType::getFirstElement()), rovingOffset(0) {
        }

        void reset_roving() {
            roving = listType::getFirstElement();
            rovingOffset = 0;
        }
        void initialize() {
            listType::initialize();
            reset_roving();
        }
        void insert_after(const input_value_type& value, element_pointer prev) {
            listType::insert_after(value, prev);
            reset_roving();
        }

        void swap(sllRovingExtension& other) {
            listType::swap(other);
            element_pointer tmpRoving = roving; roving = other.roving; other.roving = tmpRoving;
            size_type tmpOffset = rovingOffset; rovingOffset = other.rovingOffset; other.rovingOffset = tmpOffset;
        }

        void erase(element_pointer prev_node, element_pointer curr,
                   element_pointer next_node) {
            listType::erase(prev_node, curr, next_node);
            reset_roving();
        }

        reference get(size_type n)  {

            size_type i = rovingOffset;
            element_pointer tmp = roving;
            if (this->IsNil(tmp)) {
                tmp = listType::getFirstElement();

                rovingOffset = 0;

                roving = tmp;
            }
            if (i != n) {
                if (i < n) {
                    while (i != n) {
                        tmp = tmp->next_;
                        i++;
                    }
                }
                else {
                    tmp = listType::getFirstElement();
                    i = 0;
                    while (i != n) {
                        tmp = tmp->next_;
                        i++;
                    }
                }

                rovingOffset = n;

                roving = tmp;
                return tmp->getData();
            }
            else {
                return tmp->getData();
            }
        }

        const_reference get(size_type n) const {

            size_type i = rovingOffset;
            element_pointer tmp = roving;
            if (this->IsNil(tmp)) {
                tmp = listType::getFirstElement();

                const_cast<sllRovingExtension*>(this)->rovingOffset = 0;

                const_cast<sllRovingExtension*>(this)->roving = tmp;
            }
            if (i != n) {
                if (i < n) {
                    while (i != n) {
                        tmp = tmp->next_;
                        i++;
                    }
                }
                else {
                    tmp = listType::getFirstElement();
                    i = 0;
                    while (i != n) {
                        tmp = tmp->next_;
                        i++;
                    }
                }

                const_cast<sllRovingExtension*>(this)->rovingOffset = n;

                const_cast<sllRovingExtension*>(this)->roving = tmp;
                return tmp->getData();
            }
            else {
                return tmp->getData();
            }
        }

        mutable element_pointer roving;
        mutable size_type rovingOffset;
    };

    template<class listType>
    class dllRovingExtension: public listType {
    public:
        ////////////////////////////////////////////////////////////////////////
        typedef typename listType::Allocator Allocator;
        typedef typename listType::value_type value_type;
        ////////////////////////////////////////////////////////////////////////
        typedef typename listType::pointer pointer;
        typedef typename listType::reference reference;
        typedef typename listType::const_reference const_reference;

        typedef typename listType::element_pointer element_pointer;
        typedef typename listType::element_reference element_reference;

        typedef typename listType::input_value_type input_value_type;

        typedef typename listType::size_type size_type;
        typedef typename listType::difference_type difference_type;

        dllRovingExtension()
            : roving(listType::getFirstElement()), rovingOffset(0) {
        }

        void reset_roving() {
            roving = listType::getFirstElement();
            rovingOffset = 0;
        }
        void initialize() {
            listType::initialize();
            reset_roving();
        }
        void insert_after(const input_value_type& value, element_pointer prev) {
            listType::insert_after(value, prev);
            reset_roving();
        }

        void swap(dllRovingExtension& other) {
            listType::swap(other);
            element_pointer tmpRoving = roving; roving = other.roving; other.roving = tmpRoving;
            size_type tmpOffset = rovingOffset; rovingOffset = other.rovingOffset; other.rovingOffset = tmpOffset;
        }

        void erase(element_pointer prev_node, element_pointer curr,
                   element_pointer next_node) {
            listType::erase(prev_node, curr, next_node);
            reset_roving();
        }

        reference get(size_type n) {

            size_type i = rovingOffset;
            size_type newOffset = 0;
            size_type numberofObjects = listType::getSize();
            element_pointer tmp = roving;

            if (this->IsNil(tmp)) {
                tmp = listType::getFirstElement();

                rovingOffset = 0;

                roving = tmp;
            }
            if (i != n) {
                if (i < n) {
                    if (n - i <= numberofObjects - n) {
                        newOffset = n - i;
                        i = 0;
                        while (i < newOffset) {
                            tmp = tmp->next_;
                            i++;
                        }
                    }
                    else
                    {
                        newOffset = numberofObjects - n - 1;
                        tmp = listType::getLastElement();
                        i = 0;
                        while (i < newOffset) {
                            tmp = tmp->prev_;
                            i++;
                        }
                    }
                }
                else {
                    if (n <= i/2)
                    {
                        newOffset = n;
                        tmp = listType::getFirstElement();
                        i = 0;
                        while (i < newOffset) {
                            tmp = tmp->next_;
                            i++;
                        }
                    }
                    else {
                        newOffset = i - n;
                        i = 0;
                        while (i < newOffset) {
                            tmp = tmp->prev_;
                            i++;
                        }
                    }
                }

                rovingOffset = n;

                roving = tmp;
                return tmp->getData();
            }
            else {
                return tmp->getData();
            }
        }

        const_reference get(size_type n) const {

            size_type i = rovingOffset;
            size_type newOffset = 0;
            size_type numberofObjects = listType::getSize();
            element_pointer tmp = roving;

            if (this->IsNil(tmp)) {
                tmp = listType::getFirstElement();

                const_cast<dllRovingExtension*>(this)->rovingOffset = 0;

                const_cast<dllRovingExtension*>(this)->roving = tmp;
            }
            if (i != n) {
                if (i < n) {
                    if (n - i <= numberofObjects - n) {
                        newOffset = n - i;
                        i = 0;
                        while (i < newOffset) {
                            tmp = tmp->next_;
                            i++;
                        }
                    }
                    else
                    {
                        newOffset = numberofObjects - n - 1;
                        tmp = listType::getLastElement();
                        i = 0;
                        while (i < newOffset) {
                            tmp = tmp->prev_;
                            i++;
                        }
                    }
                }
                else {
                    if (n <= i/2)
                    {
                        newOffset = n;
                        tmp = listType::getFirstElement();
                        i = 0;
                        while (i < newOffset) {
                            tmp = tmp->next_;
                            i++;
                        }
                    }
                    else {
                        newOffset = i - n;
                        i = 0;
                        while (i < newOffset) {
                            tmp = tmp->prev_;
                            i++;
                        }
                    }
                }

                const_cast<dllRovingExtension*>(this)->rovingOffset = n;

                const_cast<dllRovingExtension*>(this)->roving = tmp;
                return tmp->getData();
            }
            else {
                return tmp->getData();
            }
        }

        mutable element_pointer roving;
        mutable size_type rovingOffset;
    };

    ///////////////////////////////////////////////////////////////
    // This is the main handler for the list abstract data type
    ///////////////////////////////////////////////////////////////
    template <class listType>
    class List: public listType {
    public:
        ////////////////////////////////////////////////////////////////////////
        typedef typename listType::Allocator Allocator;
        typedef typename listType::value_type value_type;
        ////////////////////////////////////////////////////////////////////////
        typedef typename listType::pointer pointer;
        typedef typename listType::reference reference;
        typedef typename listType::const_reference const_reference;
        typedef typename listType::const_input_reference const_input_reference;
        
        typedef typename listType::element_pointer element_pointer;
        typedef typename listType::element_reference element_reference;

        typedef typename listType::input_value_type input_value_type;

        typedef typename listType::size_type size_type;
        typedef typename listType::difference_type difference_type;

        typedef Iterator_Base<List> iterator;
        typedef ConstIteratorAdapter<iterator> const_iterator;

        typedef ReverseIteratorAdapter<iterator> reverse_iterator;
        typedef ReverseIteratorAdapter<const_iterator> const_reverse_iterator;

        // Copies of elements; get(n), operator[] and front()/back() return references.
        input_value_type read(size_type n) { return listType::get(n); }
        input_value_type read(size_type n) const { return listType::get(n); }
        void write(size_type n, const_input_reference value) { (*this)[n] = value; }

        List() {
        }
        List(size_type sz) {
            resize(sz);
        }
        List(size_type sz, const_input_reference value) {
            resize(sz, value);
        }
        List(iterator first, iterator last) {
            for (;first != last; first++) {
                push_back(*first);
            }
        }

        void clear() {

            if (!listType::empty()) {

                element_pointer ePointer = listType::getFirstElement();
                element_pointer endPointer = listType::getEnd();
                while (ePointer != endPointer) {
                    element_pointer toDelete = ePointer;
                    ePointer = ePointer->next_;
                    delete toDelete;
                }

            }
            listType::initialize();
        }

        List(const List& other) : List() { copy_from(other); }

        template <class listType2>
        List(const List<listType2>& other) : List() { copy_from(other); }

        // Standard range assignment. The input range must not belong to *this.
        // Consume input iterators once; do not build a staging container.
        template<class InputIterator>
        void assign(InputIterator first, InputIterator last) {
            clear();
            for (; first != last; ++first)
                push_back(*first);
        }

        List& operator=(const List& other) {
            if (this != &other) {
                List temporary(other);
                swap(temporary);
            }
            return *this;
        }

        template <class listType2>
        List& operator=(const List<listType2>& other) {
            List temporary(other);
            swap(temporary);
            return *this;
        }

        ~List() {
            clear();

        }

        ////////////////////////////////////////////////////////////////////////
        // Basic iterators of DDT
        ////////////////////////////////////////////////////////////////////////

        iterator begin() const {
            iterator it(this, listType::getFirstElement());
            return it;
        }
        iterator end() const {
            iterator it(this, listType::getEnd());
            return it;
        }

        ////////////////////////////////////////////////////////////////////////
        // Reverse iterators of DDT
        ////////////////////////////////////////////////////////////////////////

        reverse_iterator rbegin() const {
            reverse_iterator it(this, listType::getLastElement());
            return it;
        }
        reverse_iterator rend() const {
            reverse_iterator it(this, listType::getEnd());
            return it;
        }

        ////////////////////////////////////////////////////////////////////////
        // Size information
        ////////////////////////////////////////////////////////////////////////
        inline bool empty() const {
            return listType::empty();
        }
        inline size_type size() const {
            return listType::getSize();
        }

        size_t footprint() {
            const size_t node_bytes = sizeof(typename listType::listElementType);
            const size_t data_bytes = sizeof(input_value_type);
            size_t total = sizeof(*this) + listType::getSize() * node_bytes;
#ifdef OUT_OF_LINE_STORAGE
            total += listType::getSize() * data_bytes;
#endif
            if (listType::HAS_SENTINEL) total += node_bytes;
            return total;
        }

        void resize(size_type new_size, const_input_reference value = input_value_type()) {
            const size_type old_size = size();
            if (new_size > old_size) {
                for (size_type i = old_size;i < new_size; i++) {
                    push_back(value);
                }
            } else if (new_size < old_size) {
                for (size_type i = new_size;i < old_size; i++) {
                    pop_back();
                }
            }
        }

        inline size_type capacity() {
            return listType::getSize();
        }

        // Stable bottom-up merge sort: relink nodes, never move/copy payloads.
        void sort() {
            const auto n=size();
            if(n<2) return;
            using Node=element_pointer;
            const Node stop=listType::getEnd();
            auto next=[&](Node node) { return listType::getNextElement(node); };
            auto link=[&](Node node,Node successor) {
                node->next_=successor;
            };
            auto previous=[&](Node node,Node predecessor) {
                if constexpr(std::is_same<typename listType::listElementType,
                                           dllListElement<input_value_type,Allocator>>::value) {
                    node->prev_=predecessor;
                }
            };
            auto finish=[&](Node head,Node tail) {
                link(tail,stop);
                this->head_=head; this->tail_=tail;
                if constexpr(listType::HAS_SENTINEL) link(stop,head);
                Node prev=stop;
                for(Node node=head;node!=stop;node=next(node)) {
                    previous(node,prev); prev=node;
                }
                if constexpr(listType::HAS_SENTINEL) previous(stop,tail);
                listType::reset_roving();
            };
            Node head=listType::getFirstElement();
            for(size_type width=1;width<n;) {
                Node p=head, merged=stop, tail=stop;
                auto append=[&](Node node) {
                    if(tail==stop) merged=node;
                    else link(tail,node);
                    tail=node;
                };
                while(p!=stop) {
                    Node q=p;
                    size_type left=0;
                    for(;left<width && q!=stop;++left) q=next(q);
                    size_type right=width;
                    try {
                        while(left!=0 ||
                              (right!=0 && q!=stop)) {
                            Node selected;
                            if(left==0) {
                                selected=q; q=next(q); --right;
                            } else if(right==0 || q==stop) {
                                selected=p; p=next(p); --left;
                            } else if(q->getData() < p->getData()) {
                                selected=q; q=next(q); --right;
                            } else {
                                selected=p; p=next(p); --left;
                            }
                            append(selected);
                        }
                    } catch(...) {
                        // Retain every node and restore valid sentinel/prev links.
                        while(left) {Node node=p;p=next(p);--left;append(node);}
                        while(q!=stop) {Node node=q;q=next(q);append(node);}
                        finish(merged,tail);
                        throw;
                    }
                    p=q;
                }
                finish(merged,tail);
                head=merged;
                if(width>n/2) break;
                width*=2;
            }
        }

        void reserve(size_type new_capacity) {
        }

        reference operator[](size_type n) {
            return listType::get(n);
        }
        const_reference operator[](size_type n) const {
            return listType::get(n);
        }
        reference get(size_type n)  {

            return listType::get(n);
        }
        
        element_pointer getElement(size_type n) {
            return listType::getElement(n);
        }
        
        const_reference front() const {
            return listType::getFirstElement()->getData();
        }
        reference front() {
            return listType::getFirstElement()->getData();
        }
        const_reference back() const {
            return listType::getLastElement()->getData();
        }
        reference back() {
            return listType::getLastElement()->getData();
        }
        
        void push_back(const_input_reference value) {
            listType::push_back(value);
        }
        void pop_back() {
            element_pointer curr = listType::getLastElement();
            if (!(this->IsNil(curr))) {
                element_pointer prev_node = listType::getPreviousElement(curr);
                element_pointer next_node = listType::getEnd();
                listType::erase(prev_node, curr, next_node);
                delete curr;
            }
        }
        void pop_front() {
            element_pointer curr = listType::getFirstElement();
            if (!(this->IsNil(curr))) {
                element_pointer prev_node = listType::getEnd();
                element_pointer next_node = listType::getNextElement(curr);
                listType::erase(prev_node, curr, next_node);
                delete curr;
            }
        }

        ////////////////////////////////////////////////////////////////////////
        // Element access used by the iterators
        ////////////////////////////////////////////////////////////////////////
        bool exists(element_pointer x) {
            if (!listType::empty()) {
                Iterator_Base<List> it1(this, listType::getFirstElement());
                Iterator_Base<List> it2(this, listType::getEnd());
                for (Iterator_Base<List> it = it1;it != it2; it++) {
                    if (it.getData() == x){
                        return true;
                    }
                }
            }
            return false;
        }
        
        // Sorted-sequence insertion, before equivalent values. One forward walk;
        // retain the predecessor so single-linked lists do not search twice.
        template<class Compare>
        void insert_sorted(const_input_reference value, Compare less) {
            auto current = listType::getFirstElement();
            auto previous = listType::getEnd();
            while (!this->IsNil(current) &&
                   less(current->getData(), value)) {
                previous = current;
                current = listType::increment(current);
            }
            if (this->IsNil(previous)) add(0, value);
            else listType::insert_after(value, previous);
        }

        // Inserts after the current position indexed by the iterator
        void insert_after(const_input_reference value, iterator& pos)
        {
            element_pointer curr = pos.getData();
            if (!(this->IsNil(curr))) {
                listType::insert_after(value, curr);
            }

        }
        void insert_after(const_input_reference value, element_pointer item) {
            if (!(this->IsNil(item))) {
                listType::insert_after(value, item);
            }
        }
        void add(size_type n, const_input_reference value) {
            if (n == 0) {
                element_pointer tmp = new typename listType::listElementType(value);
                element_pointer old_head = listType::getFirstElement();
                tmp->makeLink(listType::getEnd(), old_head);
                listType::head_ = tmp;
                if (listType::_size == 0) {
                    listType::tail_ = tmp;
                }
                listType::_size++;
                listType::reset_roving();
            } else {
                element_pointer prev = listType::getElement(n - 1);
                listType::insert_after(value, prev);
            }
        }
        void remove(size_type n) {
            element_pointer curr = listType::getElement(n);
            element_pointer prev_node = listType::getPreviousElement(curr);
            element_pointer next_node = curr->next_;
            listType::erase(prev_node, curr, next_node);
            delete curr;
        }
        void remove_value(const_input_reference value) {
            element_pointer prev = listType::getEnd();
            element_pointer curr = listType::getFirstElement();
            while (!(this->IsNil(curr))) {
                pointer dp = listType::getData(curr);
                if (*dp == value) {
                    element_pointer next = listType::getNextElement(curr);
                    listType::erase(prev, curr, next);
                    delete curr;
                    return;
                }
                prev = curr;
                curr = listType::getNextElement(curr);
            }

        }
        iterator erase(iterator pos) {
            element_pointer curr = pos.getData();
            element_pointer next_node = curr->next_;
            if (!(this->IsNil(curr))) {
                element_pointer prev_node = listType::getPreviousElement(curr);
                listType::erase(prev_node, curr, next_node);
                delete curr;
            }

            return iterator(this, next_node);
        }
        
        reverse_iterator erase(reverse_iterator pos) {
            element_pointer curr = pos.getData();
            element_pointer prev_node = listType::getPreviousElement(curr);
            if (!(this->IsNil(curr))) {
                element_pointer next_node = curr->next_;
                listType::erase(prev_node, curr, next_node);
                delete curr;
            }

            return reverse_iterator(this, prev_node);
        }
        
        element_pointer erase(element_pointer item) {
            element_pointer curr = item;
            element_pointer next_node = curr->next_;
            if (!(this->IsNil(curr))) {
                element_pointer prev_node = listType::getPreviousElement(curr);
                listType::erase(prev_node, curr, next_node);
                delete curr;
            }

            return next_node;
        }
        
        iterator erase(iterator first, iterator last) {

            if (first != last) {
                    element_pointer firstNode = first.getData();
                    element_pointer lastNode = last.getData();
                    element_pointer next_node = lastNode;
                    
                        element_pointer prev_node = listType::getPreviousElement(firstNode);
                        listType::erase(prev_node, firstNode, next_node);
                        
                        //destruct and delete all the removed elements
                        bool first_node = true;
                        while (firstNode != lastNode) {
                            element_pointer currNode = firstNode;
                            firstNode = firstNode->next_;
                            delete currNode;
                            if (!first_node) {
                                listType::_size--;
                            }
                            first_node = false;
                        }

            }
            return last;
        }
        
        void
        printSentinel() {
            std::cout << "sentinel = " << this->sentinel
            << " " << "sentinel->data_->index = " << this->sentinel->data_.index
            << " " << "sentinel->data_->f1 = " << this->sentinel->data_.f1
            << " " << "sentinel->data_->f2 = " << this->sentinel->data_.f2
                << " " << std::endl;
        }
        
        void swap(List& other) {
            listType::swap(other);
        }
        
    private:
        template <class listType2>
        void copy_from(const List<listType2>& other) {
            typename List<listType2>::iterator it(other.begin());
            const typename List<listType2>::iterator end(other.end());
            for (;it != end; ++it) push_back(*it);
        }

        template <class> friend class List;

    };

}
////////////////////////////////////////////////////////////////////////////////
// End namespace DDTLibrary
////////////////////////////////////////////////////////////////////////////////
#endif
