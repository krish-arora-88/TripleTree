/**
 * @file        tripletree_given.h
 * @description Instructor-provided implementation for given ternary
 *              tree functions for CPSC 221 PA3.
 *
 *              THIS FILE WILL NOT BE MODIFIED/SUBMITTED FOR GRADING
 */

#include "tripletree.h"

/**
 * TripleTree destructor.
 * Destroys all of the memory associated with the
 * current TripleTree. This function should ensure that
 * memory does not leak on destruction of a TripleTree.
 *
 * @see TripleTree.cpp
 */
TripleTree::~TripleTree() {
	Clear();
}

/**
 * Copy constructor for a TripleTree.
 * Since TripleTree allocate dynamic memory (i.e., they use "new", we
 * must define the Big Three). This uses your implementation
 * of the copy function.
 * @see TripleTree.cpp
 *
 * @param other - the TripleTree we are copying.
 */
TripleTree::TripleTree(const TripleTree& other) {
	Copy(other);
}

/**
 * Overloaded assignment operator for TripleTree.
 * Part of the Big Three that we must define because the class
 * allocates dynamic memory. This uses your implementation
 * of the copy and clear funtions.
 *
 * @param rhs - the right hand side of the assignment statement.
 */
TripleTree& TripleTree::operator=(const TripleTree& rhs) {
	// only take action if this object is not living at the same address as rhs
	// i.e. this and rhs are physically different trees
	if (this != &rhs) {
		// release any previously existing memory associated with this tree
		Clear();
		root = nullptr;
		// and then copy the other tree
		Copy(rhs);
	}
	return *this;
}