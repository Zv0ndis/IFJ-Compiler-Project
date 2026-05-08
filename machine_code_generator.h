/* 
 * machine_code_generator.h
 * ============================================================
 * Machine code generator implementation for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Vojtěch Kadlec FIT VUT v Brně
 * Aleš Obr xobrale00 FIT VUT v Brně
 * Tomáš Zovníček xzvonit00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#include "syntaktic_analyzator.h"
#include "token.h"
#include "symtable.h"
#include "syntaktik_tree.h"
#include "semantic_functions.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "v_stack.h"



/**
 * Three-address code representation
 * Stores operator and operands for intermediate code generation
 */
typedef struct ThreeAdressCode{
	Token* operator;
	Token* operandA;
	Token* operandB;
	Token* result;
	struct ThreeAdressCode *next;
}ThreeAdressCode;

/**
/**
 * Generate IFJCode25 from abstract syntax tree
 * Outputs generated IFJCode25 instructions to stdout
 * @param file input file to translate
 * @return 0 on success, error code otherwise
 */
int generateIFJCode25(FILE *file);

/**
 * Convert abstract syntax tree to three-address code
 * @param abs_tree pointer to abstract syntax tree
 * @param variables pointer to symbol table with variables
 * @param stack variable stack for intermediate results
 * @return pointer to generated three-address code or NULL on error
 */
ThreeAdressCode* absToThreeAdressCode(SyntaktikTreeNode *abs_tree,Node *variables,vStack *stack);

/**
 * Convert three-address code to IFJCode25 instructions
 * @param code pointer to three-address code
 * @param keywords pointer to symbol table with IFJCode25 keywords
 * @param class_table pointer to class symbol table
 */
void ThreeAdressCodeToIFJCode25(ThreeAdressCode *code,Node *keywords,Node *class_table);

/**
 * Check if token represents a variable or literal value
 * @param token pointer to token to check
 * @return true if token is variable or literal, false otherwise
 */
bool isVariable(Token* token);

/**
 * Prepare keyword tree with IFJCode25 operations
 * @return pointer to initialized keyword tree or NULL on error
 */
Node *prepareKeywordsTree();

/**
 * Find the most recently created variable in symbol table
 * @param root pointer to symbol table root
 * @return pointer to newest variable or NULL
 */
char* findNewestVariable(Node *root);

/**
 * Create a new temporary variable with unique name
 * @param stack variable stack
 * @return dynamically allocated variable name string
 */
char* createNewVariable(vStack *stack);

/**
 * Dispose keyword tree and free allocated memory
 * @param root pointer to keyword tree root
 */
void disposeKeyWordTree(Node* root);

/**
 * Print formatted variable for IFJCode25 output
 * Handles different variable types (identifiers, literals, globals)
 * @param token pointer to token representing variable
 */
void printFormatedVariable(Token* token);

/**
 * Print operator from keywords tree and return parameter count
 * @param token pointer to operator token
 * @param keywords pointer to keywords symbol table
 * @return number of parameters for the operation
 */
int printOperator(Token* token,Node *keywords);

/**
 * Initialize three-address code structure
 * @return pointer to initialized ThreeAdressCode or NULL on error
 */
ThreeAdressCode* initThreeAdressCode();

/**
 * Print three-address code for debugging purposes
 * @param code pointer to three-address code to print
 */
void printThreeAdressCode(ThreeAdressCode *code);

/**
 * Convert abstract syntax tree to IFJCode25 instructions
 * Core code generation function with stack-based evaluation
 * @param abs_tree pointer to abstract syntax tree node
 * @param keywords pointer to keywords symbol table
 * @param stack variable stack for managing temporary values
 */
void absToIFJCode25(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack);

/**
 * Declare global variables in IFJCode25 output
 * @param global_table pointer to global symbol table
 */
void declareGlobalVariables(Node*global_table);

/**
 * Declare generator temporary variables
 * @param abs_tree pointer to abstract syntax tree
 * @param stack variable stack
 */
void declareGeneratorVariables(SyntaktikTreeNode *abs_tree,vStack *stack);

/**
 * Process addition, subtraction, and multiplication operations
 * @param abs_tree pointer to operator node
 * @param keywords pointer to keywords table
 * @param stack variable stack
 */
void processPlusMinusMul(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack);

/**
 * Process division operation with type conversions
 * @param abs_tree pointer to division node
 * @param keywords pointer to keywords table
 * @param stack variable stack
 */
static void processDiv(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack, char* result_var, char *child_results[100]);

/**
 * Process string conversion operations
 * @param abs_tree pointer to conversion node
 * @param keywords pointer to keywords table
 * @param stack variable stack
 */
static void processStr(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack, char* result_var);

/**
 * Process all other operations not handled by specific functions
 * @param abs_tree pointer to operation node
 * @param keywords pointer to keywords table
 * @param stack variable stack
 */
void processEverithingElse(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack);