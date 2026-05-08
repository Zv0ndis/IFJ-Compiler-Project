/* 
* semantic_functions.c
* ============================================================
 * Semantic analyzer implementation for the IFJ25 project.
 * ============================================================
 * Authors: Jakub Kosinka xkosinj00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#include "semantic_functions.h"
#include "error_codes.h"
#include "syntaktic_analyzator.h"
#include <string.h>

int addGetterFunction(Node **symptable, Token *identifierToken) {
    if(symptable == NULL || identifierToken == NULL) {
        return -2;
    }

    char *key = malloc(strlen(identifierToken->value) + 6);
    strcpy(key, identifierToken->value);
    strcat(key, "{get}");

    if (isGetterFunctionDefined(*symptable, identifierToken)) {
        free(key);
        return 1; // Redefinition error
    }

    if(insert(key, symptable) == false) {
        free(key);
        return -1; // Memory allocation failure
    }
    
    Node *newNode = search(*symptable, key);
    if (newNode == NULL) {
        free(key);
        return -1; // Memory allocation failure
    }

    newNode->data = malloc(sizeof(Data));
    if (newNode->data == NULL) {
        free(key);
        return -1; // Memory allocation failure
    }

    newNode->data->type = TYPE_GETTER;
    newNode->data->token = identifierToken;
    newNode->data->varType = 0;

    free(key);
    return 0;
}

int addSetterFunction(Node **symptable, Token *identifierToken) {
    if(symptable == NULL || identifierToken == NULL) {
        return -2;
    }

    char *key = malloc(strlen(identifierToken->value) + 6);
    strcpy(key, identifierToken->value);
    strcat(key, "{set}");

    if (isSetterFunctionDefined(*symptable, identifierToken)) {
        free(key);
        return 1; // Redefinition error
    }

    if(insert(key, symptable) == false) {
        free(key);
        return -1; // Memory allocation failure
    }
    
    Node *newNode = search(*symptable, key);
    if (newNode == NULL) {
        free(key);
        return -1; // Memory allocation failure
    }

    newNode->data = malloc(sizeof(Data));
    if (newNode->data == NULL) {
        free(key);
        return -1; // Memory allocation failure
    }

    newNode->data->type = TYPE_SETTER;
    newNode->data->token = identifierToken;
    newNode->data->varType = 0;

    free(key);
    return 0;
}

int addFunction(Node **symptable, Token *identifierToken, int numberOfParams) {
    if(symptable == NULL || identifierToken == NULL) {
        return -2;
    }

    char numberOfParamsStr[12];
    sprintf(numberOfParamsStr, "%d", numberOfParams);
    

    char *key = malloc(strlen(identifierToken->value) + 20);
    strcpy(key, identifierToken->value);
    strcat(key, "{");
    strcat(key, numberOfParamsStr);
    strcat(key, "}");

    if (isFunctionDefined(*symptable, identifierToken, numberOfParams)) {
        free(key);
        return 1; // Redefinition error
    }

    if(insert(key, symptable) == false) {
        free(key);
        return -1; // Memory allocation failure
    }
    
    Node *newNode = search(*symptable, key);
    if (newNode == NULL) {
        free(key);
        return -1; // Memory allocation failure
    }

    newNode->data = malloc(sizeof(Data));
    if (newNode->data == NULL) {
        free(key);
        return -1; // Memory allocation failure
    }

    newNode->data->type = TYPE_FUNCTION;
    newNode->data->token = identifierToken;
    newNode->data->varType = numberOfParams;

    free(key);
    return 0;
}

int addVariable(Node **symptable, Token *identifierToken, VarType variableType) {
    if(symptable == NULL || identifierToken == NULL) {
        return -2;
    }

    char *key = malloc(strlen(identifierToken->value) + 1);
    strcpy(key, identifierToken->value);

    if (isVariableDefined(*symptable, identifierToken)) {
        free(key);
        return 1; // Redefinition error
    }

    if(insert(key, symptable) == false) {
        free(key);
        return -1; // Memory allocation failure
    }
    
    Node *newNode = search(*symptable, key);
    if (newNode == NULL) {
        free(key);
        return -1; // Memory allocation failure
    }

    newNode->data = malloc(sizeof(Data));
    if (newNode->data == NULL) {
        free(key);
        return -1; // Memory allocation failure
    }

    newNode->data->type = TYPE_VARIABLE;
    newNode->data->token = identifierToken;
    if(variableType == VAR_TYPE_NUM) {
        newNode->data->varType = 1;
    } else if(variableType == VAR_TYPE_STRING) {
        newNode->data->varType = 2;
    } else if(variableType == VAR_TYPE_BOOL) {
        newNode->data->varType = 3;
    } else if(variableType == VAR_TYPE_NULL) {
        newNode->data->varType = 4;
    } else {
        newNode->data->varType = -1; // Undefined
    }

    free(key);
    return 0;
}

int isGetterFunctionDefined(Node *symptable, Token *identifierToken) {
    if(symptable == NULL || identifierToken == NULL) {
        return 0;
    }

    char *key = malloc(strlen(identifierToken->value) + 6);
    strcpy(key, identifierToken->value);
    strcat(key, "{get}");

    Node *node = search(symptable, key);
    free(key);

    if (node != NULL && node->data != NULL && node->data->type == TYPE_GETTER) {
        return 1; // Defined
    }
    return 0; // Not defined
}

int isSetterFunctionDefined(Node *symptable, Token *identifierToken) {
    if(symptable == NULL || identifierToken == NULL) {
        return 0;
    }

    char *key = malloc(strlen(identifierToken->value) + 6);
    strcpy(key, identifierToken->value);
    strcat(key, "{set}");

    Node *node = search(symptable, key);
    free(key);

    if (node != NULL && node->data != NULL && node->data->type == TYPE_SETTER) {
        return 1; // Defined
    }
    return 0; // Not defined
}

int isFunctionDefined(Node *symptable, Token *identifierToken, int numberOfParams) {
    if(symptable == NULL || identifierToken == NULL) {
        return 0;
    }

    char numberOfParamsStr[12];
    sprintf(numberOfParamsStr, "%d", numberOfParams);

    char *key = malloc(strlen(identifierToken->value) + 20);
    strcpy(key, identifierToken->value);
    strcat(key, "{");
    strcat(key, numberOfParamsStr);
    strcat(key, "}");

    Node *node = search(symptable, key);
    free(key);

    if (node != NULL && node->data != NULL && node->data->type == TYPE_FUNCTION) {
        return 1; // Defined
    }
    return 0; // Not defined
}

/* Helper: check if any function with given name exists (any arity) */
int isFunctionNameDefined(Node *symptable, Token *identifierToken) {
    if (symptable == NULL || identifierToken == NULL) return 0;

    // Traverse tree to find any key that starts with identifierToken->value + "{"
    size_t name_len = strlen(identifierToken->value);
    Node *stack[256];
    int sp = 0;
    Node *cur = symptable;
    while (cur || sp > 0) {
        while (cur) {
            stack[sp++] = cur;
            cur = cur->left;
        }
        cur = stack[--sp];
        if (cur->key) {
            // Check if key starts with function_name + "{"
            if (strncmp(cur->key, identifierToken->value, name_len) == 0 &&
                cur->key[name_len] == '{' &&
                cur->data && cur->data->type == TYPE_FUNCTION) {
                return 1; // Found function with this name
            }
        }
        cur = cur->right;
    }
    return 0; // No function with this name
}

int isVariableDefined(Node *symptable, Token *identifierToken) {
    if(symptable == NULL || identifierToken == NULL) {
        return 0;
    }

    Node *node = search(symptable, identifierToken->value);

    if (node != NULL && node->data != NULL && node->data->type == TYPE_VARIABLE) {
        return 1; // Defined
    }
    return 0; // Not defined
}

int isVariableUsage(struct SyntaktikTreeNode *node, struct SyntaktikTreeNode *parent_node) {
    if (node == NULL || node->token == NULL || node->token->type != TOKEN_IDENTIFIER) {
        return 0;
    }
    
    // If the identifier has children, it's likely a function call or complex expression
    if (node->childrenCount > 0) {
        return 0;
    }
    
    // Check parent context to determine if this is a variable usage
    if (parent_node == NULL) {
        return 1; // Standalone identifier likely a variable
    }
    
    // If parent is an assignment and this is the right side, it's likely a variable usage
    if (parent_node->token && parent_node->token->type == TOKEN_ASSIGN) {
        // Check if this node is the right child (value being assigned)
        if (parent_node->childrenCount == 2 && parent_node->children[1] == node) {
            return 1;
        }
    }
    
    // If parent is an expression operator, this is likely a variable usage
    if (parent_node->token) {
        TokenType parent_type = parent_node->token->type;
        if (parent_type == TOKEN_PLUS || parent_type == TOKEN_MINUS || 
            parent_type == TOKEN_MUL || parent_type == TOKEN_DIV ||
            parent_type == TOKEN_EQ || parent_type == TOKEN_NEQ ||
            parent_type == TOKEN_LT || parent_type == TOKEN_GT ||
            parent_type == TOKEN_LTE || parent_type == TOKEN_GTE) {
            return 1;
        }
    }
    
    return 1; // Default to variable usage for safety
}



VarType getVariableType(Node *local_symtable, Node *global_symtable, Token *identifierToken) {
    if(identifierToken == NULL || identifierToken->value == NULL) {
        return VAR_TYPE_UNDEFINED;
    }

    // Check local symbol table first
    if (local_symtable) {
        Node *node = search(local_symtable, identifierToken->value);
        if (node != NULL && node->data != NULL && node->data->type == TYPE_VARIABLE) {
            // If varType is negative or out of valid range, treat as undefined
            if (node->data->varType < 0 || node->data->varType > VAR_TYPE_UNDEFINED) {
                return VAR_TYPE_UNDEFINED;
            }
            return (VarType)(node->data->varType);
        }
    }
    
    // Check global symbol table
    if (global_symtable) {
        Node *node = search(global_symtable, identifierToken->value);
        if (node != NULL && node->data != NULL && node->data->type == TYPE_VARIABLE) {
            // If varType is negative or out of valid range, treat as undefined
            if (node->data->varType < 0 || node->data->varType > VAR_TYPE_UNDEFINED) {
                return VAR_TYPE_UNDEFINED;
            }
            return (VarType)(node->data->varType);
        }
    }
    
    return VAR_TYPE_UNDEFINED;
}

int checkBuiltInFunctionCallParams(Node* local_symtable, Node* global_symtable, Token *functionToken, Token **argumentTokens, int numberOfArguments, VarType *returnType){
    if(functionToken == NULL || returnType == NULL) {
        return -2;
    }

    VarType *argType = NULL;
    if(numberOfArguments > 0 && argumentTokens != NULL) {
        argType = malloc(sizeof(VarType) * numberOfArguments);
        if(argType == NULL) {
            return -2; // Memory allocation failure
        }
        for (int i = 0; i < numberOfArguments; i++) {
            Token *a = argumentTokens ? argumentTokens[i] : NULL;
            if (a == NULL) {
                argType[i] = VAR_TYPE_UNDEFINED;
                continue;
            }
            if(a->type == TOKEN_IDENTIFIER) {
                argType[i] = getVariableType(local_symtable, global_symtable, a);
            } else if(a->type == TOKEN_STRING) {
                argType[i] = VAR_TYPE_STRING;
            } else if(argumentTokens[i]->type == TOKEN_NUMBER || argumentTokens[i]->type == TOKEN_FLOAT || argumentTokens[i]->type == TOKEN_INT) {
                argType[i] = VAR_TYPE_NUM;
            } else {
                argType[i] = VAR_TYPE_UNDEFINED;
            }
        }
        if(argType == NULL) {
            return -2; // Memory allocation failure
        }
    }
    
    

    int result = 0;
    
    if(!strcmp(functionToken->value, "Ifj.read_str")){
        if(numberOfArguments != 0){
            result = 1; // Type mismatch
        } else {
            *returnType = VAR_TYPE_STRING;
            result = 0; // Parameters are correct
        }
    }else if(!strcmp(functionToken->value, "Ifj.read_num")){
        if(numberOfArguments != 0){
            result = 1; // Type mismatch
        } else {
            *returnType = VAR_TYPE_NUM;
            result = 0; // Parameters are correct
        }
    }else if(!strcmp(functionToken->value, "Ifj.write")){
        if(numberOfArguments != 1){
            result = 1; // Type mismatch
        } else {
            *returnType = VAR_TYPE_NULL;
            result = 0; // Parameters are correct
        }
    }else if(!strcmp(functionToken->value, "Ifj.floor")){
        if(numberOfArguments != 1){
            result = 1; // Type mismatch
        } else if(argType) {
            if (argType[0] >= 0 && argType[0] != VAR_TYPE_NUM && argType[0] != VAR_TYPE_UNDEFINED) {
                result = 1; // Type mismatch - only fail if type is known and wrong
            } else {
                *returnType = VAR_TYPE_NUM;
                result = 0; // Parameters are correct
            }
        } else {
            *returnType = VAR_TYPE_NUM;
            result = 0; // Parameters are correct
        }
    }else if(!strcmp(functionToken->value, "Ifj.str")){
        if(numberOfArguments != 1){
            result = 1; // Type mismatch
        } else {
            *returnType = VAR_TYPE_STRING;
            result = 0; // Parameters are correct
        }
    }else if(!strcmp(functionToken->value, "Ifj.length")){
        if(numberOfArguments != 1){
            result = 1; // Type mismatch
        } else if(argType && argType[0] >= 0 && argType[0] != VAR_TYPE_STRING && argType[0] != VAR_TYPE_UNDEFINED){
            result = 1; // Type mismatch - only fail if type is known (>= 0) and wrong
        } else {
            *returnType = VAR_TYPE_NUM;
            result = 0; // Parameters are correct
        }
    }else if(!strcmp(functionToken->value, "Ifj.substring")){
        if(numberOfArguments != 3){
            result = 1; // Type mismatch
        } else if(argType && ((argType[0] >= 0 && argType[0] != VAR_TYPE_STRING && argType[0] != VAR_TYPE_UNDEFINED) ||
           (argType[1] >= 0 && argType[1] != VAR_TYPE_NUM && argType[1] != VAR_TYPE_UNDEFINED) ||
           (argType[2] >= 0 && argType[2] != VAR_TYPE_NUM && argType[2] != VAR_TYPE_UNDEFINED))){
            result = 1; // Type mismatch - only fail if type is known (>= 0) and wrong
        } else {
            *returnType = VAR_TYPE_STRING;
            result = 0; // Parameters are correct
        }
    }else if(!strcmp(functionToken->value, "Ifj.strcmp")){
        if(numberOfArguments != 2){
            result = 1; // Type mismatch
        } else if(argType && ((argType[0] >= 0 && argType[0] != VAR_TYPE_STRING && argType[0] != VAR_TYPE_UNDEFINED) ||
           (argType[1] >= 0 && argType[1] != VAR_TYPE_STRING && argType[1] != VAR_TYPE_UNDEFINED))){
            result = 1; // Type mismatch - only fail if type is known (>= 0) and wrong
        } else {
            *returnType = VAR_TYPE_NUM;
            result = 0; // Parameters are correct
        }
    }else if(!strcmp(functionToken->value, "Ifj.ord")){
        if(numberOfArguments != 2){
            result = 1; // Type mismatch
        } else if(argType && ((argType[0] >= 0 && argType[0] != VAR_TYPE_STRING && argType[0] != VAR_TYPE_UNDEFINED) ||
           (argType[1] >= 0 && argType[1] != VAR_TYPE_NUM && argType[1] != VAR_TYPE_UNDEFINED))){
            result = 1; // Type mismatch - only fail if type is known (>= 0) and wrong
        } else {
            *returnType = VAR_TYPE_NUM;
            result = 0; // Parameters are correct
        }
    }else if(!strcmp(functionToken->value, "Ifj.chr")){
        if(numberOfArguments != 1){
            result = 1; // Type mismatch
        } else if(argType && argType[0] >= 0 && argType[0] != VAR_TYPE_NUM && argType[0] != VAR_TYPE_UNDEFINED){
            result = 1; // Type mismatch - only fail if type is known (>= 0) and wrong
        } else {
            *returnType = VAR_TYPE_STRING;
            result = 0; // Parameters are correct
        }
    }else{
        result = -1; // Unknown built-in function
    }
    
    // Clean up allocated memory
    if(argType) {
        free(argType);
    }
    
    return result;
}

VarType checkArithmeticAndStringOperationTypes(VarType leftType, VarType rightType, Token *operatorToken){
    if(operatorToken == NULL) {
        return VAR_TYPE_UNDEFINED;
    }

    if(operatorToken->type == TOKEN_PLUS){
        if(leftType == VAR_TYPE_STRING && rightType == VAR_TYPE_STRING){
            return VAR_TYPE_STRING;
        }else if(leftType == VAR_TYPE_NUM && rightType == VAR_TYPE_NUM){
            return VAR_TYPE_NUM;
        } else if((leftType == VAR_TYPE_STRING && rightType == VAR_TYPE_NULL) || 
                  (leftType == VAR_TYPE_NULL && rightType == VAR_TYPE_STRING)) {
            return VAR_TYPE_STRING; // Allow String + Null (concatenation)
        }else if(leftType == VAR_TYPE_UNDEFINED || rightType == VAR_TYPE_UNDEFINED){
            if (leftType == VAR_TYPE_STRING || rightType == VAR_TYPE_STRING) {
                return VAR_TYPE_STRING;
            }
            return VAR_TYPE_NUM; // Assume numeric for undefined types in arithmetic
        }else{
            return VAR_TYPE_UNDEFINED; // Mismatch
        }
    }else if(operatorToken->type == TOKEN_MINUS ||
            operatorToken->type == TOKEN_DIV){
        if(leftType == VAR_TYPE_NUM && rightType == VAR_TYPE_NUM){
            return VAR_TYPE_NUM;
        }else if(leftType == VAR_TYPE_UNDEFINED || rightType == VAR_TYPE_UNDEFINED){
            // Check if the KNOWN type is incompatible
            if (leftType != VAR_TYPE_UNDEFINED && leftType != VAR_TYPE_NUM) return VAR_TYPE_UNDEFINED;
            if (rightType != VAR_TYPE_UNDEFINED && rightType != VAR_TYPE_NUM) return VAR_TYPE_UNDEFINED;
            return VAR_TYPE_NUM; // Assume numeric for undefined types
        }else{
            return VAR_TYPE_UNDEFINED; // Mismatch
        }
    }else if(operatorToken->type == TOKEN_MUL){
        if(leftType == VAR_TYPE_NUM && rightType == VAR_TYPE_NUM){
            return VAR_TYPE_NUM;
        }else if(leftType == VAR_TYPE_STRING && rightType == VAR_TYPE_NUM){
            return VAR_TYPE_STRING;
        } else if(leftType == VAR_TYPE_STRING && rightType == VAR_TYPE_NULL){
             return VAR_TYPE_STRING; // Allow String * Null (treat Null as 0 -> empty string?)
        }else if(leftType == VAR_TYPE_UNDEFINED || rightType == VAR_TYPE_UNDEFINED){
            if (leftType == VAR_TYPE_STRING) return VAR_TYPE_STRING;
            // If left is undefined, we don't know if it's string or num.
            // If right is string, it's invalid (only string * num is valid, not num * string or string * string)
            if (rightType == VAR_TYPE_STRING) return VAR_TYPE_UNDEFINED;
            
            return VAR_TYPE_NUM;
        }else{
            return VAR_TYPE_UNDEFINED; // Mismatch
        }
    }

    return VAR_TYPE_UNDEFINED;
}

VarType checkRelationalOperationTypes(VarType leftType, VarType rightType, Token *operatorToken){
    if(operatorToken == NULL) {
        return VAR_TYPE_UNDEFINED;
    }

    if(operatorToken->type == TOKEN_EQ || operatorToken->type == TOKEN_NEQ){
        return VAR_TYPE_BOOL; // Equality operators support all types
    }else if(operatorToken->type == TOKEN_LT ||
            operatorToken->type == TOKEN_LTE ||
            operatorToken->type == TOKEN_GT ||
            operatorToken->type == TOKEN_GTE){
        
        if (leftType == VAR_TYPE_NUM && rightType == VAR_TYPE_NUM) return VAR_TYPE_BOOL;
        if (leftType == VAR_TYPE_STRING && rightType == VAR_TYPE_STRING) return VAR_TYPE_BOOL;
        
        if (leftType == VAR_TYPE_UNDEFINED || rightType == VAR_TYPE_UNDEFINED) return VAR_TYPE_BOOL;
        
        return VAR_TYPE_UNDEFINED; // Mismatch
    }

    return VAR_TYPE_UNDEFINED; // Unsupported types for relational operations
}

int performSemanticAnalysis(struct SyntaktikTreeNode *ast_root, Node *global_symtable) {
    if (ast_root == NULL || global_symtable == NULL) {
        return 0; // Nothing to analyze
    }
    
    // Start semantic analysis from the root
    return analyzeASTNode(ast_root, global_symtable, NULL, NULL);
}

int analyzeASTNode(struct SyntaktikTreeNode *node, Node *global_symtable, Node *local_symtable, char *current_class_name) {
    if (node == NULL) {
        return 0;
    }
    
    char *next_class_name = current_class_name;
    Node *next_local_symtable = local_symtable;
    
    // 1. Detect Class Definition
    if (node->token && node->token->type == TOKEN_IDENTIFIER) {
        if (current_class_name == NULL) {
             // This is likely a class definition
             Node *classTable = getClassTable(node->token->value);
             if (classTable) {
                 next_class_name = node->token->value;
             }
        } else {
            // We are inside a class. This identifier could be a function definition.
            if (local_symtable == NULL) {
                // Try to find a local table for this function
                int param_count = 0;
                bool has_block = false;
                for (size_t i = 0; i < node->childrenCount; i++) {
                    if (node->children[i]->token && node->children[i]->token->type == TOKEN_CURLY_BRACKET_START) {
                        has_block = true;
                    } else if (node->children[i]->token && node->children[i]->token->type == TOKEN_IDENTIFIER) {
                        param_count++;
                    }
                }
                
                if (has_block) {
                    char key[256];
                    snprintf(key, sizeof(key), "%s{%d}", node->token->value, param_count);
                    Node *lt = getFunctionLocalTable(key);
                    
                    if (!lt) {
                        snprintf(key, sizeof(key), "%s{get}", node->token->value);
                        lt = getFunctionLocalTable(key);
                    }
                    
                    if (!lt) {
                        snprintf(key, sizeof(key), "%s{set}", node->token->value);
                        lt = getFunctionLocalTable(key);
                    }
                    
                    if (lt) {
                        next_local_symtable = lt;
                    }
                }
            }
        }
    }

    // Recursively analyze all children FIRST
    for (size_t i = 0; i < node->childrenCount; i++) {
        int result = analyzeASTNode(node->children[i], global_symtable, next_local_symtable, next_class_name);
        if (result != 0) {
            return result;
        }
    }
    
    // Update nodeType based on token (for leaf nodes or re-evaluation)
    if (node->token) {
        if (node->token->type == TOKEN_NUMBER || node->token->type == TOKEN_FLOAT || node->token->type == TOKEN_INT) {
            node->nodeType = VAR_TYPE_NUM;
        } else if (node->token->type == TOKEN_STRING) {
            node->nodeType = VAR_TYPE_STRING;
        } else if (node->token->type == TOKEN_KEYWORD) {
            if (strcmp(node->token->value, "null") == 0) node->nodeType = VAR_TYPE_NULL;
            else if (strcmp(node->token->value, "true") == 0 || strcmp(node->token->value, "false") == 0) node->nodeType = VAR_TYPE_BOOL;
        } else if (node->token->type == TOKEN_IDENTIFIER || node->token->type == TOKEN_GLOBAL_IDENTIFIER) {
            // Only update if not already set (e.g. by prec_anal), or force update to be sure?
            // Force update is safer as symbol table might be more complete now.
            node->nodeType = getVariableType(next_local_symtable, global_symtable, node->token);
        }
    }
    
    // Check for assignment to update variable type in symbol table (simple type propagation)
    if (node->token && node->token->type == TOKEN_ASSIGN && node->childrenCount == 2) {
        struct SyntaktikTreeNode *left = node->children[0];
        struct SyntaktikTreeNode *right = node->children[1];
        
        if (left && left->token && left->token->type == TOKEN_IDENTIFIER) {
            // Resolve right side type
            VarType rightType = (VarType)right->nodeType;
            
            if (rightType != VAR_TYPE_UNDEFINED) {
                // Update variable type in symbol table
                Node *symNode = NULL;
                if (next_local_symtable) symNode = search(next_local_symtable, left->token->value);
                if (!symNode && global_symtable) symNode = search(global_symtable, left->token->value);
                
                if (symNode && symNode->data) {
                    symNode->data->varType = rightType;
                    // Also update the left node's type in the AST
                    left->nodeType = rightType;
                }
            }
        }
    }

    // Check for function calls
    if (node->token && (node->token->type == TOKEN_IDENTIFIER || node->token->type == TOKEN_KEYWORD) && node->childrenCount > 0) {
        // Handle built-in functions
        if (node->token->value && strncmp(node->token->value, "Ifj.", 4) == 0) {
             if (strcmp(node->token->value, "Ifj.read_str") == 0) {
                 node->nodeType = VAR_TYPE_STRING;
             } else if (strcmp(node->token->value, "Ifj.read_num") == 0) {
                 node->nodeType = VAR_TYPE_NUM;
             } else if (strcmp(node->token->value, "Ifj.write") == 0) {
                 node->nodeType = VAR_TYPE_NULL;
             } else if (strcmp(node->token->value, "Ifj.floor") == 0) {
                 node->nodeType = VAR_TYPE_NUM;
             } else if (strcmp(node->token->value, "Ifj.str") == 0) {
                 node->nodeType = VAR_TYPE_STRING;
             } else if (strcmp(node->token->value, "Ifj.length") == 0) {
                 node->nodeType = VAR_TYPE_NUM;
             } else if (strcmp(node->token->value, "Ifj.substring") == 0) {
                 node->nodeType = VAR_TYPE_STRING;
             } else if (strcmp(node->token->value, "Ifj.strcmp") == 0) {
                 node->nodeType = VAR_TYPE_NUM;
             } else if (strcmp(node->token->value, "Ifj.ord") == 0) {
                 node->nodeType = VAR_TYPE_NUM;
             } else if (strcmp(node->token->value, "Ifj.chr") == 0) {
                 node->nodeType = VAR_TYPE_STRING;
             }
        }
        // Only check if this looks like a standalone function call (not built-in)
        else {
            
            bool is_function_definition = false;
            for (size_t i = 0; i < node->childrenCount; i++) {
                if (node->children[i] && node->children[i]->token && 
                    node->children[i]->token->type == TOKEN_CURLY_BRACKET_START) {
                    is_function_definition = true;
                    break;
                }
            }
            
            if (!is_function_definition) {
                int result = checkFunctionCall(node, global_symtable, next_local_symtable, next_class_name);
                if (result != 0) {
                    return result;
                }
            }
        }
    }
    
    // Check type compatibility for arithmetic and string operations
    if (node->token && node->childrenCount == 2) {
        TokenType op_type = node->token->type;
        if (op_type == TOKEN_PLUS || op_type == TOKEN_MINUS || 
            op_type == TOKEN_MUL || op_type == TOKEN_DIV) {
            
            VarType leftType = node->children[0] ? (VarType)node->children[0]->nodeType : VAR_TYPE_UNDEFINED;
            VarType rightType = node->children[1] ? (VarType)node->children[1]->nodeType : VAR_TYPE_UNDEFINED;
            
            node->nodeType = checkArithmeticAndStringOperationTypes(leftType, rightType, node->token);
            
            if (node->nodeType == VAR_TYPE_UNDEFINED) {
                fprintf(stderr, "DEBUG: Type mismatch in arithmetic op %d. Left: %d, Right: %d\n", op_type, leftType, rightType);
                return ERROR_SEMANTIC_TYPE_MISMATCH;
            }
        }
        
        // Check relational operators
        if (op_type == TOKEN_LT || op_type == TOKEN_LTE ||
            op_type == TOKEN_GT || op_type == TOKEN_GTE ||
            op_type == TOKEN_EQ || op_type == TOKEN_NEQ) {
            
            VarType leftType = node->children[0] ? (VarType)node->children[0]->nodeType : VAR_TYPE_UNDEFINED;
            VarType rightType = node->children[1] ? (VarType)node->children[1]->nodeType : VAR_TYPE_UNDEFINED;
            
            node->nodeType = checkRelationalOperationTypes(leftType, rightType, node->token);
            
            if (node->nodeType == VAR_TYPE_UNDEFINED) {
                fprintf(stderr, "DEBUG: Type mismatch in relational op %d. Left: %d, Right: %d\n", op_type, leftType, rightType);
                return ERROR_SEMANTIC_TYPE_MISMATCH;
            }
        }

        // Check 'is' operator
        if (node->token->type == TOKEN_KEYWORD && strcmp(node->token->value, "is") == 0) {
             node->nodeType = VAR_TYPE_BOOL;
        }
    }
    
    return 0;
}

int checkFunctionCall(struct SyntaktikTreeNode *function_node, Node *global_symtable, Node *local_symtable, char *current_class_name) {
    if (function_node == NULL || function_node->token == NULL) {
        return 0;
    }
    
    char *function_name = function_node->token->value;
    if (function_name == NULL) {
        return 0;
    }
    
    // Skip built-in functions and common keywords/constructs
    if (strncmp(function_name, "Ifj.", 4) == 0) {
        return 0;
    }
    
    // Skip common non-function identifiers
    if (strcmp(function_name, "Program") == 0 || strcmp(function_name, "main") == 0 ||
        strcmp(function_name, "var") == 0 || strcmp(function_name, "if") == 0 ||
        strcmp(function_name, "while") == 0 || strcmp(function_name, "return") == 0 ||
        strcmp(function_name, "import") == 0 || strcmp(function_name, "is") == 0) {
        return 0;
    }
    
    // Count arguments (children of the function node)
    int arg_count = (int)function_node->childrenCount;
    
    // Create a token for the function name to use with existing functions
    Token function_token;
    function_token.value = function_name;
    function_token.type = TOKEN_IDENTIFIER;
    
    // Check in the Program class symbol table
    Node *program_node = getClassTable("Program");
    
    if (program_node) {
        // Check if function is defined with the given parameter count in Program class
        if (isFunctionDefined(program_node, &function_token, arg_count)) {
            return 0; // Function found in Program class with correct param count
        }
        
        // Check if function exists with different parameter count
        if (isFunctionNameDefined(program_node, &function_token)) {
            return ERROR_SEMANTIC_ARGUMENTS; // Function exists but wrong param count
        }
        
        // Also check for getter/setter functions in Program class
        if (arg_count == 0 && isGetterFunctionDefined(program_node, &function_token)) {
            return 0; // Getter found in Program class
        }
        if (arg_count == 1 && isSetterFunctionDefined(program_node, &function_token)) {
            return 0; // Setter found in Program class
        }
    }
    
    // Check in global symbol table for global functions
    if (isFunctionDefined(global_symtable, &function_token, arg_count)) {
        return 0; // Function found in global scope with correct param count
    }
    
    // Check if function exists in global with different parameter count
    if (isFunctionNameDefined(global_symtable, &function_token)) {
        return ERROR_SEMANTIC_ARGUMENTS; // Function exists but wrong param count
    }
    
    // If we reach here, the function was not found anywhere
    // fprintf(stderr, "DEBUG: Function '%s' with %d args not found. Program table: %p, Global table: %p\n", function_name, arg_count, (void*)program_node, (void*)global_symtable);
    fprintf(stderr, "DEBUG: Function '%s' with %d args not found.\n", function_name, arg_count);
    return ERROR_SEMANTIC_UNDEFINED;
}