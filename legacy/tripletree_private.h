/**
 * @file        tripletree_private.h
 * @description student declarations of private functions
 *              for TripleTree, CPSC 221 PA3
 *
 *              THIS FILE WILL BE SUBMITTED.
 *
 *              Simply declare your function prototypes here.
 *              No other scaffolding is necessary.
 */

 // begin your declarations below

// In tripletree_private.h or wherever the class TripleTree is declared


int NumLeaves_helper(const Node* sub_root) const;

void color_pixels(const Node* sub_root, PNG* img) const;

Node* Copy_helper(const Node* other_sub_tree);

void Prune_sub_tree(double tol, Node* sub_tree);

void Clear_sub_tree(Node* sub_tree);

bool Leaves_Prunable(double tol, RGBAPixel pixel, Node* sub_tree);

Node* Flip_helper(Node* sub_tree);

void update_leaves(Node* sub_tree);

Node* rotate_helper(Node* sub_tree);

void swap_width_height(Node* sub_tree);