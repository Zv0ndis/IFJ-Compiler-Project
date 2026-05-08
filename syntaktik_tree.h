/* 
* syntaktik_tree.h
* ============================================================
 * Symbol table implementation for the IFJ25 project.
 * Implements a general tree structure for storing tokens, where each node can have a dynamic number of children.
 * Each node may represent a token and can have zero or more child nodes.
 * ============================================================
 * Authors: 
 * Jakub Kosinka xkosinj00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#ifndef SYNTAKTIK_TREE_H
#define SYNTAKTIK_TREE_H


#include "token.h"
#include <stdbool.h>
#include <stdlib.h>

typedef struct SyntaktikTreeNode {
    Token* token;
    struct SyntaktikTreeNode** children;
    size_t childrenCount;
    int nodeType; // Stores the semantic type of the node (VarType)
} SyntaktikTreeNode;

#define MAX_CHILDREN 100

/**
 * Creates a new syntaktik tree node with the given token.
 * @param token The token to be stored in the node.
 * @return A pointer to the newly created syntaktik tree node.
 */
SyntaktikTreeNode* createSyntaktikTreeNode(Token* token);

/**
 * Adds a child node to the given parent node.
 * @param parent The parent node to which the child will be added.
 * @param child The child node to be added.
 * @return The index of the added child node, or -1 if the addition failed.
 */
size_t addChildNode(SyntaktikTreeNode* parent, SyntaktikTreeNode* child);

/**
 * Retrieves the child node at the specified index from the given parent node.
 * @param parent The parent node from which the child will be retrieved.
 * @param childIndex The index of the child node to be retrieved.
 * @return A pointer to the child node at the specified index, or NULL if the index is out of bounds.
 */
SyntaktikTreeNode* getChildNode(SyntaktikTreeNode* parent, size_t childIndex);

/**
 * Removes a child node at the specified index from the given parent node.
 * @param parent The parent node from which the child will be removed.
 * @param childIndex The index of the child node to be removed.
 * @return true if the child was successfully removed, false otherwise.
 */
bool removeChildNode(SyntaktikTreeNode* parent, size_t childIndex);

/**
 * Frees the memory allocated for the syntaktik tree starting from the given root node.
 * @param root The root node of the syntaktik tree to be freed.
 */
void freeSyntaktikTree(SyntaktikTreeNode* root);

#endif // SYNTAKTIK_TREE_H