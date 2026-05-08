/* 
* syntaktik_tree.c
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

#include "syntaktik_tree.h"

SyntaktikTreeNode* createSyntaktikTreeNode(Token* token){
    SyntaktikTreeNode* newNode = (SyntaktikTreeNode*)malloc(sizeof(SyntaktikTreeNode));
    if (!newNode) {
        return NULL;
    }
    newNode->token = token;
    newNode->children = NULL;
    newNode->childrenCount = 0;
    newNode->nodeType = -1; // Initialize as undefined/not analyzed
    return newNode;
}

size_t addChildNode(SyntaktikTreeNode* parent, SyntaktikTreeNode* child){
    if (!parent || !child || parent->childrenCount >= MAX_CHILDREN || parent == child) {
        return (size_t)-1;
    }

    if(parent->children == NULL) {
        parent->children = (SyntaktikTreeNode**)malloc(1 * sizeof(SyntaktikTreeNode*));
        if (!parent->children) {
            return (size_t)-1;
        }
    } else {
        SyntaktikTreeNode** resizedChildren = (SyntaktikTreeNode**)realloc(parent->children, (parent->childrenCount + 1) * sizeof(SyntaktikTreeNode*));
        if (!resizedChildren) {
            return (size_t)-1;
        }
        parent->children = resizedChildren;
    }

    parent->children[parent->childrenCount] = child;
    parent->childrenCount++;

    return parent->childrenCount - 1;
}

SyntaktikTreeNode* getChildNode(SyntaktikTreeNode* parent, size_t childIndex){
    if (!parent || childIndex >= parent->childrenCount) {
        return NULL;
    }
    return parent->children[childIndex];
}

bool removeChildNode(SyntaktikTreeNode* parent, size_t childIndex){
    if (!parent || childIndex >= parent->childrenCount) {
        return false;
    }

    // Free the child node
    freeSyntaktikTree(parent->children[childIndex]);

    // Shift remaining children down
    for (size_t i = childIndex; i < parent->childrenCount - 1; i++) {
        parent->children[i] = parent->children[i + 1];
    }

    // Shrink the children array to free unused memory
    if (parent->childrenCount == 0) {
        free(parent->children);
        parent->children = NULL;
    } else {
        SyntaktikTreeNode** resizedChildren = (SyntaktikTreeNode**)realloc(parent->children, parent->childrenCount * sizeof(SyntaktikTreeNode*));
        if (resizedChildren == NULL) {
            return false;
        }
    }
    parent->childrenCount--;

    return true;
}

void freeSyntaktikTree(SyntaktikTreeNode* root){
    if (root) {
        for (size_t i = 0; i < root->childrenCount; i++) {
            // Recursively free each child
            freeSyntaktikTree(root->children[i]);
        }
        freeToken(root->token);
        free(root->children);
        free(root);
    }
}