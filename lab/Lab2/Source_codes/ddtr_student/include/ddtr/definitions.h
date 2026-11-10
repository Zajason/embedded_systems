#ifndef DDTR_TYPES_H
#define DDTR_TYPES_H

#include "allocator.hpp"
#include "Vector.h"
#include "List.h"
#include "Tree.h"
#include "BTree.h"

template <typename T, class ALLOC = ddtr::malloc_allocator>
struct ddt_helper {
	//List helper definitions
	typedef DDTLibrary::sllListElement<T, ALLOC> SLLELEMENT;
	typedef DDTLibrary::dllListElement<T, ALLOC> DLLELEMENT;
	typedef DDTLibrary::listBase<SLLELEMENT> SLLLISTBASE;
	typedef DDTLibrary::listBase<DLLELEMENT> DLLLISTBASE;
	typedef DDTLibrary::listBaseNoSentinel<SLLELEMENT> SLL_NOSENTINEL_LISTBASE;
	typedef DDTLibrary::listBaseNoSentinel<DLLELEMENT> DLL_NOSENTINEL_LISTBASE;
	typedef DDTLibrary::sll<SLLLISTBASE> SLLLISTTYPE;
	typedef DDTLibrary::dll<DLLLISTBASE> DLLLISTTYPE;
	typedef DDTLibrary::sll<SLL_NOSENTINEL_LISTBASE> SLLLIST_NOSENTINEL_TYPE;
	typedef DDTLibrary::dll<DLL_NOSENTINEL_LISTBASE> DLLLIST_NOSENTINEL_TYPE;

	//Tree helper definitions
	typedef DDTLibrary::BinaryTreeNode<T, ALLOC> BINARYTREENODE;
	typedef DDTLibrary::RedBlackTreeNode<T, ALLOC> REDBLACKTREENODE;
	typedef DDTLibrary::AVLTreeNode<T, ALLOC> AVLTREENODE;
	typedef DDTLibrary::BinaryTree<BINARYTREENODE> BINARYTREEBASE;
	typedef DDTLibrary::BinaryTree<REDBLACKTREENODE> REDBLACKTREE_BASE;
	typedef DDTLibrary::RedBlackTreeExtension<REDBLACKTREE_BASE> REDBLACKTREEBASE;
	typedef DDTLibrary::BinaryTree<AVLTREENODE> AVLTREE_BASE;
	typedef DDTLibrary::AVLTreeExtension<AVLTREE_BASE> AVLTREEBASE;

	//DDT definitions
	typedef DDTLibrary::Vector<T, ALLOC> VECTOR;
	typedef DDTLibrary::List<SLLLISTTYPE> SLLLIST;
	typedef DDTLibrary::List<DLLLISTTYPE> DLLLIST;
	typedef DDTLibrary::List<DDTLibrary::sllRovingExtension<SLLLISTTYPE> > SLLROVINGLIST;
	typedef DDTLibrary::List<DDTLibrary::dllRovingExtension<DLLLISTTYPE> > DLLROVINGLIST;
	typedef DDTLibrary::List<SLLLIST_NOSENTINEL_TYPE> SLLLIST_NOSENTINEL;
	typedef DDTLibrary::List<DLLLIST_NOSENTINEL_TYPE> DLLLIST_NOSENTINEL;
	typedef DDTLibrary::List<DDTLibrary::sllRovingExtension<SLLLIST_NOSENTINEL_TYPE> > SLLNOSENTINELROVINGLIST;
	typedef DDTLibrary::List<DDTLibrary::dllRovingExtension<DLLLIST_NOSENTINEL_TYPE> > DLLNOSENTINELROVINGLIST;
	typedef DDTLibrary::Tree<BINARYTREEBASE> BINARYTREE;
	typedef DDTLibrary::Tree<REDBLACKTREEBASE> REDBLACKTREE;
	typedef DDTLibrary::Tree<AVLTREEBASE> AVLTREE;
};

#endif
