/* 
* symtable.h
* ============================================================
 * Symbol table header for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Vojtěch Kadlec xkadlev00 FIT VUT v Brně
 * Aleš Obr xobral00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */
#ifndef SYMTABLE_H
#define SYMTABLE_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include "token.h"

#define MAX_LENGTH_OF_STRING 20

/**
 * @brief Enumeration for different data types stored in the symbol table.
 */
typedef enum DataType {
    TYPE_FUNCTION,
    TYPE_VARIABLE,
    TYPE_GETTER,
    TYPE_SETTER,
    TYPE_KEYWORD
} DataType;

/**
 * @brief Data structure to hold information about symbols.
 */
typedef struct Data
{
    DataType type;
    Token *token;
    int varType; /*
        For variables: 1 = number, 2 = string, 3 = bool, 4 = null, -1 = undefined
        For functions: number of parameters
        0 if not applicable or for getters/setters
    */
} Data;

/**
 * @brief Node structure for the binary search tree.
 */
typedef struct Node {
    char *key;
    Data *data;
    struct Node *left;
    struct Node *right;
} Node;





/**
 * @brief Initialize a new node with given data.
 *
 * @param data NUL-terminated string to store (caller-owned). A copy or ownership
 * behavior depends on implementation in source file.
 * @return Pointer to newly allocated Node or NULL on allocation failure.
 */
Node *init(char *data);

/**
 * @brief Build a binary tree from a NULL-terminated array of strings.
 *
 * Each string should not exceed MAX_LENGTH_OF_STRING.
 *
 * @param node Pointer to Node* that will hold the root of the created tree.
 * @param leftIndex Left index in the array (usually 0)
 * @param rightIndex Right index in the array (usually length-1)
 * @param array Array of strings
 */
void binTreeFromArray(Node **node, int leftIndex, int rightIndex, char **array);

/**
 * @brief Insert a string into the binary tree rooted at root.
 *
 * @param data String to insert
 * @param root Root of the tree where insertion will occur
 * @return true on success, false on failure
 */
bool insert(char *data, Node **root);

/**
 * @brief Search for a key within the binary tree.
 *
 * @param root Root of the tree
 * @param k Key to search for
 * @return pointer on node with key if is present, NULL otherwise
 */
Node* search(Node *root, char* k);

/**
 * @brief Search for a node by its stored string data.
 *
 * @param root Root of the tree
 * @param data NUL-terminated string to look for
 * @return true if a node with matching data exists, false otherwise
 */
bool searchData(Node *root, const char *data);

/**
 * @brief Delete a node containing data from the tree.
 *
 * @param data String identifying node to delete
 * @param root Root of the tree
 * @return true on successful deletion, false otherwise
 */
bool delete(char *data, Node *root);

/**
 * @brief Dispose (free) the entire binary tree.
 *
 * @param root Root of the tree to be freed
 */
void dispose(Node *root);

#endif // SYMTABLE_H