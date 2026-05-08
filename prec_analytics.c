/* 
* prec_analytics.c
* ============================================================
 * Precedence analyzer implementation for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Tomáš Zovníček xzvonit00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#include "prec_analytics.h"
#include "syntaktic_analyzator.h"
#include "lexikal_analyzator.h"
#include "syntaktik_tree.h"
#include "semantic_functions.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Forward declaration to avoid including syntaktik_tree.h when analyzer reports it unused */
void psa_AttachTokenToCurrentParent(Token *token);

// Static file pointer for token retrieval
static FILE *input_file = NULL;
extern int Error;  // Assuming Error is defined in syntaktic_analyzator.c

const PrecRelation PREC_TABLE[E_COUNT][E_COUNT] = {
/*                +             -            *              /           <           <=          >           >=          ==          !=          is          (           )           id          int         str         nil         $END        */
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*  +   */     { PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*  -   */     { PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*  *   */     { PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*  /   */     { PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*  <   */     { PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*  <=  */     { PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*  >   */     { PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*  >=  */     { PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*  ==  */     { PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*  !=  */     { PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*  is  */     { PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_REDUCE, PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_REDUCE },
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*  (   */     { PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_EQUAL,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_STOP   },
/*  )   */     { PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_STOP,   PREC_REDUCE, PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_REDUCE },
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*  id  */     { PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_STOP,   PREC_REDUCE, PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_REDUCE },
/*  int */     { PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_STOP,   PREC_REDUCE, PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_REDUCE },
/*  str */     { PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_STOP,   PREC_REDUCE, PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_REDUCE },
/*  nil */     { PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_STOP,   PREC_REDUCE, PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_STOP,   PREC_REDUCE },
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
/*  $END*/     { PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_STOP,   PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_SHIFT,  PREC_STOP   },
/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
};

/**
 * @brief Sets the input file for expression parsing
 * @param file Input file pointer
 */
void psa_set_input_file(FILE *file) {
    input_file = file;
}

/* =====================================
   Stack Operations
   ===================================== */

/**
 * @brief Initializes the precedence stack
 * @param stack Pointer to PrecStack structure
 */
void stack_init(PrecStack *stack) {
    stack->capacity = 32;  // Initial capacity
    stack->data = (StackSymbol*)malloc(stack->capacity * sizeof(StackSymbol));
    if (!stack->data) {
        // Handle allocation error
        exit(99);  // Internal error
    }
    stack->top = -1;
}

/**
 * @brief Pushes a symbol onto the stack
 * @param stack Pointer to PrecStack
 * @param symbol Symbol to push
 */
void stack_push(PrecStack *stack, StackSymbol symbol) {
    // Resize if necessary
    if (stack->top + 1 >= stack->capacity) {
        stack->capacity *= 2;
        stack->data = (StackSymbol*)realloc(stack->data, stack->capacity * sizeof(StackSymbol));
        if (!stack->data) {
            exit(99);  // Internal error
        }
    }
    stack->data[++stack->top] = symbol;
}

/**
 * @brief Pops a symbol from the stack
 * @param stack Pointer to PrecStack
 * @return Popped symbol
 */
StackSymbol stack_pop(PrecStack *stack) {
    if (stack->top < 0) {
        // Stack underflow - syntax error
        exit(2);  // Syntax error
    }
    return stack->data[stack->top--];
}

/**
 * @brief Finds the topmost terminal on the stack
 * @param stack Pointer to PrecStack
 * @return Topmost terminal symbol
 */
StackSymbol stack_top_terminal(PrecStack *stack) {
    for (int i = stack->top; i >= 0; i--) {
        if (stack->data[i].type == STACK_TERMINAL) {
            return stack->data[i];
        }
    }
    // Should not happen - at least $END should be there
    exit(99);  // Internal error
}

/**
 * @brief Frees the stack memory and all owned resources
 * @param stack Pointer to PrecStack
 */
void stack_free(PrecStack *stack) {
    if (stack->data) {
        // Free all tokens and AST nodes still on stack
        for (int i = 0; i <= stack->top; i++) {
            if (stack->data[i].token) {
                freeToken(stack->data[i].token);
            }
            if (stack->data[i].ast_node) {
                freeSyntaktikTree(stack->data[i].ast_node);
            }
        }
        free(stack->data);
        stack->data = NULL;
    }
    stack->top = -1;
    stack->capacity = 0;
}

/* =====================================
   Helper Functions
   ===================================== */

/**
 * @brief Converts Token to PrecSymbol
 * @param token Token from lexical analyzer
 * @return Corresponding precedence symbol
 */
PrecSymbol token_to_prec_symbol(Token *token) {
    switch (token->type) {
        case TOKEN_PLUS:      return E_PLUS;
        case TOKEN_MINUS:     return E_MINUS;
        case TOKEN_MUL:       return E_MUL;
        case TOKEN_DIV:       return E_DIV;
        case TOKEN_LT:        return E_LT;
        case TOKEN_LTE:       return E_LTE;
        case TOKEN_GT:        return E_GT;
        case TOKEN_GTE:       return E_GTE;
        case TOKEN_EQ:        return E_EQ;
        case TOKEN_NEQ:       return E_NEQ;
        
        case TOKEN_ROUND_BRACKET_START:  return E_LPAREN;
        case TOKEN_ROUND_BRACKET_END:    return E_RPAREN;
        
        case TOKEN_IDENTIFIER: return E_ID;
        case TOKEN_GLOBAL_IDENTIFIER: return E_ID;
        
        case TOKEN_INT:
        case TOKEN_FLOAT:
        case TOKEN_NUMBER:     return E_INT;
        case TOKEN_STRING:     return E_STR;
        
        case TOKEN_KEYWORD:
            if (token->value && strcmp(token->value, "is") == 0) {
                return E_IS;
            }
            
            // IMPORTANT: Check if it's a module method call (contains '.')
            // Examples: Ifj.write, Ifj.readstr, Ifj.readi, etc.
            // These are tokenized as keywords but should be treated as identifiers
            // in expression context for function calls
            if (token->value && strchr(token->value, '.') != NULL) {
                return E_ID;  // Treat as identifier so PSA can handle function call
            }
            
            // Check for type names (used with 'is' operator)
            // These include: Num, String, Null, and any class names
            if (token->value && (strcmp(token->value, "Num") == 0 || 
                                 strcmp(token->value, "String") == 0 ||
                                 strcmp(token->value, "Null") == 0)) {
                return E_ID;
            }
            
            // Check if it's "null" or "nil" - both are valid operands in expressions
            if (token->value && (strcmp(token->value, "null") == 0 || strcmp(token->value, "nil") == 0)) { 
                return E_NIL;
            }

            return E_END;
        
        // Terminating tokens
        case TOKEN_NEWLINE:
        case TOKEN_CURLY_BRACKET_START:
        case TOKEN_CURLY_BRACKET_END:
        case TOKEN_COMMA:
        case TOKEN_SYMBOL:  // ; , etc.
        case TOKEN_END_OF_FILE:
        case TOKEN_EOF:
        default:
            return E_END;
    }
}

/**
 * @brief Gets precedence relation between two symbols
 * @param stack_sym Symbol on top of stack
 * @param input_sym Current input symbol
 * @return Precedence relation (SHIFT/REDUCE/EQUAL/STOP)
 */
PrecRelation get_precedence(PrecSymbol stack_sym, PrecSymbol input_sym) {
  if (stack_sym >= E_COUNT || input_sym >= E_COUNT) {
    return PREC_NONE; // Invalid symbols
  }
  return PREC_TABLE[stack_sym][input_sym];
}

/**
 * @brief Inserts '<' handle marker onto the stack
 * @param stack Precedence stack
 */
void insert_handle_mark(PrecStack *stack) {
    // Find topmost terminal
    int terminal_index = -1;
    for (int i = stack->top; i >= 0; i--) {
        if (stack->data[i].type == STACK_TERMINAL) {
            terminal_index = i;
            break;
        }
    }

    if (terminal_index == -1) {
        exit(99); // Internal error - no terminal found
    }

    // Ensure stack has capacity
    if (stack->top + 1 >= stack->capacity) {
        stack->capacity *= 2;
        stack->data = (StackSymbol*)realloc(stack->data, stack->capacity * sizeof(StackSymbol));
        if (!stack->data) {
            exit(99);
        }
    }
    
    // Shift everything from terminal_index + 1 one position up
    for (int i = stack->top; i > terminal_index; i--) {
        stack->data[i + 1] = stack->data[i];
    }
    
    // Insert handle marker after the terminal
    stack->data[terminal_index + 1] = (StackSymbol){
        .type = STACK_HANDLE,
        .symbol = E_END,
        .token = NULL,
        .ast_node = NULL
    };
    stack->top++;
}


/* =====================================
   Reduction Rules
   ===================================== */

/**
 * @brief Checks which reduction rule matches the sequence on stack
 * @param stack Precedence stack
 * @return Matching reduction rule or RULE_INVALID
 */
ReductionRule check_reduction_rule(PrecStack *stack) {
    // Find handle marker '<'
    int handle_pos = -1;
    for (int i = stack->top; i >= 0; i--) {
        if (stack->data[i].type == STACK_HANDLE) {
            handle_pos = i;
            break;
        }
    }

    if (handle_pos == -1) {
        return RULE_INVALID;
    }

    int length = stack->top - handle_pos;

    // Rule: E → id | int | string | nil
    if (length == 1 && stack->data[handle_pos + 1].type == STACK_TERMINAL) {
        PrecSymbol sym = stack->data[handle_pos + 1].symbol;
        if (sym == E_ID || sym == E_INT || sym == E_STR || sym == E_NIL) {
            return RULE_OPERAND;
        }
    }

    // Rule: E → (E)
    if (length == 3 && stack->data[handle_pos + 1].type == STACK_TERMINAL &&
        stack->data[handle_pos + 1].symbol == E_LPAREN &&
        stack->data[handle_pos + 2].type == STACK_NONTERMINAL &&
        stack->data[handle_pos + 3].type == STACK_TERMINAL &&
        stack->data[handle_pos + 3].symbol == E_RPAREN) {
        return RULE_PARENS;
    }

    // Rule: E → E op E
    if (length == 3 && stack->data[handle_pos + 1].type == STACK_NONTERMINAL &&
        stack->data[handle_pos + 2].type == STACK_TERMINAL &&
        stack->data[handle_pos + 3].type == STACK_NONTERMINAL) {

        PrecSymbol op = stack->data[handle_pos + 2].symbol;
        if (op >= E_PLUS && op <= E_IS) { // Binary operators
            return RULE_BINARY_OP;
        }
    }

    return RULE_INVALID;
}

/**
 * @brief Performs reduction on the stack and builds AST node
 * @param stack Precedence stack
 * @return 0 on success, error code otherwise
 */
int reduce_stack(PrecStack *stack) {
    ReductionRule rule = check_reduction_rule(stack);

    if (rule == RULE_INVALID) {
        return 2; // Syntax error
    }

    // Find handle marker
    int handle_pos = -1;
    for (int i = stack->top; i >= 0; i--) {
        if (stack->data[i].type == STACK_HANDLE) {
            handle_pos = i;
            break;
        }
    }

    struct SyntaktikTreeNode *result_node = NULL;

    switch (rule) {
        case RULE_OPERAND:
            // E → id | int | string | nil
            // If there's already an AST node (e.g., function call with arguments attached),
            // use it instead of creating a new one
            if (stack->data[handle_pos + 1].ast_node) {
                result_node = stack->data[handle_pos + 1].ast_node;
                stack->data[handle_pos + 1].ast_node = NULL; // Transfer ownership
                // The token ownership was already transferred to the AST node
                stack->data[handle_pos + 1].token = NULL;
            } else {
                // No AST node yet, create one from the token
                Token *t = stack->data[handle_pos + 1].token;
                result_node = createSyntaktikTreeNode(t);
                
                // Set nodeType
                if (t->type == TOKEN_NUMBER || t->type == TOKEN_FLOAT || t->type == TOKEN_INT) result_node->nodeType = VAR_TYPE_NUM;
                else if (t->type == TOKEN_STRING) result_node->nodeType = VAR_TYPE_STRING;
                else if (t->type == TOKEN_KEYWORD && strcmp(t->value, "null") == 0) result_node->nodeType = VAR_TYPE_NULL;
                else if (t->type == TOKEN_KEYWORD && (strcmp(t->value, "true") == 0 || strcmp(t->value, "false") == 0)) result_node->nodeType = VAR_TYPE_BOOL;
                else if (t->type == TOKEN_IDENTIFIER || t->type == TOKEN_GLOBAL_IDENTIFIER) {
                     Node *local_tbl = parserGetLocalSymtable();
                     if (!local_tbl) local_tbl = parserGetClassSymtable();
                     Node *global_tbl = parserGetGlobalSymtable();
                     result_node->nodeType = getVariableType(local_tbl, global_tbl, t);
                }
                
                stack->data[handle_pos + 1].token = NULL; // Transfer ownership
            }
            break;

        case RULE_PARENS:
            // E → (E) - just use the inner expression
            result_node = stack->data[handle_pos + 2].ast_node;
            stack->data[handle_pos + 2].ast_node = NULL; // Transfer ownership
            // Free parentheses tokens
            if (stack->data[handle_pos + 1].token) freeToken(stack->data[handle_pos + 1].token);
            if (stack->data[handle_pos + 3].token) freeToken(stack->data[handle_pos + 3].token);
            break;

        case RULE_BINARY_OP:
            // E → E op E - create operator node with two children
            {
                SyntaktikTreeNode *left_node = stack->data[handle_pos + 1].ast_node;
                SyntaktikTreeNode *right_node = stack->data[handle_pos + 3].ast_node;
                Token *op_token = stack->data[handle_pos + 2].token;
                
                VarType leftType = left_node ? (VarType)left_node->nodeType : VAR_TYPE_UNDEFINED;
                VarType rightType = right_node ? (VarType)right_node->nodeType : VAR_TYPE_UNDEFINED;
                VarType resultType = VAR_TYPE_UNDEFINED;

                // Check if operation is arithmetic/string or relational
                if (op_token->type == TOKEN_PLUS || op_token->type == TOKEN_MINUS ||
                    op_token->type == TOKEN_MUL || op_token->type == TOKEN_DIV) {
                    // Arithmetic or string concatenation
                    resultType = checkArithmeticAndStringOperationTypes(leftType, rightType, op_token);
                } else if (op_token->type == TOKEN_LT || op_token->type == TOKEN_LTE ||
                           op_token->type == TOKEN_GT || op_token->type == TOKEN_GTE ||
                           op_token->type == TOKEN_EQ || op_token->type == TOKEN_NEQ) {
                    // Relational operation
                    resultType = checkRelationalOperationTypes(leftType, rightType, op_token);
                }
                
                result_node = createSyntaktikTreeNode(op_token);
                stack->data[handle_pos + 2].token = NULL; // Transfer ownership
                if (result_node) {
                    result_node->nodeType = resultType;
                    addChildNode(result_node, left_node);
                    addChildNode(result_node, right_node);
                    stack->data[handle_pos + 1].ast_node = NULL;
                    stack->data[handle_pos + 3].ast_node = NULL;
                }
            }
            break;

        default:
            return 2;
    }

    if (!result_node) {
        return 99; // Internal error - failed to create AST node
    }

    // Pop the handle and reduced symbols
    stack->top = handle_pos - 1;
    
    // Push the resulting non-terminal with AST node
    stack_push(stack, (StackSymbol){
        .type = STACK_NONTERMINAL,
        .symbol = E_END,
        .token = NULL,
        .ast_node = result_node
    });

    return 0; // Success
}


/* =====================================
   Main Expression Parser
   ===================================== */

/**
 * @brief Parses function arguments, which is a comma-separated list of expressions.
 * @param current_ptr Pointer to the current token pointer. Will be updated.
 * @param arg_tokens_out Output array of argument tokens (for semantic checking). Can be NULL.
 * @param arg_count_out Output parameter for number of arguments. Can be NULL.
 * @return 0 on success, error code otherwise.
 */
static int parse_function_args(Token **current_ptr, Token ***arg_tokens_out, int *arg_count_out) {
    Token *current = *current_ptr;
    
    // Dynamic array for argument tokens
    int arg_capacity = 4;
    int arg_count = 0;
    Token **arg_tokens = NULL;
    
    if (arg_tokens_out) {
        arg_tokens = malloc(arg_capacity * sizeof(Token*));
        if (!arg_tokens) return 99;
    }

    // Check if there are no arguments
    if (current->type == TOKEN_ROUND_BRACKET_END) {
        *current_ptr = current;
        if (arg_tokens_out) *arg_tokens_out = arg_tokens;
        if (arg_count_out) *arg_count_out = arg_count;
        return 0;
    }

    // Parse the first argument expression
    // Save parent BEFORE calling parseExpression (which might modify it)
    struct SyntaktikTreeNode *parent = getCurrentParentNode();
    Token *after_expr = NULL;
    struct SyntaktikTreeNode *arg_ast = NULL;
    int result = parseExpression(current, &after_expr, &arg_ast);
    if (result != 0) {
        if (arg_tokens) free(arg_tokens);
        return result;
    }
    
    // Store argument token for semantic checking
    if (arg_tokens && arg_ast && arg_ast->token) {
        if (arg_count >= arg_capacity) {
            arg_capacity *= 2;
            Token **new_tokens = realloc(arg_tokens, arg_capacity * sizeof(Token*));
            if (!new_tokens) {
                free(arg_tokens);
                return 99;
            }
            arg_tokens = new_tokens;
        }
        arg_tokens[arg_count++] = arg_ast->token;
    }
    
    // Attach argument to current parent (function call node)
    if (arg_ast && parent) {
        addChildNode(parent, arg_ast);
    }
    
    current = after_expr;

    // Parse subsequent arguments separated by commas
    while (current->type == TOKEN_COMMA) {
        // Consume comma and get next token
        current = getNextToken(&Error, input_file, NULL, NULL);
        if (!current) {
            if (arg_tokens) free(arg_tokens);
            return Error != 0 ? Error : 99;
        }

        // Parse the next expression
        arg_ast = NULL;
        result = parseExpression(current, &after_expr, &arg_ast);
        if (result != 0) {
            freeToken(current);
            if (arg_tokens) free(arg_tokens);
            return result;
        }
        
        // Store argument token
        if (arg_tokens && arg_ast && arg_ast->token) {
            if (arg_count >= arg_capacity) {
                arg_capacity *= 2;
                Token **new_tokens = realloc(arg_tokens, arg_capacity * sizeof(Token*));
                if (!new_tokens) {
                    free(arg_tokens);
                    return 100;
                }
                arg_tokens = new_tokens;
            }
            arg_tokens[arg_count++] = arg_ast->token;
        }
        
        // Attach argument to current parent (parent was saved before the loop)
        if (arg_ast && parent) {
            addChildNode(parent, arg_ast);
        }
        
        current = after_expr;
    }

    *current_ptr = current;
    if (arg_tokens_out) *arg_tokens_out = arg_tokens;
    if (arg_count_out) *arg_count_out = arg_count;
    return 0;
}


/**
 * @brief Runs the precedence syntactic analysis (PSA) for an expression
 * @param first_token Token that was already read and belongs to the expression
 * @param next_token_out Output parameter for the token after the expression
 * @param ast_result_out Output parameter for the resulting AST subtree (can be NULL)
 * @return 0 on success, otherwise an error code
 */
int parseExpression(Token *first_token, Token **next_token_out, struct SyntaktikTreeNode **ast_result_out) {
    PrecStack stack;
    stack_init(&stack);

    // Push $END onto stack
    stack_push(&stack, (StackSymbol){
        .type = STACK_TERMINAL,
        .symbol = E_END,
        .token = NULL,
        .ast_node = NULL
    });

    Token *current = first_token;
    PrecSymbol input_sym = token_to_prec_symbol(current);
    
    // Main PSA loop
    while (1) {
        StackSymbol top_term = stack_top_terminal(&stack);
        PrecRelation relation = get_precedence(top_term.symbol, input_sym);
        
        switch (relation) {
            case PREC_SHIFT:
            case PREC_EQUAL:
                // For both SHIFT and EQUAL, we shift the input token onto the stack.
                // For SHIFT, we also insert a handle marker before it.
                if (relation == PREC_SHIFT) {
                    insert_handle_mark(&stack);
                }
                
                // Special case for function calls: id followed by (
                if (input_sym == E_ID) {
                    Token *next = getNextToken(&Error, input_file, NULL, NULL);
                    if (!next) {
                        if (current != first_token) freeToken(current);
                        stack_free(&stack);
                        return Error ? Error : 99;
                    }

                    if (next->type == TOKEN_ROUND_BRACKET_START) {
                        // Function call: create AST node
                        
                        // Check if function is defined (with any parameter count)
                        Node *class_tbl = parserGetClassSymtable();
                        Node *global_tbl = parserGetGlobalSymtable();
                        Node *func_node = NULL;
                        
                        // Functions are stored as "name{paramCount}" in symbol table
                        char func_key[256];
                        
                        // Check for built-in functions (Ifj.xxx format)
                        bool is_builtin = (current->value && strchr(current->value, '.') != NULL);
                        
                        // NOTE: We don't check for undefined functions here because functions
                        // may be defined later in the file (forward references). Undefined
                        // function check is done in semantic analysis after parsing is complete.
                        
                        struct SyntaktikTreeNode *call_node = createSyntaktikTreeNode(current);
                        
                        // Push terminal with AST node (token ownership transferred to AST)
                        stack_push(&stack, (StackSymbol){
                            .type = STACK_TERMINAL,
                            .symbol = E_ID,
                            .token = NULL,  // Token owned by call_node, not by stack symbol
                            .ast_node = call_node
                        });
                        
                        current = NULL; // Ownership transferred

                        // Free the '(' token
                        freeToken(next);

                        // Get the token that starts the arguments (or is ')')
                        current = getNextToken(&Error, input_file, NULL, NULL);
                        if (!current) {
                            stack_free(&stack);
                            return 99;
                        }

                        // Parse arguments and attach to call_node
                        struct SyntaktikTreeNode *saved_parent = getCurrentParentNode();
                        setCurrentParentNode(call_node);
                        
                        Token **arg_tokens = NULL;
                        int arg_count = 0;
                        int arg_result = parse_function_args(&current, &arg_tokens, &arg_count);
                        
                        setCurrentParentNode(saved_parent);
                        
                        if (arg_result != 0) {
                            if (current) freeToken(current);
                            if (arg_tokens) free(arg_tokens);
                            stack_free(&stack);
                            return arg_result;
                        }
                        
                        // Second pass: Now that we know arg_count, search for function with exact parameter count
                        if (!is_builtin) {
                            snprintf(func_key, sizeof(func_key), "%s{%d}", call_node->token->value, arg_count);
                            if (class_tbl) func_node = search(class_tbl, func_key);
                            if (!func_node && global_tbl) func_node = search(global_tbl, func_key);
                            
                            if (!func_node) {
                                // Also check for getter if arg_count == 0
                                if (arg_count == 0) {
                                    snprintf(func_key, sizeof(func_key), "%s{get}", call_node->token->value);
                                    if (class_tbl) func_node = search(class_tbl, func_key);
                                    if (!func_node && global_tbl) func_node = search(global_tbl, func_key);
                                }
                            }
                            
                            // If still not found, it could be:
                            // 1. Undefined function (error 3) - but we can't check this during parsing
                            //    because of forward references
                            // 2. Wrong parameter count (error 5) - but we can't check this either
                            //    because we don't know all function overloads yet
                            // So we just continue - semantic analysis will catch these errors
                        }
                        
                        
                        // Built-in function parameter checking
                        if (is_builtin && arg_tokens && arg_count >= 0) {
                            Node *local_tbl = parserGetLocalSymtable();
                            if (!local_tbl) local_tbl = parserGetClassSymtable();
                            Node *global_tbl = parserGetGlobalSymtable();
                            
                            // Create function token for built-in check
                            Token func_tok;
                            func_tok.type = TOKEN_KEYWORD;
                            func_tok.value = call_node->token ? call_node->token->value : NULL;
                            
                            VarType return_type = VAR_TYPE_UNDEFINED;
                            int builtin_result = checkBuiltInFunctionCallParams(local_tbl, global_tbl, &func_tok, arg_tokens, arg_count, &return_type);
                            
                            if (builtin_result != 0) {
                                // Built-in function parameter error
                                // checkBuiltInFunctionCallParams returns: 0=OK, 1=type mismatch, -1=unknown, -2=error
                                // We need to translate to proper error codes
                                if (current) freeToken(current);
                                if (arg_tokens) free(arg_tokens);
                                stack_free(&stack);
                                if (builtin_result == 1) {
                                    return 5;  // ERROR_SEMANTIC_ARGUMENTS - wrong param count or type
                                } else {
                                    return 99;  // ERROR_INTERNAL for unknown or other errors
                                }
                            }
                        }
                        
                        // Free argument tokens array (tokens themselves are owned by AST)
                        if (arg_tokens) free(arg_tokens);

                        // Expect ')'
                        if (current->type != TOKEN_ROUND_BRACKET_END) {
                            if (current) freeToken(current);
                            stack_free(&stack);
                            return 2; // Syntax error: expected ')'
                        }

                        // Reduce the function call to E
                        reduce_stack(&stack);
                        
                        // Get token after ')'
                        freeToken(current);
                        current = getNextToken(&Error, input_file, NULL, NULL);
                        if (!current) {
                            stack_free(&stack);
                            return Error;
                        }
                        input_sym = token_to_prec_symbol(current);
                        continue; // Restart loop with new token
                    } else {
                        // Not a function call, just identifier
                        // Check for undefined variables (only for local identifiers, not global or keywords)
                        // Global variables (__xxx) are implicitly defined with null value
                        if (current->type == TOKEN_IDENTIFIER) {
                            Node *local_tbl = parserGetLocalSymtable();
                            Node *class_tbl = parserGetClassSymtable();
                            Node *global_tbl = parserGetGlobalSymtable();
                            
                            bool found = false;
                            if (local_tbl && search(local_tbl, current->value)) found = true;
                            if (!found && class_tbl && search(class_tbl, current->value)) found = true;
                            if (!found && global_tbl && search(global_tbl, current->value)) found = true;
                            
                            // Also check for getters (stored as "name{get}")
                            if (!found) {
                                char getter_key[256];
                                snprintf(getter_key, sizeof(getter_key), "%s{get}", current->value);
                                if (class_tbl && search(class_tbl, getter_key)) found = true;
                                if (!found && global_tbl && search(global_tbl, getter_key)) found = true;
                            }
                            
                            // Also check for shadowed variables
                            if (!found && local_tbl) {
                                char shadowed_name[256];
                                for (int i = 1; i < 10; i++) {
                                    snprintf(shadowed_name, sizeof(shadowed_name), "%s_%d", current->value, i);
                                    if (search(local_tbl, shadowed_name)) {
                                        found = true;
                                        break;
                                    }
                                }
                            }
                            
                            if (!found) {
                                // Undefined variable - semantic error 3
                                freeToken(current);
                                freeToken(next);
                                stack_free(&stack);
                                return 3;
                            }
                        }
                        
                        stack_push(&stack, (StackSymbol){
                            .type = STACK_TERMINAL,
                            .symbol = input_sym,
                            .token = current,
                            .ast_node = NULL
                        });
                        current = NULL; // Ownership transferred
                        current = next;
                        input_sym = token_to_prec_symbol(current);
                    }
                } else {
                    // Regular shift for other tokens
                    
                    // Check for undefined variables (only for local TOKEN_IDENTIFIER, not global or keywords)
                    // Global variables (__xxx) are implicitly defined with null value
                    if (input_sym == E_ID && current->type == TOKEN_IDENTIFIER) {
                        // Check if variable is defined in symbol tables
                        Node *local_tbl = parserGetLocalSymtable();
                        Node *class_tbl = parserGetClassSymtable();
                        Node *global_tbl = parserGetGlobalSymtable();
                        
                        bool found = false;
                        if (local_tbl && search(local_tbl, current->value)) found = true;
                        if (!found && class_tbl && search(class_tbl, current->value)) found = true;
                        if (!found && global_tbl && search(global_tbl, current->value)) found = true;
                        
                        // Also check for getters (stored as "name{get}")
                        if (!found) {
                            char getter_key[256];
                            snprintf(getter_key, sizeof(getter_key), "%s{get}", current->value);
                            if (class_tbl && search(class_tbl, getter_key)) found = true;
                            if (!found && global_tbl && search(global_tbl, getter_key)) found = true;
                        }
                        
                        // Also check for shadowed variables (with _1, _2 suffix)
                        if (!found && local_tbl) {
                            char shadowed_name[256];
                            for (int i = 1; i < 10; i++) {
                                snprintf(shadowed_name, sizeof(shadowed_name), "%s_%d", current->value, i);
                                if (search(local_tbl, shadowed_name)) {
                                    found = true;
                                    break;
                                }
                            }
                        }
                        
                        if (!found) {
                            // Undefined variable - semantic error 3
                            if (current != first_token) freeToken(current);
                            stack_free(&stack);
                            return 3;
                        }
                    }
                    
                    stack_push(&stack, (StackSymbol){
                        .type = STACK_TERMINAL,
                        .symbol = input_sym,
                        .token = current,
                        .ast_node = NULL
                    });
                    current = NULL; // Ownership transferred
                    current = getNextToken(&Error, input_file, NULL, NULL);
                    if (!current) {
                        stack_free(&stack);
                        return Error != 0 ? Error : 99;
                    }
                    input_sym = token_to_prec_symbol(current);
                }
                break;

            case PREC_REDUCE:
                if (reduce_stack(&stack) != 0) {
                    if (current != first_token) freeToken(current);
                    stack_free(&stack);
                    return 2; // Syntax error during reduction
                }
                // Do not consume the input token, re-evaluate with the new stack top.
                break;

            case PREC_STOP:
                // Check for successful parse: stack should contain $E$
                if (stack.top == 1 && stack.data[0].symbol == E_END && stack.data[1].type == STACK_NONTERMINAL) {
                    *next_token_out = current; // Pass back the token that terminated the expression
                    
                    // Return the AST node if requested
                    if (ast_result_out) {
                        *ast_result_out = stack.data[1].ast_node;
                        stack.data[1].ast_node = NULL; // Transfer ownership
                    }
                    
                    stack_free(&stack);
                    return 0; // Success
                } else {
                    // Error state
                    if (current && current != first_token) freeToken(current);
                    stack_free(&stack);
                    return 2; // Syntax error
                }
                break;

            default: // PREC_NONE or other error
                if (current && current != first_token) freeToken(current);
                stack_free(&stack);
                return 2; // Syntax error: invalid precedence relation
        }
    }
}