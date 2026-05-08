/* 
* prec_analytics.h
* ============================================================
 * Precedence analyzer header for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Tomáš Zovníček xzvonit00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */
#include "syntaktik_tree.h"
#include "syntaktic_analyzator.h"
#include "token.h"

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

/* =====================================
   Transition to precedence expression analysis
   ===================================== */

typedef enum {
  PREC_NONE = 0,
  PREC_SHIFT,
  PREC_EQUAL,
  PREC_REDUCE,
  PREC_STOP
} PrecRelation;

typedef enum {
    E_PLUS,      // 0  +
    E_MINUS,     // 1  -
    E_MUL,       // 2  *
    E_DIV,       // 3  /
    E_LT,        // 4  <
    E_LTE,       // 5  <=
    E_GT,        // 6  >
    E_GTE,       // 7  >=
    E_EQ,        // 8  ==
    E_NEQ,       // 9  !=
    E_IS,        // 10 is (type check operator)
    E_LPAREN,    // 11 (
    E_RPAREN,    // 12 )
    E_ID,        // 13 identifier
    E_INT,       // 14 integer literal
    E_STR,       // 15 string literal
    E_NIL,       // 16 nil/null
    E_END,       // 17 $ (end of expression)
    E_COUNT      // Total count (18)
} PrecSymbol;

extern const PrecRelation PREC_TABLE[E_COUNT][E_COUNT];


/**
 * @brief Sets the input file for expression parsing
 * @param file Input file pointer
 */
void psa_set_input_file(FILE *file);

/* =====================================
   Stack for PSA
   ===================================== */

typedef enum {
  STACK_TERMINAL,
  STACK_NONTERMINAL, // Expression E
  STACK_HANDLE       // '<' marker for handle start
} StackSymbolType;

// Forward declaration for AST node
struct SyntaktikTreeNode;

typedef struct {
  StackSymbolType type;
  PrecSymbol symbol; // If type is TERMINAL
  Token *token;      // Token for terminals (owned by stack)
  struct SyntaktikTreeNode *ast_node; // AST node for nonterminals
} StackSymbol;

typedef struct {
  StackSymbol *data;
  int top;
  int capacity;
} PrecStack;

// Stack operations
void stack_init(PrecStack *stack);
void stack_push(PrecStack *stack, StackSymbol symbol);
StackSymbol stack_pop(PrecStack *stack);
StackSymbol stack_top_terminal(PrecStack *stack); // Finds topmost terminal
void stack_free(PrecStack *stack);



/* =====================================
   Reduction rules
   ===================================== */

// Example reduction rules:
// E → E + E
// E → E * E
// E → (E)
// E → id
// E → literal

typedef enum {
  RULE_OPERAND,   // E → id | int | string | nil
  RULE_BINARY_OP, // E → E op E
  RULE_PARENS,    // E → (E)
  RULE_INVALID
} ReductionRule;

/**
 * @brief Checks which reduction rule matches the sequence on stack
 * @param stack Precedence stack
 * @return Matching reduction rule or RULE_INVALID
 */
ReductionRule check_reduction_rule(PrecStack *stack);

/**
 * @brief Converts TokenType to PrecSymbol
 * @param type Token type from lexical analyzer
 * @return Corresponding precedence symbol
 */
PrecSymbol token_to_prec_symbol(Token *token);

/**
 * @brief Gets precedence relation between two symbols
 * @param stack_sym Symbol on top of stack
 * @param input_sym Current input symbol
 * @return Precedence relation (SHIFT/REDUCE/EQUAL/STOP)
 */
PrecRelation get_precedence(PrecSymbol stack_sym, PrecSymbol input_sym);

/**
 * @brief Inserts '<' handle marker onto the stack
 * @param stack Precedence stack
 */
void insert_handle_mark(PrecStack *stack);


/* =====================================
   Parsing functions
   ===================================== */

/**
 * Runs the precedence syntactic analysis (PSA) for an expression.
 *
 * Uses a precedence table and a stack of symbols.
 * Stops at a terminating token (e.g. ')', ';', '{', '}'),
 * which signals a return to recursive descent parsing.
 *
 * @param first_token Token that was already read and belongs to the expression.
 * @param next_token_out Pointer to store the token after the expression ends
 * @param ast_result_out Pointer to store the resulting AST subtree (can be NULL)
 * @return int 0 on success, otherwise an error code
 */
int parseExpression(Token *first_token, Token **next_token_out, struct SyntaktikTreeNode **ast_result_out);

/**
 * @brief Sets the terminating token for PSA depending on context.
 *
 * Used when parsing expressions inside parentheses or conditions,
 * to indicate where the PSA should stop and return control.
 */
void psa_set_end_token(TokenType type);

#endif // PRECEDENCE_H
