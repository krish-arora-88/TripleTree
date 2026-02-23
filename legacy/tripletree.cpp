/**
 * @file        tripletree.cpp
 * @description Student-implemented functions of a ternary tree for CPSC 221 PA3.
 *
 *              THIS FILE WILL BE SUBMITTED FOR GRADING
 */

#include "tripletree.h"

 /**
      * Constructor that builds a TripleTree out of the given PNG.
      *
      * The TripleTree represents the subimage from (0,0) to (w-1, h-1) where
      * w-1 and h-1 are the largest valid image coordinates of the original PNG.
      * Every node corresponds to a rectangle of pixels in the original PNG,
      * represented by an (x,y) pair for the upper left corner of the
      * square and two integers for the number of pixels on the width and
      * height dimensions of the rectangular region the node defines.
      *
      * The node's three children correspond to a partition
      * of the node's rectangular region into three approximately equal-size strips.
      *
      * If the rectangular region is taller than it is wide, the region is divided
      * into horizontal strips:
      *  +-------+
      *  |   A   |
      *  |       |
      *  +-------+
      *  |   B   |
      *  |       |    (Split tall)
      *  +-------+
      *  |   C   |
      *  |       |
      *  +-------+
      *
      * If the rectangular region is wider than it is tall, the region is divided
      * into vertical strips:
      *  +---------+---------+---------+
      *  |    A    |    B    |    C    |
      *  |         |         |         |    (Split wide)
      *  +---------+---------+---------+
      *
      * Your regions are not guaranteed to have dimensions exactly divisible by 3.
      * If the dimensions of your rectangular region are 3p x q or q x 3p where 3p
      * represents the length of the long side, then your rectangular regions will
      * each have dimensions p x q (or q x p)
      *
      * If the dimensions are (3p+1) x q, subregion B gets the extra pixel of size
      * while subregions A and C have dimensions p x q.
      *
      * If the dimensions are (3p+2) x q, subregions A and C each get an extra pixel
      * of size, while subregion B has dimensions p x q.
      *
      * If the region to be divided is a square, then apply the Split wide behaviour.
      *
      * Every leaf in the constructed tree corresponds to a pixel in the PNG.
      *
      * @param imIn - the input image used to construct the tree
      */
TripleTree::TripleTree(PNG& imIn) {
    pair<unsigned int, unsigned int> ul;
    ul.first = 0;
    ul.second = 0;
    root = BuildNode(imIn, ul, imIn.width(), imIn.height());
	
}

/**
 * Render returns a PNG image consisting of the pixels
 * stored in the tree. It may be used on pruned trees. Draws
 * every leaf node's rectangle onto a PNG canvas using the
 * average color stored in the node.
 *
 * You may want a recursive helper function for this.
 */
PNG TripleTree::Render() const {
    if (root != NULL) {
        PNG img = PNG(root->width, root->height);
        color_pixels(root, &img);
        return img;
    }
    return PNG();
}

void TripleTree::color_pixels(const Node* sub_tree, PNG *img) const {
    if (sub_tree != nullptr) {
        if (sub_tree->A == nullptr && sub_tree->B == nullptr && sub_tree->C == nullptr) {
            for (unsigned int x = sub_tree->upperleft.first; x < sub_tree->upperleft.first + sub_tree->width; x++) {
                for (unsigned int y = sub_tree->upperleft.second; y < sub_tree->upperleft.second + sub_tree->height; y++) {
                    RGBAPixel *p = img->getPixel(x, y);
                    p->r = sub_tree->avg.r;
                    p->g = sub_tree->avg.g;
                    p->b = sub_tree->avg.b;
                    p->a = sub_tree->avg.a;
                }
            }
        } else {
            color_pixels(sub_tree->A, img);
            color_pixels(sub_tree->B, img);
            color_pixels(sub_tree->C, img);
        }
    }

}


/*
 * Prune function trims subtrees as high as possible in the tree.
 * A subtree is pruned (cleared) if all of its leaves are within
 * tol of the average color stored in the root of the subtree.
 * Pruning criteria should be evaluated on the original tree, not
 * on a pruned subtree. (we only expect that trees would be pruned once.)
 *
 * You may want a recursive helper function for this.
 *
 * @param tol - maximum allowable RGBA color distance to qualify for pruning
 */
void TripleTree::Prune(double tol) {
    Prune_sub_tree(tol, root);
}

void TripleTree::Prune_sub_tree(double tol, Node* sub_tree) {
    if (sub_tree == nullptr) {
        return;
    }
    if (Leaves_Prunable(tol, sub_tree->avg, sub_tree)) {
            Clear_sub_tree(sub_tree->A);
            sub_tree->A = nullptr;
            Clear_sub_tree(sub_tree->B);
            sub_tree->B = nullptr;
            Clear_sub_tree(sub_tree->C);
            sub_tree->C = nullptr;
    } else {
        Prune_sub_tree(tol, sub_tree->A);
        Prune_sub_tree(tol, sub_tree->B);
        Prune_sub_tree(tol, sub_tree->C);
    }
}

bool TripleTree::Leaves_Prunable(double tol, RGBAPixel pixel, Node* sub_tree) {
    if (sub_tree == NULL) {
        return true;
    }
    if (sub_tree->A == NULL && sub_tree->B == NULL && sub_tree->C == NULL) {
        // this is a leaf node
        return pixel.distanceTo(sub_tree->avg) <= tol;
    } else {
        // this is not a leaf node, so go deeper
        return Leaves_Prunable(tol, pixel, sub_tree->A) && Leaves_Prunable(tol, pixel, sub_tree->B) && Leaves_Prunable(tol, pixel, sub_tree->C);
    }
}

/**
 * Rearranges the tree contents so that when rendered, the image appears
 * to be mirrored horizontally (flipped over a vertical axis).
 * This may be called on pruned trees and/or previously flipped/rotated trees.
 *
 * You may want a recursive helper function for this.
 */
void TripleTree::FlipHorizontal() {
    Flip_helper(root);
	
}

Node* TripleTree::Flip_helper(Node* sub_tree) {
    if (sub_tree == nullptr) {
        return nullptr;
    }

    if (sub_tree->A != nullptr && sub_tree->C != nullptr) {
        if (sub_tree->A->upperleft.first != sub_tree->C->upperleft.first) {
            unsigned int dist = sub_tree->C->width;
            if (sub_tree->B != nullptr) {
                dist += sub_tree->B->width;
            }
            sub_tree->A->upperleft.first = sub_tree->upperleft.first + dist;
            sub_tree->C->upperleft.first = sub_tree->upperleft.first;

            swap(sub_tree->A, sub_tree->C);
            update_leaves(sub_tree->A);
            update_leaves(sub_tree->C);
        }
    }
    sub_tree->A = Flip_helper(sub_tree->A);
    sub_tree->B = Flip_helper(sub_tree->B);
    sub_tree->C = Flip_helper(sub_tree->C);
    return sub_tree;
}

void TripleTree::update_leaves(Node* sub_tree) {
    if (sub_tree != nullptr) {
        
        if (sub_tree->A != nullptr && sub_tree->C != nullptr) {
            if (sub_tree->A->upperleft.first != sub_tree->C->upperleft.first) {
                unsigned int dist = sub_tree->A->width;
                if (sub_tree->B != nullptr) {
                    sub_tree->B->upperleft.first = sub_tree->upperleft.first + dist;
                    sub_tree->B->upperleft.second = sub_tree->upperleft.second;
                    dist += sub_tree->B->width;
                }
                sub_tree->A->upperleft = sub_tree->upperleft;
                sub_tree->C->upperleft.second = sub_tree->upperleft.second;
                sub_tree->C->upperleft.first = sub_tree->upperleft.first + dist;
                update_leaves(sub_tree->A);
                update_leaves(sub_tree->B);
                update_leaves(sub_tree->C);
            } else {
                unsigned int dist = sub_tree->A->height;
                if (sub_tree->B != nullptr) {
                    sub_tree->B->upperleft.second = sub_tree->upperleft.second + dist;
                    sub_tree->B->upperleft.first = sub_tree->upperleft.first;
                    dist += sub_tree->B->height;
                }
                sub_tree->A->upperleft = sub_tree->upperleft;
                sub_tree->C->upperleft.first = sub_tree->upperleft.first;
                sub_tree->C->upperleft.second = sub_tree->upperleft.second + dist;
                update_leaves(sub_tree->A);
                update_leaves(sub_tree->B);
                update_leaves(sub_tree->C);
            }
        }
        
    }
}

/**
 * Rearranges the tree contents so that when rendered, the image appears
 * to be rotated 90 degrees counter-clockwise.
 * This may be called on pruned trees and/or previously flipped/rotated trees.
 *
 * You may want a recursive helper function for this.
 */
void TripleTree::RotateCCW() {
    swap_width_height(root);
    root = rotate_helper(root);
}
// PRE: ul, width, height of sub_tree is correct
// POST: ul, width, height of sub_tree->A/B/C are correct
// i.e. convert stack to row or row to stack
Node* TripleTree::rotate_helper(Node* sub_tree) {
    if (sub_tree == nullptr) {
        return nullptr;
    }
    if (sub_tree->A == nullptr) {
        return sub_tree;
    }
    swap_width_height(sub_tree->A);
    swap_width_height(sub_tree->B);
    swap_width_height(sub_tree->C);
    if (sub_tree->A->upperleft.first > sub_tree->C->upperleft.first || sub_tree->A->upperleft.second > sub_tree->C->upperleft.second) {
        swap(sub_tree->A, sub_tree->C);
    }
    if (sub_tree->A->upperleft.first == sub_tree->C->upperleft.first) {
        // A B C are a stack
        unsigned int dist = sub_tree->A->width;
        if (sub_tree->B != nullptr) {
            sub_tree->B->upperleft.first = sub_tree->upperleft.first + dist;
            sub_tree->B->upperleft.second = sub_tree->upperleft.second;
            dist += sub_tree->B->width;
        }
        sub_tree->A->upperleft = sub_tree->upperleft;
        sub_tree->C->upperleft.second = sub_tree->upperleft.second;
        sub_tree->C->upperleft.first = sub_tree->upperleft.first + dist;       
    } else {
        // A B C are a row
        unsigned int dist = sub_tree->C->height;
        if (sub_tree->B != nullptr) {
            sub_tree->B->upperleft.second = sub_tree->upperleft.second + dist;
            sub_tree->B->upperleft.first = sub_tree->upperleft.first;
            dist += sub_tree->B->height;
        }
        sub_tree->C->upperleft = sub_tree->upperleft;
        sub_tree->A->upperleft.first = sub_tree->upperleft.first;
        sub_tree->A->upperleft.second = sub_tree->upperleft.second + dist;
    }
    if (sub_tree->A->upperleft.first > sub_tree->C->upperleft.first || sub_tree->A->upperleft.second > sub_tree->C->upperleft.second) {
        swap(sub_tree->A, sub_tree->C);
    }
    sub_tree->A = rotate_helper(sub_tree->A);
    sub_tree->B = rotate_helper(sub_tree->B);
    sub_tree->C = rotate_helper(sub_tree->C);
    return sub_tree;
}

void TripleTree::swap_width_height(Node* sub_tree) {
    if (sub_tree != nullptr) {
        unsigned int temp = sub_tree->height;
        sub_tree->height = sub_tree->width;
        sub_tree->width = temp;	
    }
}

/*
 * Returns the number of leaf nodes in the tree.
 *
 * You may want a recursive helper function for this.
 */
int TripleTree::NumLeaves() const {
    return NumLeaves_helper(root);
}

int TripleTree::NumLeaves_helper(const Node* sub_root) const {
    if (sub_root == NULL) {
        return 0;
    }
    if (sub_root->A == NULL &&
        sub_root->B == NULL &&
        sub_root->C == NULL) {
        return 1;
    }

    return NumLeaves_helper(sub_root->A) + NumLeaves_helper(sub_root->B) + NumLeaves_helper(sub_root->C);
}

/**
     * Destroys all dynamically allocated memory associated with the
     * current TripleTree object. To be completed for PA3.
     * You may want a recursive helper function for this one.
     */
void TripleTree::Clear() {
    Clear_sub_tree(root);
    root = nullptr;
}


void TripleTree::Clear_sub_tree(Node* sub_tree) {
    if (sub_tree != NULL) {
        Clear_sub_tree(sub_tree->A);
        Clear_sub_tree(sub_tree->B);
        Clear_sub_tree(sub_tree->C);
        delete sub_tree;
    }
}

/**
 * Copies the parameter other TripleTree into the current TripleTree.
 * Does not free any memory. Called by copy constructor and operator=.
 * You may want a recursive helper function for this one.
 * @param other - The TripleTree to be copied.
 */
void TripleTree::Copy(const TripleTree& other) {
    root = new Node(other.root->upperleft, other.root->width, other.root->height);
    root->A = Copy_helper(other.root->A);
    root->avg = other.root->avg;
    if (other.root->B != nullptr) {
        root->B = Copy_helper(other.root->B);
    }
    root->C = Copy_helper(other.root->C);
}

Node* TripleTree::Copy_helper(const Node* other_sub_tree) {
    if (other_sub_tree == NULL) {
        return NULL;
    } else {
        Node* new_node = new Node((*other_sub_tree).upperleft, (*other_sub_tree).width, (*other_sub_tree).height);
        (*new_node).avg = (*other_sub_tree).avg;
        (*new_node).A = Copy_helper((*other_sub_tree).A);
        (*new_node).B = Copy_helper((*other_sub_tree).B);
        (*new_node).C = Copy_helper((*other_sub_tree).C);
        return new_node;
    }
}

/**
 * Private helper function for the constructor. Recursively builds
 * the tree according to the specification of the constructor.
 * @param im - reference image used for construction
 * @param ul - upper left point of node to be built's rectangle.
 * @param w - width of node to be built's rectangle.
 * @param h - height of node to be built's rectangle.
 */
Node* TripleTree::BuildNode(PNG& im, pair<unsigned int, unsigned int> ul, unsigned int w, unsigned int h) {
    // replace the line below with your implementation
    Node* sub_tree = new Node(ul, w, h);

    if (w == 0 || h == 0) {
        return nullptr;
    }

    if (w == 1 && h == 1) {
        RGBAPixel p = *im.getPixel(ul.first, ul.second);
        (*sub_tree).avg.r = p.r;
        (*sub_tree).avg.g = p.g;
        (*sub_tree).avg.b = p.b; 
        (*sub_tree).avg.a = p.a;
        return sub_tree;
    }

    if (h > w) {
        // split tall
        if (h % 3 == 0) {
            // all equal h
            (*sub_tree).A = BuildNode(im, ul, w, h/3);
            (*sub_tree).B = BuildNode(im, make_pair(ul.first, ul.second + h/3), w, h/3);
            (*sub_tree).C = BuildNode(im, make_pair(ul.first, ul.second + 2*h/3), w, h/3);
        } else if ((h+1) % 3 == 0) {
            // B has one less
            if (h == 2) {
                (*sub_tree).A = BuildNode(im, ul, w, 1);
                (*sub_tree).B = NULL;
                (*sub_tree).C = BuildNode(im, make_pair(ul.first, ul.second + 1), w, 1);
            } else {
                (*sub_tree).A = BuildNode(im, ul, w, (h+1)/3);
                (*sub_tree).B = BuildNode(im, make_pair(ul.first, ul.second + (h+1)/3), w, (h+1)/3-1);
                (*sub_tree).C = BuildNode(im, make_pair(ul.first, ul.second + 2*(h+1)/3 - 1), w, (h+1)/3);
            }
        } else {
            // A and C have one less
            (*sub_tree).A = BuildNode(im, ul, w, (h+2)/3 - 1);
            (*sub_tree).B = BuildNode(im, make_pair(ul.first, ul.second + (h+2)/3 - 1), w, (h+2)/3);
            (*sub_tree).C = BuildNode(im, make_pair(ul.first, ul.second + 2*(h+2)/3 -1), w, (h+2)/3 - 1);
        }
    } else {
        // split wide
        if (w % 3 == 0) {
            // all equal w
            (*sub_tree).A = BuildNode(im, ul, w/3, h);
            (*sub_tree).B = BuildNode(im, make_pair(ul.first+w/3, ul.second), w/3, h);
            (*sub_tree).C = BuildNode(im, make_pair(ul.first+2*w/3, ul.second), w/3, h);
        } else if ((w + 1) % 3 == 0) {
            // B has one less
            if (w == 2) {
                (*sub_tree).A = BuildNode(im, ul, 1, h);
                (*sub_tree).B = NULL;
                (*sub_tree).C = BuildNode(im, make_pair(ul.first + 1, ul.second), 1, h);
            } else {
                (*sub_tree).A = BuildNode(im, ul, (w + 1)/3, h);
                (*sub_tree).B = BuildNode(im, make_pair(ul.first + (w + 1)/3, ul.second), (w + 1)/3-1, h);
                (*sub_tree).C = BuildNode(im, make_pair(ul.first + 2*(w + 1)/3-1, ul.second), (w + 1)/3, h);
            }
        } else {
            // A and C have one less
            (*sub_tree).A = BuildNode(im, ul, (w + 2)/3-1, h);
            (*sub_tree).B = BuildNode(im, make_pair(ul.first + (w + 2)/3 - 1, ul.second), (w + 2)/3, h);
            (*sub_tree).C = BuildNode(im, make_pair(ul.first + 2*(w + 2)/3 - 1, ul.second), (w + 2)/3-1, h);
        }
    }
    unsigned int parentArea = ((*sub_tree).width * (*sub_tree).height);
    double weightA = ((*sub_tree).A->width * (*sub_tree).A->height);
    double weightC = (double)((*sub_tree).C->width * (*sub_tree).C->height);
    if ((*sub_tree).B == NULL) {
        (*sub_tree).avg.r = ((*sub_tree).A->avg.r * (weightA) + (*sub_tree).C->avg.r * (weightC))/parentArea;
        (*sub_tree).avg.g = ((*sub_tree).A->avg.g * (weightA) + (*sub_tree).C->avg.g * (weightC))/parentArea;
        (*sub_tree).avg.b = ((*sub_tree).A->avg.b * (weightA) + (*sub_tree).C->avg.b * (weightC))/parentArea;
        (*sub_tree).avg.a = ((*sub_tree).A->avg.a * (weightA) + (*sub_tree).C->avg.a * (weightC))/parentArea;
    } else {
        double weightB = ((*sub_tree).B->width * (*sub_tree).B->height);
        (*sub_tree).avg.r = ((*sub_tree).A->avg.r * (weightA) + (*sub_tree).B->avg.r * (weightB) + (*sub_tree).C->avg.r * (weightC))/parentArea;
        (*sub_tree).avg.g = ((*sub_tree).A->avg.g * (weightA) + (*sub_tree).B->avg.g * (weightB) + (*sub_tree).C->avg.g * (weightC))/parentArea;
        (*sub_tree).avg.b = ((*sub_tree).A->avg.b * (weightA) + (*sub_tree).B->avg.b * (weightB) + (*sub_tree).C->avg.b * (weightC))/parentArea;
        (*sub_tree).avg.a = ((*sub_tree).A->avg.a * (weightA) + (*sub_tree).B->avg.a * (weightB) + (*sub_tree).C->avg.a * (weightC))/parentArea;
    }

    return sub_tree;
}