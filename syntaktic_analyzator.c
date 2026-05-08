/*
 * syntaktic_analyzator.c
 * ============================================================
 * Syntaktic analyzer implementation for the IFJ25 project.
 * ============================================================
 * Authors:
 * Tomáš Zovníček xzvonit00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#include "syntaktic_analyzator.h"
#include "prec_analytics.h"
#include "symtable.h"
#include "syntaktik_tree.h"
#include "semantic_functions.h"
#include "error_codes.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------- Debug macros ---------------------------------------------------- */
// Uncomment to enable debug output:
// #define DEBUG_PARSER
// Uncomment to enable symbol table and AST printing:
// #define PRINT_TABLES

#ifdef DEBUG_PARSER
  #define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
  #define DEBUG_PRINT(...) ((void)0)
#endif

/* -------- Static variables ------------------------------------------------ */
static Token *currentToken = NULL;
static FILE *input_file = NULL;
int Error = 0;

// Requirements tracking
static bool prologSeen = false;
static bool programHasMain = false;
static char *currentClassName = NULL;

// Symbol table management
static Node *global_symtable = NULL;
static Node *currentClassTable = NULL;
static Node *currentLocalTable = NULL;

// Dynamic variable occurrence counter for unique naming
typedef struct VarCounter {
    char *var_name;
    int count;
    struct VarCounter *next;
} VarCounter;

static VarCounter *varCounterList = NULL;

// Scope depth tracking for variable redefinition detection
static int current_scope_depth = 0;


char *strdup(const char *str)
{
    int n = strlen(str) + 1;
    char *dup = malloc(n);
    if(dup)
    {
        strcpy(dup, str);
    }
    return dup;
}


/*
 * Function Local Tables Storage
 * ==============================
 * Since each function has its own local symbol table, and the code generator
 * needs access to ALL of them (not just the current one), we maintain a linked
 * list that maps function keys to their local tables.
 * 
 * Usage:
 * - During parsing: store_function_local_table("main{0}", current_local_table)
 * - During code gen: Node *locals = get_function_local_table("main{0}")
 * 
 * Function key format:
 * - Regular functions: "functionName{paramCount}" (e.g., "main{0}", "add{2}")
 * - Getters: "propertyName{get}" (e.g., "unicorn{get}")
 * - Setters: "propertyName{set}" (e.g., "unicorn{set}")
 */
typedef struct FunctionLocalTableEntry {
    char *function_key;  // e.g., "main{0}", "unicorn{get}", "unicorn{set}"
    Node *local_table;
    struct FunctionLocalTableEntry *next;
} FunctionLocalTableEntry;

static FunctionLocalTableEntry *function_local_tables = NULL;

/*
 * Class Tables Storage
 * ====================
 * Each class has its own symbol table containing methods, getters, and setters.
 * We store these in a linked list for code generator access.
 * 
 * Usage:
 * - During parsing: store_class_table("Program", current_class_table)
 * - During code gen: Node *class_tbl = get_class_table("Program")
 */
typedef struct ClassTableEntry {
    char *class_name;  // e.g., "Ifj", "Program"
    Node *class_table;
    struct ClassTableEntry *next;
} ClassTableEntry;

static ClassTableEntry *classTables = NULL;

// Store local table for a function
void storeFunctionLocalTable(const char *function_key, Node *local_table) {
    FunctionLocalTableEntry *entry = malloc(sizeof(FunctionLocalTableEntry));
    if (!entry) return;
    
    entry->function_key = strdup(function_key);
    entry->local_table = local_table;
    entry->next = function_local_tables;
    function_local_tables = entry;
}

// Retrieve local table for a function (for code generator)
Node* getFunctionLocalTable(const char *function_key) {
    FunctionLocalTableEntry *current = function_local_tables;
    while (current) {
        if (strcmp(current->function_key, function_key) == 0) {
            return current->local_table;
        }
        current = current->next;
    }
    return NULL;
}

// Free all function local table entries (call at end of parsing)
void freeFunctionLocalTables(void) {
    FunctionLocalTableEntry *current = function_local_tables;
    while (current) {
        FunctionLocalTableEntry *next = current->next;
        free(current->function_key);
        // Don't dispose local_table here - generator still needs it
        free(current);
        current = next;
    }
    function_local_tables = NULL;
}

// Store class table for a class
void storeClassTable(const char *class_name, Node *class_table) {
    ClassTableEntry *entry = malloc(sizeof(ClassTableEntry));
    if (!entry) return;
    
    entry->class_name = strdup(class_name);
    entry->class_table = class_table;
    entry->next = classTables;
    classTables = entry;
}

// Retrieve class table for a class (for code generator)
Node* getClassTable(const char *class_name) {
    ClassTableEntry *current = classTables;
    while (current) {
        if (strcmp(current->class_name, class_name) == 0) {
            return current->class_table;
        }
        current = current->next;
    }
    return NULL;
}

/* Syntactic tree globals */
static SyntaktikTreeNode *syntaxTreeRoot = NULL;
static SyntaktikTreeNode *currentParentNode = NULL;


// Get next occurrence number for a variable name (0 for first, 1 for second, etc.)
// Peek at occurrence count without incrementing
static int peekVarOcurrence(const char *var_name) {
    VarCounter *current = varCounterList;
    while (current) {
        if (strcmp(current->var_name, var_name) == 0) {
            return current->count;
        }
        current = current->next;
    }
    return 0;  // Not found
}

static int getVarOccurrence(const char *var_name) {
    VarCounter *current = varCounterList;
    while (current) {
        if (strcmp(current->var_name, var_name) == 0) {
            return current->count++;
        }
        current = current->next;
    }
    
    // First occurrence - add to list
    VarCounter *new_counter = malloc(sizeof(VarCounter));
    if (!new_counter) return 0;
    
    new_counter->var_name = strdup(var_name);
    if (!new_counter->var_name) {
        free(new_counter);
        return 0;
    }
    
    new_counter->count = 1;  // Next will be 1
    new_counter->next = varCounterList;
    varCounterList = new_counter;
    
    return 0;  // First occurrence
}

// Reset variable counters when starting new function
static void resetVarCounters(void) {
  VarCounter *current = varCounterList;
  while (current) {
    VarCounter *next = current->next;
    free(current->var_name);
    free(current);
    current = next;
  }
  varCounterList = NULL;
}

// Scope tracking for variable redefinition
typedef struct ScopeVar {
    char *name;
    struct ScopeVar *next;
} ScopeVar;

typedef struct ScopeLevel {
    ScopeVar *vars;
    struct ScopeLevel *next;
} ScopeLevel;

static ScopeLevel *scopeStack = NULL;

static void pushScope(void) {
    ScopeLevel *newScope = malloc(sizeof(ScopeLevel));
    if (newScope) {
        newScope->vars = NULL;
        newScope->next = scopeStack;
        scopeStack = newScope;
    }
    current_scope_depth++;
}

static void popScope(void) {
    if (scopeStack) {
        ScopeLevel *top = scopeStack;
        scopeStack = top->next;
        
        // Free vars in this scope
        ScopeVar *v = top->vars;
        while (v) {
            ScopeVar *next = v->next;
            free(v->name);
            free(v);
            v = next;
        }
        free(top);
    }
    if (current_scope_depth > 0) current_scope_depth--;
}

static void addVarToCurrentScope(const char *name) {
    if (!scopeStack) return;
    
    ScopeVar *v = malloc(sizeof(ScopeVar));
    if (v) {
        v->name = strdup(name);
        v->next = scopeStack->vars;
        scopeStack->vars = v;
    }
}

static bool isVarDeclaredInCurrentScope(const char *name) {
    if (!scopeStack) return false;
    
    ScopeVar *v = scopeStack->vars;
    while (v) {
        if (strcmp(v->name, name) == 0) return true;
        v = v->next;
    }
    return false;
}

// Scope depth management wrappers
static void enterScope(void) {
  pushScope();
}

static void exitScope(void) {
  popScope();
}

// Check if variable is already defined in current scope
static bool isVarInCurrentScope(Node *symtable, const char *var_name) {
  (void)symtable; // Unused now
  if (!var_name) return false;
  
  return isVarDeclaredInCurrentScope(var_name);
}// =======================================================================
//   Tree Helper Functions  
// =======================================================================
// For better readability was necessery to break naming convention here.
void psa_AttachTokenToCurrentParent(Token *token) { 
  if (currentParentNode && token) {
    SyntaktikTreeNode *expr_node = createSyntaktikTreeNode(token);
    if (expr_node) {
      addChildNode(currentParentNode, expr_node);
    }
  }
}

// For better readability was necessery to break naming convention here.
void psa_Attach_AST_ToCurrentParent(SyntaktikTreeNode *ast_node) {
  if (currentParentNode && ast_node) {
    addChildNode(currentParentNode, ast_node);
  }
}

// =======================================================================
//   Functions to access parser internal data for testing and code generation
// =======================================================================
Node *parserGetGlobalSymtable(void) { return global_symtable; }
Node *parserGetClassSymtable(void) { return currentClassTable; }
Node *parserGetLocalSymtable(void) { return currentLocalTable; }

SyntaktikTreeNode *getSyntaxTreeRoot(void) { return syntaxTreeRoot; }
SyntaktikTreeNode *getCurrentParentNode(void) { return currentParentNode; }
void setCurrentParentNode(SyntaktikTreeNode *n) { currentParentNode = n; }

/* =====================================
   Helper Functions
   ===================================== */

#ifdef PRINT_TABLES
static void printSymtable_impl(Node *root, int indent, FILE *output) {
  if (!root || !output) return;
  
  // In-order traversal: left, root, right
  printSymtable_impl(root->left, indent + 2, output);
  
  // Print current node with indentation
  for (int i = 0; i < indent; i++) {
    fprintf(output, " ");
  }
  fprintf(output, "%s", root->key);
  
  if (root->data) {
    fprintf(output, " [");
    switch (root->data->type) {
      case TYPE_FUNCTION:
        fprintf(output, "FUNCTION");
        if (root->data->varType > 0) {
          fprintf(output, "(%d params)", root->data->varType);
        }
        break;
      case TYPE_VARIABLE:
        fprintf(output, "VARIABLE");
        break;
      case TYPE_GETTER:
        fprintf(output, "GETTER");
        break;
      case TYPE_SETTER:
        fprintf(output, "SETTER");
        break;
    }
    fprintf(output, "]");
  }
  fprintf(output, "\n");
  
  printSymtable_impl(root->right, indent + 2, output);
}

static void printSyntaktikTree_impl(SyntaktikTreeNode *node, int indent) {
  if (!node) return;
  
  // Print current node with indentation
  for (int i = 0; i < indent; i++) {
    fprintf(stderr, "  ");
  }
  
  if (node->token) {
    fprintf(stderr, "Token: type=%d, value='%s'\n", 
            node->token->type,
            node->token->value ? node->token->value : "(null)");
  } else {
    fprintf(stderr, "ROOT\n");
  }
  
  // Print children
  for (size_t i = 0; i < node->childrenCount; i++) {
    if (node->children[i]) {
      printSyntaktikTree_impl(node->children[i], indent + 1);
    }
  }
}

#define printSymtable(root, indent, output) printSymtable_impl(root, indent, output)
#define printSyntaktikTree(node, indent) printSyntaktikTree_impl(node, indent)
#else
#define printSymtable(root, indent, output) ((void)0)
#define printSyntaktikTree(node, indent) ((void)0)
#endif // PRINT_TABLES

static int nextToken(void) {
  if (currentToken) {
    freeToken(currentToken);
    currentToken = NULL;
  }
  currentToken = getNextToken(&Error, input_file, NULL, NULL);
  if (Error != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s (Lexer Error)\n", Error,
            __func__);
    return Error;
  }
  if (!currentToken) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s (Failed to get token)\n",
            __func__);
    return ERROR_INTERNAL;
  }
  DEBUG_PRINT("[DEBUG PARSER] Current Token: type=%d, value='%s'\n",
          currentToken->type,
          currentToken->value ? currentToken->value : "NULL");
  return ERROR_SUCCESS;
}

static int skipNewlines(void) {
  while (currentToken && currentToken->type == TOKEN_NEWLINE) {
    int res = nextToken();
    if (res != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", res, __func__);
      return res;
    }
  }
  return ERROR_SUCCESS;
}

/* =====================================
   Grammar Rules with Semantic Actions
   ===================================== */

// Register a parameter in the current local symbol table and in the occurrence counter.
static int registerParamToken(Token *param_token) {
  if (!param_token) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }

  if (searchData(currentLocalTable, param_token->value)) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_REDEFINITION from %s\n", __func__);
    return ERROR_SEMANTIC_REDEFINITION;
  }

  int semResult = addVariable(&currentLocalTable, param_token, VAR_TYPE_UNDEFINED);
  if (semResult != 0) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s\n", __func__);
    return ERROR_SEMANTIC_UNDEFINED;
  }

  // Register parameter in occurrence counter for shadowing support
  getVarOccurrence(param_token->value);
  
  // Add to current scope
  addVarToCurrentScope(param_token->value);

  return ERROR_SUCCESS;
}

// Create AST node for parameter and attach to current parent.
// Does not advance tokens.
static int attachParamNode(Token *param_token) {
  SyntaktikTreeNode *param_node = createSyntaktikTreeNode(param_token);
  if (!param_node) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  addChildNode(currentParentNode, param_node);
  return ERROR_SUCCESS;
}

int ruleParams(int *paramCount) {
  int count = 0;

  if (!currentToken || currentToken->type != TOKEN_IDENTIFIER) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }

  // First parameter
  int res = registerParamToken(currentToken);
  if (res != 0) {
    return res;
  }
  res = attachParamNode(currentToken);
  if (res != 0) {
    return res;
  }
  count++;

  // Advance past the identifier
  currentToken = NULL;
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }

  // Additional parameters separated by commas
  while (currentToken && currentToken->type == TOKEN_COMMA) {
    if (nextToken() != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
      return Error;
    }

    if (currentToken->type != TOKEN_IDENTIFIER) {
      DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
      return ERROR_SYNTAX;
    }

    res = registerParamToken(currentToken);
    if (res != 0) {
      return res;
    }
    res = attachParamNode(currentToken);
    if (res != 0) {
      return res;
    }
    count++;

    currentToken = NULL;
    if (nextToken() != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
      return Error;
    }
  }

  if (paramCount) {
    *paramCount = count;
  }

  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}

int ruleImportStmt(void) {
  SyntaktikTreeNode *import_node = createSyntaktikTreeNode(currentToken);
  if (!import_node) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  addChildNode(currentParentNode, import_node);
  currentToken = NULL;
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }
  skipNewlines();  // Import může být na více řádcích
  if (currentToken->type != TOKEN_STRING) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }
  if (currentToken->value && strcmp(currentToken->value, "\"ifj25\"") == 0) {
    prologSeen = true;
  }
  SyntaktikTreeNode *str_node = createSyntaktikTreeNode(currentToken);
  if (!str_node) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  addChildNode(import_node, str_node);
  currentToken = NULL;
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }
  skipNewlines();  // 'for' může být na dalším řádku
  if (currentToken->type != TOKEN_KEYWORD ||
      strcmp(currentToken->value, "for") != 0) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }
  skipNewlines();  // Název modulu může být na dalším řádku
  if (currentToken->type != TOKEN_IDENTIFIER &&
      currentToken->type != TOKEN_KEYWORD) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }
  
  // Insert imported module into global symbol table
  if (!insert(currentToken->value, &global_symtable)) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  
  SyntaktikTreeNode *IDnode = createSyntaktikTreeNode(currentToken);
  if (!IDnode) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  addChildNode(import_node, IDnode);
  currentToken = NULL;
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }
  // After import statement, we should have a newline or EOF
  // Skip the newline(s) and let the caller continue parsing
  skipNewlines();
  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}

int ruleImportList(void) {
  skipNewlines();
  if (currentToken && currentToken->type == TOKEN_KEYWORD &&
      strcmp(currentToken->value, "import") == 0) {
    int result = ruleImportStmt();
    if (result != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
      return result;
    }
    return ruleImportList();
  }
  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}

int ruleProgram(void) {
  syntaxTreeRoot = createSyntaktikTreeNode(NULL);
  if (!syntaxTreeRoot) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  currentParentNode = syntaxTreeRoot;
  int result = ruleImportList();
  if (result != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
    return result;
  }
  result = ruleClassList();
  if (result != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
    return result;
  }
  skipNewlines();
  if (!prologSeen) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s (Prologue not seen)\n",
            __func__);
    return ERROR_SYNTAX;
  }
  if (!programHasMain) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s (Main not found)\n", __func__);
    return ERROR_SEMANTIC_UNDEFINED;
  }
  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}

int ruleClassList(void) {
  skipNewlines();
  if (currentToken && currentToken->type == TOKEN_KEYWORD &&
      strcmp(currentToken->value, "class") == 0) {
    int result = ruleClassDef();
    if (result != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
      return result;
    }
    return ruleClassList();
  }
  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}

int ruleClassDef(void) {
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }
  if (currentToken->type != TOKEN_IDENTIFIER) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }
  if (searchData(global_symtable, currentToken->value)) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s\n", __func__);
    return ERROR_SEMANTIC_UNDEFINED;
  }
  // Add class to global symbol table
  if (!insert(currentToken->value, &global_symtable)) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  currentClassName = strdup(currentToken->value);
  if (!currentClassName) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  SyntaktikTreeNode *class_node = createSyntaktikTreeNode(currentToken);
  if (!class_node) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  addChildNode(currentParentNode, class_node);
  currentToken = NULL;
  SyntaktikTreeNode *savedParent = currentParentNode;
  currentParentNode = class_node;
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }
  int result = ruleClassBody();
  
#ifdef PRINT_TABLES
  fprintf(stderr, "\n--- Class '%s' Symbol Table ---\n", current_class_name ? current_class_name : "unknown");
  if (current_class_table) {
    printSymtable(current_class_table, 0, stderr);
  } else {
    fprintf(stderr, "(empty)\n");
  }
  fprintf(stderr, "-------------------------------\n\n");
#endif
  
  // Store class table for code generator access
  if (currentClassName && currentClassTable) {
    storeClassTable(currentClassName, currentClassTable);
  }
  
  // Don't dispose class_table - it stays in memory for code generator
  currentClassTable = NULL;
  free(currentClassName);
  currentClassName = NULL;
  currentParentNode = savedParent;
  DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
  return result;
}

int ruleClassBody(void) {
  // Expecting '{'
  if (currentToken->type != TOKEN_CURLY_BRACKET_START) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }

  // Advance past '{'
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }

  // Expecting newline after '{'
  if (currentToken->type != TOKEN_NEWLINE) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }

  // Advance past newline
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }

  // Advance past newline
  skipNewlines();
  int result = ruleMemberList();
  if (result != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
    return result;
  }

  // Expecting '}'
  skipNewlines();
  if (currentToken->type != TOKEN_CURLY_BRACKET_END) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }

  // Advance past '}'
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }

  // Expecting EOF or NEWLINE after class body
  if (currentToken->type != TOKEN_END_OF_FILE &&
      currentToken->type != TOKEN_NEWLINE) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }

  // Advance past newline if present
  if (currentToken->type == TOKEN_NEWLINE) {
    if (nextToken() != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
      return Error;
    }
  }

  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}

int ruleMemberList(void) {
  skipNewlines();
  // Expecting member or end of member list
  if (currentToken && currentToken->type != TOKEN_CURLY_BRACKET_END &&
      currentToken->type != TOKEN_END_OF_FILE) {
    int result = ruleMember();
    if (result != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
      return result;
    }
    return ruleMemberList();
  }

  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}


// Helper to finish member processing and clean up
static int memberFinish(SyntaktikTreeNode *savedParent, int result, char *funcName) {
#ifdef PRINT_TABLES
  fprintf(stderr, "\n--- Local Symbol Table (function in class '%s') ---\n",
          currentClassName ? currentClassName : "unknown");
  if (currentLocalTable) {
    printSymtable(currentLocalTable, 0, stderr);
  } else {
    fprintf(stderr, "(empty)\n");
  }
  fprintf(stderr, "---------------------------------------------------\n\n");
#endif
  if (funcName) free(funcName);
  currentLocalTable = NULL;
  currentParentNode = savedParent;
  DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
  return result;
}



// Check for function redefinition and add to class table
static int checkAndAddFunction(Token *funcToken, int paramCount, const char *keyFormat, 
                                   int (*addFunc)(Node**, Token*, int)) {
  char key[256];
  snprintf(key, sizeof(key), keyFormat, funcToken->value, paramCount);
  
  if (searchData(currentClassTable, key)) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_REDEFINITION from %s\n", __func__);
    return ERROR_SEMANTIC_REDEFINITION;
  }
  
  int semResult = addFunc(&currentClassTable, funcToken, paramCount);
  if (semResult != 0) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s\n", __func__);
    return ERROR_SEMANTIC_UNDEFINED;
  }
  
  return 0;
}


// Check for getter redefinition and add to class table
static int checkAndAddGetter(Token *funcToken, const char *key) {
  if (searchData(currentClassTable, key)) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_REDEFINITION from %s (getter redefinition)\n", __func__);
    return ERROR_SEMANTIC_REDEFINITION;
  }
  
  int semResult = addGetterFunction(&currentClassTable, funcToken);
  if (semResult != 0) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s\n", __func__);
    return ERROR_SEMANTIC_UNDEFINED;
  }
  
  return 0;
}


// Check for setter redefinition and add to class table
static int checkAndAddSetter(Token *funcToken, const char *key) {
  if (searchData(currentClassTable, key)) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_REDEFINITION from %s (setter redefinition)\n", __func__);
    return ERROR_SEMANTIC_REDEFINITION;
  }
  
  int semResult = addSetterFunction(&currentClassTable, funcToken);
  if (semResult != 0) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s\n", __func__);
    return ERROR_SEMANTIC_UNDEFINED;
  }
  
  return 0;
}


// Process regular function
static int processRegularFunction(SyntaktikTreeNode *savedParent, Token *funcToken, 
                                     char *funcName, int *paramCount) {

  
  if (nextToken() != 0) return memberFinish(savedParent, Error, funcName);

  // Push scope for function parameters and body
  pushScope();

  if (currentToken->type == TOKEN_IDENTIFIER) {
    int r = ruleParams(paramCount);
    if (r != 0) {
      popScope();
      return memberFinish(savedParent, r, funcName);
    }
  }

  if (currentToken->type != TOKEN_ROUND_BRACKET_END) {
    popScope();
    return memberFinish(savedParent, 2, funcName);
  }

  char funcKey[256];
  snprintf(funcKey, sizeof(funcKey), "%s{%d}", funcToken->value, *paramCount);
  
  int checkResult = checkAndAddFunction(funcToken, *paramCount, "%s{%d}", addFunction);
  if (checkResult != 0) {
    popScope();
    return memberFinish(savedParent, checkResult, funcName);
  }

  if (nextToken() != 0) {
    popScope();
    return memberFinish(savedParent, Error, funcName);
  }

  int result = ruleBlock(false);
  
  // Pop scope after function body
  popScope();
  
  if (result == 0) {
    storeFunctionLocalTable(funcKey, currentLocalTable);
  }
  
  return memberFinish(savedParent, result, funcName);
}


// Process getter function
static int process_getter(SyntaktikTreeNode *savedParent, Token *funcToken, char *funcName) {
  char getterKey[256];
  snprintf(getterKey, sizeof(getterKey), "%s{get}", funcToken->value);
  
  int checkResult = checkAndAddGetter(funcToken, getterKey);
  if (checkResult != 0) {
    return memberFinish(savedParent, checkResult, funcName);
  }
  
  int result = ruleBlock(false);
  
  if (result == 0) {
    storeFunctionLocalTable(getterKey, currentLocalTable);
  }
  
  return memberFinish(savedParent, result, funcName);
}


// Process setter function
static int processSetter(SyntaktikTreeNode *savedParent, Token *funcToken, 
                          char *funcName, int *paramCount) {
                    
  // Expecting '('
  if (nextToken() != 0) return memberFinish(savedParent, Error, funcName);

  if (currentToken->type != TOKEN_ROUND_BRACKET_START) {
    return memberFinish(savedParent, 2, funcName);
  }

  
  if (nextToken() != 0) return memberFinish(savedParent, Error, funcName);

  if (currentToken->type == TOKEN_IDENTIFIER) {
    int r = ruleParams(paramCount);
    if (r != 0) return memberFinish(savedParent, r, funcName);
  }

  // Expecting ')'
  if (currentToken->type != TOKEN_ROUND_BRACKET_END) {
    return memberFinish(savedParent, 2, funcName);
  }

  char setterKey[256];
  snprintf(setterKey, sizeof(setterKey), "%s{set}", funcToken->value);
  
  int checkResult = checkAndAddSetter(funcToken, setterKey);
  if (checkResult != 0) {
    return memberFinish(savedParent, checkResult, funcName);
  }

  if (nextToken() != 0) return memberFinish(savedParent, Error, funcName);

  int result = ruleBlock(false);
  
  if (result == 0) {
    storeFunctionLocalTable(setterKey, currentLocalTable);
  }
  
  return memberFinish(savedParent, result, funcName);
}


// Handle variable member declaration
static int handleVarMember(bool isStatic) {
  if (isStatic) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }

  // Create variable declaration node
  SyntaktikTreeNode *varDeclNode = createSyntaktikTreeNode(currentToken);
  if (!varDeclNode) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }

  addChildNode(currentParentNode, varDeclNode);
  currentToken = NULL;
  
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }

  // Expecting identifier for variable name
  if (currentToken->type != TOKEN_IDENTIFIER) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }

  // Check for variable redefinition in the current class scope
  if (searchData(currentClassTable, currentToken->value)) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_REDEFINITION from %s\n", __func__);
    return ERROR_SEMANTIC_REDEFINITION;
  }

  // Add variable to class symbol table
  Token *varToken = currentToken;
  int semResult = addVariable(&currentClassTable, varToken, VAR_TYPE_UNDEFINED);
  if (semResult != 0) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s\n", __func__);
    return ERROR_SEMANTIC_UNDEFINED;
  }

  // Register variable in occurrence counter for shadowing support
  SyntaktikTreeNode *IDnode = createSyntaktikTreeNode(currentToken);
  if (!IDnode) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }

  addChildNode(varDeclNode, IDnode);
  currentToken = NULL;
  
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }

  // Check for optional assignment
  if (currentToken->type == TOKEN_ASSIGN) {
    if (nextToken() != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
      return Error;
    }
    Token *exprToken = currentToken;
    currentToken = NULL;
    SyntaktikTreeNode *expr_ast = NULL;
    int result = parseExpression(exprToken, &currentToken, &expr_ast);
    if (result != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
      return result;
    }
    psa_Attach_AST_ToCurrentParent(expr_ast);
  }


  // Expecting newline after variable declaration
  if (currentToken->type != TOKEN_NEWLINE) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }

  // Advance to next token
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    return Error;
  }



  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}

// Handle member starting with identifier (function, getter, setter)
static int handleIdentifierMember(void) {
  Token *funcToken = currentToken;
  char *funcName = strdup(currentToken->value);

  // Check for 'main' function in 'Program' class
  if (currentClassName && strcmp(currentClassName, "Program") == 0 &&
      strcmp(currentToken->value, "main") == 0) {
    programHasMain = true;
  }

  // Create member node
  SyntaktikTreeNode *memberNode = createSyntaktikTreeNode(currentToken);
  if (!memberNode) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    free(funcName);
    return ERROR_INTERNAL;
  }

  // Add member node to current parent
  addChildNode(currentParentNode, memberNode);
  currentToken = NULL;
  
  SyntaktikTreeNode *savedParent = currentParentNode;
  currentParentNode = memberNode;

  // Advance to next token
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    free(funcName);
    return Error;
  }

  currentLocalTable = NULL;
  resetVarCounters();
  int paramCount = 0;

  // Determine member type based on next token
  if (currentToken->type == TOKEN_ROUND_BRACKET_START) {
    return processRegularFunction(savedParent, funcToken, funcName, &paramCount);
    
  } else if (currentToken->type == TOKEN_CURLY_BRACKET_START) {
    return process_getter(savedParent, funcToken, funcName);
    
  } else if (currentToken->type == TOKEN_ASSIGN) {
    return processSetter(savedParent, funcToken, funcName, &paramCount);
  }

  return memberFinish(savedParent, 2, funcName);
}

int ruleMember(void) {
  bool isStatic = false;

  if (currentToken->type == TOKEN_KEYWORD &&
      strcmp(currentToken->value, "static") == 0) {
    isStatic = true;
    if (nextToken() != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
      return Error;
    }
  }

  if (currentToken->type == TOKEN_KEYWORD &&
      strcmp(currentToken->value, "var") == 0) {
    return handleVarMember(isStatic);
  }

  if (currentToken->type == TOKEN_IDENTIFIER) {
    return handleIdentifierMember();
  }

  DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
  return ERROR_SYNTAX;
}


int ruleBlock(bool isStandalone) {
  if (currentToken->type != TOKEN_CURLY_BRACKET_START) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    return ERROR_SYNTAX;
  }
  
  if (isStandalone) {
    pushScope();
  }

  SyntaktikTreeNode *blockMode = createSyntaktikTreeNode(currentToken);
  if (!blockMode) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    if (isStandalone) popScope();
    return ERROR_INTERNAL;
  }
  addChildNode(currentParentNode, blockMode);
  currentToken = NULL;
  SyntaktikTreeNode *savedParent = currentParentNode;
  currentParentNode = blockMode;
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    currentParentNode = savedParent;
    if (isStandalone) popScope();
    return Error;
  }
  if (currentToken->type != TOKEN_NEWLINE) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    currentParentNode = savedParent;
    if (isStandalone) popScope();
    return ERROR_SYNTAX;
  }
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    currentParentNode = savedParent;
    if (isStandalone) popScope();
    return Error;
  }
  int result = ruleStatementList();
  if (result != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
    currentParentNode = savedParent;
    if (isStandalone) popScope();
    return result;
  }
  skipNewlines();
  if (currentToken->type != TOKEN_CURLY_BRACKET_END) {
    DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
    currentParentNode = savedParent;
    if (isStandalone) popScope();
    return ERROR_SYNTAX;
  }
  if (nextToken() != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
    currentParentNode = savedParent;
    if (isStandalone) popScope();
    return Error;
  }
  currentParentNode = savedParent;
  
  if (isStandalone) {
    popScope();
  }

  DEBUG_PRINT("DEBUG: Returning 0 farom %s\n", __func__);
  return ERROR_SUCCESS;
}

int ruleStatementList(void) {
  skipNewlines();
  if (currentToken && currentToken->type != TOKEN_CURLY_BRACKET_END &&
      currentToken->type != TOKEN_END_OF_FILE) {
    int result = ruleStatement();
    if (result != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
      return result;
    }
    return ruleStatementList();
  }
  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}

int rule_args(void) {
  Token *exprToken = currentToken;
  currentToken = NULL;
  SyntaktikTreeNode *arg_ast = NULL;
  int result = parseExpression(exprToken, &currentToken, &arg_ast);
  if (result != 0) {
    DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
    return result;
  }
  psa_Attach_AST_ToCurrentParent(arg_ast);
  while (currentToken && currentToken->type == TOKEN_COMMA) {
    if (nextToken() != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
      return Error;
    }
    exprToken = currentToken;
    currentToken = NULL;
    arg_ast = NULL;
    result = parseExpression(exprToken, &currentToken, &arg_ast);
    if (result != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
      return result;
    }
    psa_Attach_AST_ToCurrentParent(arg_ast);
  }
  DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
  return ERROR_SUCCESS;
}

int ruleStatement(void) {
  if (!currentToken) {
    DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
    return ERROR_INTERNAL;
  }
  if (currentToken->type == TOKEN_CURLY_BRACKET_START) {
    return ruleBlock(true); // Standalone block
  }
  if (currentToken->type == TOKEN_KEYWORD) {
    if (strcmp(currentToken->value, "if") == 0) {
      SyntaktikTreeNode *if_node = createSyntaktikTreeNode(currentToken);
      if (!if_node) {
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }
      addChildNode(currentParentNode, if_node);
      currentToken = NULL;
      SyntaktikTreeNode *savedParent = currentParentNode;
      currentParentNode = if_node;
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      if (currentToken->type != TOKEN_ROUND_BRACKET_START) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      Token *exprToken = currentToken;
      currentToken = NULL;
      SyntaktikTreeNode *expr_ast = NULL;
      int result = parseExpression(exprToken, &currentToken, &expr_ast);
      if (result != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
        return result;
      }
      psa_Attach_AST_ToCurrentParent(expr_ast);
      if (currentToken->type != TOKEN_ROUND_BRACKET_END) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      result = ruleBlock(true); // If body creates new scope, increment depth
      if (result != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
        return result;
      }
      skipNewlines();
      if (currentToken && currentToken->type == TOKEN_KEYWORD &&
          strcmp(currentToken->value, "else") == 0) {
        if (nextToken() != 0) {
          DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
          return Error;
        }
        result = ruleBlock(true); // Else body creates new scope, increment depth
        if (result != 0) {
          DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
          return result;
        }
      }
      currentParentNode = savedParent;
      DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
      return ERROR_SUCCESS;
    }
    if (strcmp(currentToken->value, "while") == 0) {
      SyntaktikTreeNode *while_node = createSyntaktikTreeNode(currentToken);
      if (!while_node) {
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }
      addChildNode(currentParentNode, while_node);
      currentToken = NULL;
      SyntaktikTreeNode *savedParent = currentParentNode;
      currentParentNode = while_node;
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      if (currentToken->type != TOKEN_ROUND_BRACKET_START) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      Token *exprToken = currentToken;
      currentToken = NULL;
      SyntaktikTreeNode *expr_ast = NULL;
      int result = parseExpression(exprToken, &currentToken, &expr_ast);
      if (result != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
        return result;
      }
      psa_Attach_AST_ToCurrentParent(expr_ast);
      if (currentToken->type != TOKEN_ROUND_BRACKET_END) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      result = ruleBlock(true); // While body creates new scope, increment depth
      if (result != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
        return result;
      }
      currentParentNode = savedParent;
      DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
      return ERROR_SUCCESS;
    }
    if (strcmp(currentToken->value, "return") == 0) {
      SyntaktikTreeNode *return_node = createSyntaktikTreeNode(currentToken);
      if (!return_node) {
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }
      addChildNode(currentParentNode, return_node);
      currentToken = NULL;
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      if (currentToken->type != TOKEN_NEWLINE &&
          currentToken->type != TOKEN_CURLY_BRACKET_END) {
        SyntaktikTreeNode *savedParent = currentParentNode;
        currentParentNode = return_node;
        Token *exprToken = currentToken;
        currentToken = NULL;
        SyntaktikTreeNode *expr_ast = NULL;
        int result = parseExpression(exprToken, &currentToken, &expr_ast);
        if (result != 0) {
          DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
          return result;
        }
        psa_Attach_AST_ToCurrentParent(expr_ast);
        currentParentNode = savedParent;
      }
      if (currentToken->type == TOKEN_NEWLINE) {
        if (nextToken() != 0) {
          DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
          return Error;
        }
      } else if (currentToken->type != TOKEN_CURLY_BRACKET_END &&
                 currentToken->type != TOKEN_END_OF_FILE) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }
      DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
      return ERROR_SUCCESS;
    }
    if (strcmp(currentToken->value, "var") == 0) {
      SyntaktikTreeNode *var_decl_node = createSyntaktikTreeNode(currentToken);
      if (!var_decl_node) {
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }
      addChildNode(currentParentNode, var_decl_node);
      currentToken = NULL;
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      if (currentToken->type != TOKEN_IDENTIFIER) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }
      
      // Check for redefinition in current scope (not shadowing from outer scope)
      if (isVarInCurrentScope(currentLocalTable, currentToken->value)) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_REDEFINITION from %s (var redef in same scope)\n", __func__);
        return ERROR_SEMANTIC_REDEFINITION;
      }
      // Create unique variable name for shadowing support
      // Global variables (starting with __) are stored without suffix
      char uniqueVarName[256];
      bool isGlobalVar = (currentToken->value[0] == '_' && currentToken->value[1] == '_');
      
      if (!isGlobalVar) {
        // For local variables, check if variable already exists (from parameters or outer scopes)
        // Use occurrence counter for shadowing support
        int occurrence = getVarOccurrence(currentToken->value);
        
        if (occurrence > 0) {
          // This is shadowing in a nested block - use unique name
          snprintf(uniqueVarName, sizeof(uniqueVarName), "%s_%d", 
                   currentToken->value, occurrence);
        } else {
          // First occurrence - use base name
          snprintf(uniqueVarName, sizeof(uniqueVarName), "%s", currentToken->value);
        }
      } else {
        snprintf(uniqueVarName, sizeof(uniqueVarName), "%s", currentToken->value);
      }
      
      // Create a token with unique name for symbol table
      // Need to allocate on heap since symbol table stores the pointer
      Token *symToken = malloc(sizeof(Token));
      if (!symToken) {
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }
      symToken->type = currentToken->type;
      symToken->value = strdup(uniqueVarName);
      if (!symToken->value) {
        free(symToken);
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }
      
      // Add to global or local symbol table based on variable name
      Node **targetTable = isGlobalVar ? &global_symtable : &currentLocalTable;
      int semResult = addVariable(targetTable, symToken, VAR_TYPE_UNDEFINED);
      if (semResult != 0) {
        freeToken(symToken);
        DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s\n", __func__);
        return ERROR_SEMANTIC_UNDEFINED;
      }
      
      // Add to current scope if it's a local variable
      if (!isGlobalVar) {
          addVarToCurrentScope(currentToken->value);
      }
      
      SyntaktikTreeNode *IDnode = createSyntaktikTreeNode(currentToken);
      if (!IDnode) {
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }
      addChildNode(var_decl_node, IDnode);
      currentToken = NULL;
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      if (currentToken->type == TOKEN_ASSIGN) {
        if (nextToken() != 0) {
          DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
          return Error;
        }
        SyntaktikTreeNode *savedParent = currentParentNode;
        currentParentNode = var_decl_node;
        Token *exprToken = currentToken;
        currentToken = NULL;
        SyntaktikTreeNode *expr_ast = NULL;
        int result = parseExpression(exprToken, &currentToken, &expr_ast);
        if (result != 0) {
          DEBUG_PRINT("DEBUG: Returning %d from %s\n", result, __func__);
          return result;
        }
        psa_Attach_AST_ToCurrentParent(expr_ast);
        currentParentNode = savedParent;
      }
      if (currentToken->type != TOKEN_NEWLINE) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
      return ERROR_SUCCESS;
    }
  }
  if (currentToken->type == TOKEN_IDENTIFIER ||
      currentToken->type == TOKEN_GLOBAL_IDENTIFIER ||
      (currentToken->type == TOKEN_KEYWORD &&
       strchr(currentToken->value, '.'))) {
    Token *id_token = currentToken;
    currentToken = NULL;
    if (nextToken() != 0) {
      freeToken(id_token);
      DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
      return Error;
    }
    if (currentToken->type == TOKEN_ASSIGN) {
      // For assignments without 'var', check if variable is defined
      char setter_name[256];
      snprintf(setter_name, sizeof(setter_name), "%s{set}", id_token->value);
      bool is_setter = (searchData(currentClassTable, setter_name) != 0);
      
      if (!is_setter) {
        // Regular variable assignment - must be previously defined with 'var' or be a parameter
        bool isGlobalVar = (id_token->value[0] == '_' && id_token->value[1] == '_');
        bool varExists;
        
        if (isGlobalVar) {
          // Global variables can be auto-created on assignment
          varExists = (searchData(global_symtable, id_token->value) != 0);
          if (!varExists) {
            // Auto-create global variable
            Token *symToken = malloc(sizeof(Token));
            if (!symToken) {
              freeToken(id_token);
              DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
              return ERROR_INTERNAL;
            }
            symToken->type = id_token->type;
            symToken->value = strdup(id_token->value);
            if (!symToken->value) {
              free(symToken);
              freeToken(id_token);
              DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
              return ERROR_INTERNAL;
            }
            int semResult = addVariable(&global_symtable, symToken, VAR_TYPE_UNDEFINED);
            if (semResult != 0) {
              freeToken(symToken);
              freeToken(id_token);
              DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s\n", __func__);
              return ERROR_SEMANTIC_UNDEFINED;
            }
          }
        } else {
          // Local variables must be defined with 'var' or be parameters
          varExists = (searchData(currentLocalTable, id_token->value) != 0);
          if (!varExists) {
            freeToken(id_token);
            DEBUG_PRINT("DEBUG: Returning ERROR_SEMANTIC_UNDEFINED from %s (undefined local variable)\n", __func__);
            return ERROR_SEMANTIC_UNDEFINED;
          }
        }
      }
      // If is_setter, we don't insert anything into local symbol table
      
      SyntaktikTreeNode *assignNode = createSyntaktikTreeNode(currentToken);
      if (!assignNode) {
        freeToken(id_token);
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }

      addChildNode(currentParentNode, assignNode);
      currentToken = NULL;
      SyntaktikTreeNode *IDnode = createSyntaktikTreeNode(id_token);
      if (!IDnode) {
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }

      addChildNode(assignNode, IDnode);
      id_token = NULL;
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }

      SyntaktikTreeNode *savedParent = currentParentNode;
      currentParentNode = assignNode;
      Token *exprToken = currentToken;
      currentToken = NULL;
      SyntaktikTreeNode *expr_ast = NULL;
      int res = parseExpression(exprToken, &currentToken, &expr_ast);
      if (res != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", res, __func__);
        return res;
      }

      psa_Attach_AST_ToCurrentParent(expr_ast);
      currentParentNode = savedParent;
      if (currentToken->type == TOKEN_NEWLINE) {
        if (nextToken() != 0) {
          DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
          return Error;
        }
      } else if (currentToken->type != TOKEN_CURLY_BRACKET_END &&
                 currentToken->type != TOKEN_END_OF_FILE) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }

      DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
      return ERROR_SUCCESS;
    } 
    else if (currentToken->type == TOKEN_ROUND_BRACKET_START) {
      SyntaktikTreeNode *call_node = createSyntaktikTreeNode(id_token);
      if (!call_node) {
        freeToken(currentToken);
        DEBUG_PRINT("DEBUG: Returning ERROR_INTERNAL from %s\n", __func__);
        return ERROR_INTERNAL;
      }
      addChildNode(currentParentNode, call_node);
      id_token = NULL;
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      SyntaktikTreeNode *savedParent = currentParentNode;
      currentParentNode = call_node;
      if (currentToken->type != TOKEN_ROUND_BRACKET_END) {
        int res = rule_args();
        if (res != 0) {
          DEBUG_PRINT("DEBUG: Returning %d from %s\n", res, __func__);
          return res;
        }
      }
      if (currentToken->type != TOKEN_ROUND_BRACKET_END) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }
      if (nextToken() != 0) {
        DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
        return Error;
      }
      if (currentToken->type == TOKEN_NEWLINE) {
        if (nextToken() != 0) {
          DEBUG_PRINT("DEBUG: Returning %d from %s\n", Error, __func__);
          return Error;
        }
      } else if (currentToken->type != TOKEN_CURLY_BRACKET_END &&
                 currentToken->type != TOKEN_END_OF_FILE) {
        DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
        return ERROR_SYNTAX;
      }
      currentParentNode = savedParent;
      DEBUG_PRINT("DEBUG: Returning 0 from %s\n", __func__);
      return ERROR_SUCCESS;
    } else {
      freeToken(id_token);
      DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s\n", __func__);
      return ERROR_SYNTAX;
    }
  }

  DEBUG_PRINT("DEBUG: Returning ERROR_SYNTAX from %s (Unrecognized statement)\n",
          __func__);
  return ERROR_SYNTAX;
}

int parserRun(Token *token, FILE *file) {
  input_file = file;
  psa_set_input_file(file);
  if (token) {
    currentToken = token;
  } else {
    if (nextToken() != 0) {
      int res = Error ? Error : 99;
      fprintf(stderr, "Parser finished with result code: %d\n", res);
      return res;
    }
  }
  // Skip leading newlines before parsing starts
  skipNewlines();
  int result = ruleProgram();
  
  if (result == 0) {
    // Run semantic analysis on the AST
    result = performSemanticAnalysis(syntaxTreeRoot, global_symtable);
    if (result != 0) {
      DEBUG_PRINT("DEBUG: Returning %d from %s (Semantic Analysis Failed)\n", result, __func__);
    }
  }
  
#ifdef PRINT_TABLES
  // Print symbol tables and AST
  fprintf(stderr, "\n========== GLOBAL SYMBOL TABLE ==========\n");
  if (global_symtable) {
    printSymtable(global_symtable, 0, stderr);
  } else {
    fprintf(stderr, "(empty)\n");
  }
  fprintf(stderr, "=========================================\n\n");
  
  fprintf(stderr, "========== ABSTRACT SYNTAX TREE ==========\n");
  if (syntax_tree_root) {
    printSyntaktikTree(syntax_tree_root, 0);
  } else {
    fprintf(stderr, "(empty)\n");
  }
  fprintf(stderr, "==========================================\n\n");
#endif

  
  //printf("Parser finished with result code: %d\n", result);
  return result;
}

void parser_cleanup(void) {
  if (syntaxTreeRoot) {
    freeSyntaktikTree(syntaxTreeRoot);
    syntaxTreeRoot = NULL;
  }
  if (global_symtable) {
    dispose(global_symtable);
    global_symtable = NULL;
  }
  if (currentToken) {
    freeToken(currentToken);
    currentToken = NULL;
  }
  /* clear other parser state */
  currentClassTable = NULL;
  currentLocalTable = NULL;
  currentParentNode = NULL;
}