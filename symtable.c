/* 
* symtable.c
* ============================================================
 * Symbol table implementation for the IFJ25 project.
 * Implements a binary search tree for storing strings.
 * The tree is height-balanced using AVL rotations on insertions.
 * ============================================================
 * Authors: 
 * Vojtěch Kadlec xkadlev00 FIT VUT v Brně
 * Aleš Obr xobral00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */
#include "symtable.h"

Node *init(char *data){
    Node *root = malloc(sizeof(Node));  // allocate memory for the root node itself
    if(root == NULL){
        return NULL;
    }
    root->key = malloc(sizeof(char) * MAX_LENGTH_OF_STRING);
    if (root->key == NULL) {
        free(root);
        return NULL;
    }
    /* Copy up to MAX_LENGTH_OF_STRING-1 chars and NUL-terminate */
    if (data) {
        strncpy(root->key, data, MAX_LENGTH_OF_STRING - 1);
        root->key[MAX_LENGTH_OF_STRING - 1] = '\0';
    } else {
        root->key[0] = '\0';
    }
    root->data = NULL; /* Data payload (metadata) not set by init */
    root->left = NULL;
    root->right = NULL;
    return root;
}

void binTreeFromArray(Node **node, int leftIndex, int rightIndex,char **array){
    if(leftIndex <= rightIndex){
        int middle = (leftIndex+rightIndex)/2;
        *node = (Node*)malloc(sizeof(Node));
        if(*node == NULL){
            return;
        }

        (*node)->key = malloc(sizeof(char) * MAX_LENGTH_OF_STRING);
        if((*node)->key == NULL){
            free(*node);
            *node = NULL;
            return;
        }
        int i = 0;
        (*node)->left = NULL;
        (*node)->right = NULL;
        while(array[middle][i] != '\0' && i != MAX_LENGTH_OF_STRING - 1){
            (*node)->key[i] = array[middle][i];
            i++;
        }
        (*node)->key[i] = '\0';
        (*node)->data = NULL;
        
        binTreeFromArray(&(*node)->left,leftIndex,middle-1,array); // recursively build the left subtree
        binTreeFromArray(&(*node)->right,middle+1,rightIndex,array); // recursively build the right subtree
    }
    else{
        (*node) = NULL; // set the node to NULL if the indices are invalid
    }
}

Node* search(Node *root,char* k){
    /*if (root == NULL){
        return NULL;
    }
    /* Interpret stored key as integer if possible and compare to k. This function
       is kept for API compatibility with header; most callers use searchData(). 
    int node_val = 0;
    if (root->key) node_val = atoi(root->key);
    if (node_val == k) return root;
    if (k < node_val) return search(root->left, k);
    return search(root->right, k);*/
    if(root == NULL){
        return NULL;
    }
    int cmp = strcmp(k, root->key);
    if(cmp == 0){
        return root;
    } else if(cmp < 0){
        return search(root->left, k);
    } else {
        return search(root->right, k);
    }
}

bool searchData(Node *root, const char *data){
    if(root == NULL){
        return false;
    }
    int cmp = strcmp(data, root->key);
    if(cmp == 0){
        return true;
    } else if(cmp < 0){
        return searchData(root->left, data);
    } else {
        return searchData(root->right, data);
    }
}

void dispose(Node *root){
    if(root != NULL){
        dispose(root->left);
        dispose(root->right);
        if (root->key) free(root->key);
        if (root->data) {
            /* free Data struct if present */
            if (root->data->token) freeToken(root->data->token);
            free(root->data);
        }
        free(root);
    }
}

// AVL-balanced insert implementation

static int nodeHeight(Node *n){
    if(n == NULL) return 0;
    int lh = nodeHeight(n->left);
    int rh = nodeHeight(n->right);
    return 1 + (lh > rh ? lh : rh);
}

static int getBalance(Node *n){
    if(n == NULL) return 0;
    return nodeHeight(n->left) - nodeHeight(n->right);
}

static Node* rightRotate(Node* y){
    Node* x = y->left;
    Node* T2 = x->right;
    x->right = y;
    y->left = T2;
    return x;
}

static Node* leftRotate(Node* x){
    Node* y = x->right;
    Node* T2 = y->left;
    y->left = x;
    x->right = T2;
    return y;
}

static Node* insertNode(Node *node, const char *data, bool *ok){
    if(node == NULL){
        Node *n = init((char*)data);
        if(n == NULL){
            *ok = false;
            return NULL;
        }
        *ok = true;
        return n;
    }

    int cmp = strcmp(data, node->key);
    if(cmp < 0){
        node->left = insertNode(node->left, data, ok);
    } else if(cmp > 0){
        node->right = insertNode(node->right, data, ok);
    } else {
        // already exists — preserve original behavior: return true (no insertion)
        *ok = true;
        return node;
    }

    int balance = getBalance(node);

    // Left Left
    if(balance > 1 && node->left && strcmp(data, node->left->key) < 0)
        return rightRotate(node);

    // Right Right
    if(balance < -1 && node->right && strcmp(data, node->right->key) > 0)
        return leftRotate(node);

    // Left Right
    if(balance > 1 && node->left && strcmp(data, node->left->key) > 0){
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    // Right Left
    if(balance < -1 && node->right && strcmp(data, node->right->key) < 0){
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

bool insert(char *data, Node **root){
    bool ok = false;
    Node *newRoot = insertNode(*root, data, &ok);
    if(newRoot == NULL && ok == false) return false; // allocation failure
    *root = newRoot;
    return true; // keep original semantics: success even if item existed
}

bool delete(char *data, Node *root){
    bool deleted = false;
    // helper returns new subtree root after deletion
    extern Node* deleteNode(Node *node, const char *data, bool *deleted);
    // call helper (we ignore returned root for the caller since signature does not allow updating caller pointer)
    Node *newRoot = deleteNode(root, data, &deleted);
    (void)newRoot; // silence unused variable warning
    return deleted;
}

// find minimum node in subtree
static Node* findMin(Node* node){
    while(node && node->left) node = node->left;
    return node;
}

// recursive delete helper
Node* deleteNode(Node *node, const char *data, bool *deleted){
    if(node == NULL) return NULL;
    int cmp = strcmp(data, node->key);
    if(cmp < 0){
        node->left = deleteNode(node->left, data, deleted);
    } else if(cmp > 0){
        node->right = deleteNode(node->right, data, deleted);
    } else {
        // found node to delete
        *deleted = true;
        // case: no child
        if(node->left == NULL && node->right == NULL){
            free(node->data);
            free(node);
            return NULL;
        }
        // case: only right child
        else if(node->left == NULL){
            Node *tmp = node->right;
            free(node->data);
            free(node);
            return tmp;
        }
        // case: only left child
        else if(node->right == NULL){
            Node *tmp = node->left;
            free(node->data);
            free(node);
            return tmp;
        }
        // case: two children
        else {
            Node *succ = findMin(node->right);
            // copy successor data into current node's data buffer
            if(succ && succ->key){
                /* copy successor's key into current node's key buffer */
                strncpy(node->key, succ->key, MAX_LENGTH_OF_STRING - 1);
                node->key[MAX_LENGTH_OF_STRING - 1] = '\0';
            }
            /* delete the successor node from right subtree */
            node->right = deleteNode(node->right, succ->key, deleted);
        }
    }
    return node;
}