/* 
* syntaktic_analyzator.h
* ============================================================
 * Syntaktic analyzer header for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Tomáš Zovníček xzvonit00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#ifndef PARSER_H
#define PARSER_H

#include "lexikal_analyzator.h" // header for lexikal
#include "syntaktik_tree.h"     // syntax tree header
#include "symtable.h"           // symtable header
#include "error_codes.h"        // error codes header
#include "token.h"


/**
 * @brief Get the global symbol table (contains all classes)
 * @return Pointer to the root of the global symbol table BST
 */
Node* parserGetGlobalSymtable(void);

/**
 * @brief Get the current class symbol table (contains class members)
 * @return Pointer to the root of the current class symbol table BST
 */
Node* parserGetClassSymtable(void);

/**
 * @brief Get the current local symbol table (contains function variables)
 * @return Pointer to the root of the current local symbol table BST
 */
Node* parserGetLocalSymtable(void);

/**
 * @brief Get the local symbol table for a specific function
 * @param function_key Function key in format "name{paramcount}" or "name{get}" or "name{set}"
 * @return Pointer to the local symbol table for that function, or NULL if not found
 */
Node* getFunctionLocalTable(const char *function_key);

/**
 * @brief Get the class symbol table for a specific class
 * @param class_name Name of the class (e.g., "Program", "Ifj")
 * @return Pointer to the class symbol table, or NULL if not found
 */
Node* getClassTable(const char *class_name);

/* Parser -> expose these helpers for prec_anal.c */
int parser_insert_identifier(const char *name);
void psa_AttachTokenToCurrentParent(Token *token);
Token* token_dup(const Token* t);

/* Accessors for syntax tree current parent (optional) */
SyntaktikTreeNode *getCurrentParentNode(void);
void setCurrentParentNode(SyntaktikTreeNode *n);
SyntaktikTreeNode *getSyntaxTreeRoot(void);

/*
 * Release parser-owned resources (syntax tree, symbol tables, token).
 * Call this after you finish using data produced by parserRun().
 */
void parser_cleanup(void);


/**
 * Runs syntactic analyzation
 *
 * Function accepts token from lexikal analyzation
 * and checks if grammar equals language IFJ25 / Wren
 * If token is null, it ask lexer for next token and proceed checks
 * @param token is actual token for checking
 * @return int 0 if success, else raises error
 */
int parserRun(Token *token, FILE *file);

/* =====================================
   Recursive descent rules LL(1)
   ===================================== */

/* Main rules for LL(1) */

/**
 * program → class_list EOF
 */
int ruleProgram(void);

/**
 * class_list → class_def class_list | ε
 */
int ruleClassList(void);

/**
 * class_def → "class" identifier class_body
 */
int ruleClassDef(void);

/**
 * class_body → "{" member_list "}"
 */
int ruleClassBody(void);

/**
 * member_list → member member_list | ε
 */
int ruleMemberList(void);

/**
 * member → "var" id ("=" expr)? ";" | id "(" params? ")" block
 */
int ruleMember(void);

/**
 * block → "{" stmt_list "}"
 * @param is_standalone True for standalone blocks, false for if/while/function bodies
 */
int ruleBlock(bool is_standalone);

/**
 * params → id ("," id)*
 * @param param_count Output parameter for number of parameters parsed
 * @return 0 on success, error code on failure
 */
int ruleParams(int *param_count);


/* ---------- Statements ---------- */
int ruleImportList(void);
int ruleImportStmt(void);
/**
 * stmt_list → stmt stmt_list | ε
 */
int ruleStatementList(void);

/**
 * stmt → "if" "(" expr ")" block ("else" block)? 
 *      | "while" "(" expr ")" block
 *      | "return" expr? ";"
 *      | "var" id ("=" expr)? ";"
 *      | id stmt_suffix

 *      | block
 */
int ruleStatement(void);
int rule_stmt_suffix(void);

/**
 * args → expr ("," expr)* | ε
 */
int rule_args(void);

#endif // PARSER_H
