/*
 * machine_code_generator.c
 * ============================================================
 * Machine code generator implementation for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Vojtech Kadlec xkadlev00 FIT VUT v Brně
 * Aleš Obr xobrale00 FIT VUT v Brně
 * Tomáš Zovníček xzvonit00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#include "machine_code_generator.h"

static int global_var_counter = 0;  //Glbal count for temp variables
static int global_label_counter = 0;  // Global count for labels  

// Track declared variables to avoid redeclaration
static char declared_vars[1000][50] = {0};
static int declared_vars_count = 0;


static void reset_declared_vars() {
    declared_vars_count = 0;
    memset(declared_vars, 0, sizeof(declared_vars));
}

static bool is_var_declared(const char *var_name) {
    for (int i = 0; i < declared_vars_count; i++) {
        if (strcmp(declared_vars[i], var_name) == 0) {
            return true;
        }
    }
    return false;
}

static void mark_var_declared(const char *var_name) {
    if (declared_vars_count < 1000 && !is_var_declared(var_name)) {
        strncpy(declared_vars[declared_vars_count++], var_name, 49);
    }
}

// Collect all variable declarations from the AST (recursive scan)
static void collect_variable_declarations(SyntaktikTreeNode *node) {
    if (!node) return;
    
    // Check if this is a VAR declaration
    if (node->token && node->token->value && strcmp(node->token->value, "var") == 0) {
        if (node->childrenCount > 0 && node->children[0]->token && node->children[0]->token->value) {
            mark_var_declared(node->children[0]->token->value);
        }
    }
    
    // Recursively scan children
    for (size_t i = 0; i < node->childrenCount; i++) {
        collect_variable_declarations(node->children[i]);
    }
}

static void defvar_gf(const char *var_name){
    return;
}



// Forward declarations
void processFunction(SyntaktikTreeNode *func_node, const char *class_name, Node *keywords, vStack *stack);
void processClass(SyntaktikTreeNode *class_node, Node *keywords, vStack *stack);

int generateIFJCode25(FILE *file)
{
    global_var_counter = 0;  // Reset počítadla dočasných proměnných
    global_label_counter = 0;  // Reset počítadla labelů
    int parse_result = parserRun(NULL, file);
    if (parse_result != 0) {
        return parse_result;  // Return parse error immediately
    }
    
    SyntaktikTreeNode *abs_tree = getSyntaxTreeRoot();
    Node *global = parserGetGlobalSymtable();
    
    // Perform semantic analysis after parsing
    int semantic_result = performSemanticAnalysis(abs_tree, global);
    if (semantic_result != 0) {
        freeSyntaktikTree(abs_tree);
        dispose(global);
        return semantic_result;  // Return semantic error
    }
    
    vStack stack;
    vStackInit(&stack);
    
    fprintf(stdout, ".IFJcode25\n");
    
    Node *keyword_tree = prepareKeywordsTree();
    if (keyword_tree == NULL)
    {
        return 1;
    }
    
    // Deklaruj globální proměnné
    declareGlobalVariables(global);
    
    // Deklaruj globální dočasné proměnné (x1 až x100) pro výrazy
    // Tyto proměnné jsou sdílené napříč všemi funkcemi
    // for(int i = 1; i <= 100; i++){
    //     fprintf(stdout, "DEFVAR GF@x%d\n", i);
    // }
    
    // Vygeneruj JUMP na main
    fprintf(stdout, "JUMP $$main\n");
    
    // Zpracuj všechny třídy a jejich funkce
    if(abs_tree != NULL){
        for(size_t i = 0; i < abs_tree->childrenCount; i++){
            SyntaktikTreeNode *child = abs_tree->children[i];
            if(child && child->token && child->token->type == TOKEN_IDENTIFIER){
                // Toto je třída
                processClass(child, keyword_tree, &stack);
            }
        }
    }
    
    // LABEL pro main
    fprintf(stdout, "LABEL $$main\n");
    
    // Zavolej Program.main()
    fprintf(stdout, "CREATEFRAME\n");
    fprintf(stdout, "CALL $Program$main$0\n");
    
    freeSyntaktikTree(abs_tree);
    disposeKeyWordTree(keyword_tree);
    dispose(global);
    disposeVStack(&stack);
    return 0;
}

ThreeAdressCode *absToThreeAdressCode(SyntaktikTreeNode *abs_tree, Node *variables,vStack *stack)
{
    ThreeAdressCode *code = initThreeAdressCode();
    ThreeAdressCode *pointer = NULL;
    code->operator = abs_tree->token;
    char *variable = vStackPop(stack);
    code->result = createToken(TOKEN_GENERATOR_VAR, variable);
    if(abs_tree->childrenCount > 0){
        if (isVariable(abs_tree->children[0]->token))
        {
            code->operandA = abs_tree->children[0]->token;
        }
        else
        {
            char *variable = createNewVariable(stack);
            code->operandA = createToken(TOKEN_GENERATOR_VAR, variable);
            insert(variable, &variables);
        }
        code->next = absToThreeAdressCode(abs_tree->children[0], variables,stack);
        pointer = code;
    }
    if(abs_tree->childrenCount > 1){
        if (isVariable(abs_tree->children[1]->token))
        {
            code->operandB = abs_tree->children[1]->token;
        }
        else
        {
            char *variable = createNewVariable(stack);
            code->operandB = createToken(TOKEN_GENERATOR_VAR, variable);
            insert(variable, &variables);
        }
        while (code->next != NULL)
        {
            code = code->next;
        }
        code->next = absToThreeAdressCode(abs_tree->children[1], variables,stack);
    }
    else{
        pointer = code;
    }
    return pointer;
}

void ThreeAdressCodeToIFJCode25(ThreeAdressCode *code, Node *keywords,Node *class_table)
{
    printThreeAdressCode(code);
    printf("---------\n");
    ThreeAdressCode *old_code = NULL;
    int number_of_parametrs = 0;
    while(code != NULL){
        if(code->operator != NULL){
            number_of_parametrs = printOperator(code->operator,keywords);
            if(number_of_parametrs == 1){
                printFormatedVariable(code->operandA);
                fprintf(stdout, "\n");
            }
            if(number_of_parametrs == 2){
                printFormatedVariable(code->operandA);
                printFormatedVariable(code->operandB);
                fprintf(stdout, "\n");
            }
            if(number_of_parametrs == 3){
                printFormatedVariable(code->result);
                printFormatedVariable(code->operandA);
                printFormatedVariable(code->operandB);
                fprintf(stdout, "\n");
            }
        }
        old_code = code;
        code = code->next;
        if(old_code->operandA != NULL){
            if(old_code->operandA->type == TOKEN_GENERATOR_VAR){
                freeToken(old_code->operandA);
            }
        }
        if(old_code->operandB != NULL){
            if(old_code->operandB->type == TOKEN_GENERATOR_VAR){
                freeToken(old_code->operandB);
            }
        }
        if(old_code->operator != NULL){
            if(old_code->operator->type == TOKEN_GENERATOR_VAR){
                freeToken(old_code->operator);
            }
        }
        if(old_code->result != NULL){
            if(old_code->result->type == TOKEN_GENERATOR_VAR){
                freeToken(old_code->result);
            }
        }
        free(old_code);
    }
}

bool isVariable(Token *token)
{
    if (token->type == TOKEN_IDENTIFIER)
    {
        return true;
    }
    if (token->type == TOKEN_GLOBAL_IDENTIFIER){
        return true;
    }
    if(token->type == TOKEN_GENERATOR_VAR){
        return true;
    }
    if (token->type == TOKEN_STRING){
        return true;
    }
    if (token->type == TOKEN_INT){
        return true;
    }
    if (token->type == TOKEN_FLOAT){
        return true;
    }
    if (token->type == TOKEN_KEYWORD && token->value && strcmp(token->value, "null") == 0){
        return true;  // null je literál
    }
    else
    {
        return false;
    }
}

Node *prepareKeywordsTree()
{
    char *array_of_IFJ25_keywords[39] = {"+", "-", "*", "/", "&", "|", "!", "=", "!=", "<", ">", "==", "<=", ">=", "class", "if", "else", "is", "null", "return", "var", "while", "Ifj", "static", "import", "for", "Num", "String", "Null", "Ifj.read_str", "Ifj.read_num", "Ifj.write", "Ifj.floor", "Ifj.str", "Ifj.length", "Ifj.substring", "Ifj.strcmp", "Ifj.ord", "Ifj.chr"};
    Token array_of_IFJCode25_keywords[39] = {{TOKEN_KEYWORD, "ADD"}, {TOKEN_KEYWORD, "SUB"}, {TOKEN_KEYWORD, "MUL"}, {TOKEN_DIV, "DIV"}, {TOKEN_KEYWORD, "AND"}, {TOKEN_KEYWORD, "OR"}, {TOKEN_KEYWORD, "NOT"}, {TOKEN_KEYWORD, "MOVE"}, {TOKEN_NEQ, "EQ"}, {TOKEN_LT, "LT"}, {TOKEN_GT, "GT"}, {TOKEN_EQ, "EQ"}, {TOKEN_LTE, "LT"}, {TOKEN_GTE, "GT"}, {TOKEN_KEYWORD, " "}, {TOKEN_UNKNOWN, " "}, {TOKEN_UNKNOWN, " "}, {TOKEN_UNKNOWN, "TYPE"}, {TOKEN_UNKNOWN, "nil"}, {TOKEN_KEYWORD, "RETURN"}, {TOKEN_KEYWORD, "DEFVAR"}, {TOKEN_UNKNOWN, " "}, {TOKEN_KEYWORD, " "}, {TOKEN_KEYWORD, " "}, {TOKEN_KEYWORD, " "}, {TOKEN_KEYWORD, " "}, {TOKEN_KEYWORD, "DEFVAR"}, {TOKEN_KEYWORD, " "}, {TOKEN_UNKNOWN, "nil"}, {TOKEN_UNKNOWN, "READ"}, {TOKEN_UNKNOWN, "READ"}, {TOKEN_KEYWORD, "WRITE"}, {TOKEN_KEYWORD, "FLOAT2INT"}, {TOKEN_KEYWORD, "INT2STR"}, {TOKEN_KEYWORD, "STRLEN"}, {TOKEN_UNKNOWN, " "}, {TOKEN_UNKNOWN, " "}, {TOKEN_KEYWORD, "STRI2INT"}, {TOKEN_KEYWORD, " "}};
    int parametrs[39] =                     {          3,                   3,                     3,                  3,                 3,                     3,                   2,                    2,                        3,                 3,              3,                 3,               3,              3,                3,                   3,                  3,                 2,                       3,                  0,                            1,                  3,                  1,                 1,                  0,                 3,                       1,                  0,                     0,                   2,                    2,                        1,                         2,                          2,                        2,                       3,                    3,                     3,                         2};
    Node *tree = init(array_of_IFJ25_keywords[0]);
    for (int i = 1; i < 39; i++)
    {
        insert(array_of_IFJ25_keywords[i], &tree);
    }
    for (int i = 0; i < 39; i++)
    {
        Node *pointer = search(tree, array_of_IFJ25_keywords[i]);

        if (pointer != NULL)
        {
            Data *data = malloc(sizeof(Data));
            if (data == NULL)
            {
                dispose(tree);
                return NULL;
            }
            data->token = createToken(array_of_IFJCode25_keywords[i].type, array_of_IFJCode25_keywords[i].value);
            data->type = TYPE_KEYWORD;
            data->varType = parametrs[i];
            pointer->data = data;
        }
    }
    return tree;
}

char *createNewVariable(vStack *stack)
{
    (void)stack;  // Nepoužíváme stack, ale parametr musí zůstat kvůli kompatibilitě
    char *new_variable = malloc(sizeof(char) * MAX_LEN_OF_VARIABLE);
    if (new_variable == NULL)
    {
        return NULL;
    }
    
    global_var_counter++;
    sprintf(new_variable, "x%d", global_var_counter);
    return new_variable;
}

void disposeKeyWordTree(Node* root){
    if(root != NULL){
        disposeKeyWordTree(root->left);
        disposeKeyWordTree(root->right);
        if (root->key) free(root->key);
        free(root);
    }
}

void printFormatedVariable(Token* token){
    if(token != NULL){
        if(token->type == TOKEN_STRING){
            char* value = malloc(sizeof(char)*100);
            strcpy(value,token->value);
            for(size_t i = 0;i < strlen(value);i++){
                value[i] = value[i+1];
            }
            value[strlen(value) - 1] = '\0';
            int i = 0;
            while(value[i] != '\0'){
                if((value[i] >= 0 && value[i] <= 32) || value[i] == 35 || value[i] == 92){
                    int j = strlen(value);
                    while(j > i){
                        value[j+3] = value[j];
                        j--;
                    }
                    value[i+2] = (value[i]/10) + '0';
                    value[i+3] = (value[i]%10) + '0';
                    value[i] = 92; // zpetne lomitko
                    value[i+1] = '0';
                }
                i++;
            }
            fprintf(stdout,"string@%s ",value);
            free(value);
        }
        if(token->type == TOKEN_INT){
            fprintf(stdout,"int@%s ",token->value);
        }
        if(token->type == TOKEN_FLOAT){
            // Konverze float na hex formát pro IFJCode25
            double val = atof(token->value);
            fprintf(stdout,"float@%a ", val);
        }
        if(token->type == TOKEN_IDENTIFIER){
            fprintf(stdout,"LF@%s ",token->value);
        }
        if(token->type == TOKEN_KEYWORD && token->value && strcmp(token->value, "null") == 0){
            fprintf(stdout,"nil@nil ");
        }
        if(token->type == TOKEN_GLOBAL_IDENTIFIER){
            fprintf(stdout,"GF@%s ",token->value);
        }
        if(token->type == TOKEN_GENERATOR_VAR){
            // Check if it is a literal (contains @)
            if(strchr(token->value, '@') != NULL){
                fprintf(stdout,"%s ",token->value);
            } else {
                fprintf(stdout,"LF@%s ",token->value);
            }
        }
    }
}

int printOperator(Token* token,Node *keywords){
    Node* pointer = NULL;
    if(token->value != NULL){
        pointer = search(keywords, token->value);
    }
    else{
        if(token->type == TOKEN_ASSIGN){
            pointer = search(keywords,"=");
        }
        if(token->type == TOKEN_PLUS){
            pointer = search(keywords,"+");
        }
        if(token->type == TOKEN_MINUS){
            pointer = search(keywords,"-");
        }
        if(token->type == TOKEN_MUL){
            pointer = search(keywords,"*");
        }
        if(token->type == TOKEN_DIV){
            pointer = search(keywords,"/");
        }
        if(token->type == TOKEN_LT){
            pointer = search(keywords,"<");
        }
        if(token->type == TOKEN_GT){
            pointer = search(keywords,">");
        }
        if(token->type == TOKEN_EQ){
            pointer = search(keywords,"==");
        }
        if(token->type == TOKEN_NEQ){
            pointer = search(keywords,"!=");
        }
        if(token->type == TOKEN_LTE){
            pointer = search(keywords,"<=");
        }
        if(token->type == TOKEN_GTE){
            pointer = search(keywords,">=");
        }
    }

    if(pointer != NULL){
        fprintf(stdout,"%s ",pointer->data->token->value);
        return pointer->data->varType;
    }
    else{
        return 0;
    }
}

ThreeAdressCode* initThreeAdressCode(){
    ThreeAdressCode *code = malloc(sizeof(ThreeAdressCode));
    if (code == NULL)
    {
        return NULL;
    }
    code->next = NULL;
    code->operandA = NULL;
    code->operandB = NULL;
    code->operator = NULL;
    code->result = NULL;
    return code;
}

void printThreeAdressCode(ThreeAdressCode *code){
    while(code != NULL){
        if(code->operator != NULL){
            if(code->operator->value != NULL){
                printf("op %s\n",code->operator->value);
            }
        }
        if(code->operandA != NULL){
            if(code->operandA->value != NULL){
                printf("A %s\n",code->operandA->value);
            }
        }
        if(code->operandB != NULL){
            if(code->operandB->value != NULL){
                printf("B %s\n",code->operandB->value);
            }
        }
        if(code->result != NULL){
            if(code->result->value != NULL){
                printf("r %s\n",code->result->value);
            }
        }
        code = code->next;
    }
}

/**
 * Process leaf nodes in the syntax tree and push their values onto the stack.
 * @param node The syntax tree node to process.
 * @param stack The variable stack to push values onto.
 */
static void processLeafNodes(SyntaktikTreeNode *node, vStack *stack){
    if(node->token->type == TOKEN_IDENTIFIER){
        vStackPush(stack, node->token->value);
    }
    else if(node->token->type == TOKEN_GLOBAL_IDENTIFIER){
        vStackPush(stack, node->token->value);
    }
    else if(node->token->type == TOKEN_INT){
        char val[256];
        snprintf(val, 256, "int@%s", node->token->value);
        vStackPush(stack, val);
    }
    else if(node->token->type == TOKEN_FLOAT){
        char val[256];
        double d = atof(node->token->value);
        snprintf(val, 256, "float@%a", d);
        vStackPush(stack, val);
    }
    else if(node->token->type == TOKEN_STRING){
        char escaped[256];
        char *raw = node->token->value;
        int start = 0;
        int end = strlen(raw);
        if(end > 0 && raw[0] == '"') start = 1;
        if(end > 0 && raw[end-1] == '"') end--;

        int j = 0;
        strcpy(escaped, "string@");
        j = 7;
        for(int i = start; i < end && j < 250; i++){
            if(raw[i] <= 32 || raw[i] == 35 || raw[i] == 92){ 
                j += sprintf(escaped + j, "\\%03d", raw[i]);
            } else {
                escaped[j++] = raw[i];
            }
        }
        escaped[j] = '\0';
        vStackPush(stack, escaped);
    }
    else if(node->token->type == TOKEN_KEYWORD && strcmp(node->token->value, "null") == 0){
        vStackPush(stack, "nil");
    }
    return;
}


/**
 * Check if a node represents an expression.
 * @param node The syntax tree node to check.
 * @return true if the node is an expression, false otherwise.
 */
bool isExpression(SyntaktikTreeNode *node){
    if(!node || !node->token) return false;
    if(node->token->value){
        if(strcmp(node->token->value, "if") == 0) return false;
        if(strcmp(node->token->value, "while") == 0) return false;
        if(strcmp(node->token->value, "return") == 0) return false;
        if(strcmp(node->token->value, "var") == 0) return false;
        if(strcmp(node->token->value, "class") == 0) return false;
        if(strcmp(node->token->value, "import") == 0) return false;
    }
    // Block is not an expression in our context (it's a container)
    if(node->token->type == TOKEN_CURLY_BRACKET_START) return false;
    
    return true;
}


/**
 * Process if statement nodes.
 * @param abs_tree The abstract syntax tree node representing the if statement.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 */
static void ifBeforePostorder(SyntaktikTreeNode *abs_tree, Node *keywords, vStack *stack){
        // IF has 2 or 3 children: [condition, then-block, (else-block)]
    if(abs_tree->childrenCount >= 2){
        // Generate unique labels
        int label_id = global_label_counter++;
        char else_label[50], end_label[50];
        char bool_check_label[50], then_label[50];
        snprintf(else_label, sizeof(else_label), "$else%d", label_id);
        snprintf(end_label, sizeof(end_label), "$endif%d", label_id);
        snprintf(bool_check_label, sizeof(bool_check_label), "$bool_check%d", label_id);
        snprintf(then_label, sizeof(then_label), "$then%d", label_id);
        
        // Evaluate condition (first child)
        absToIFJCode25(abs_tree->children[0], keywords, stack);
        char *cond_result = vStackPop(stack);
        
        // 1. Check for nil
        fprintf(stdout,"JUMPIFEQ %s LF@%s nil@nil\n", (abs_tree->childrenCount == 3 ? else_label : end_label), cond_result);
        
        // 2. Check type
        char *type_var = createNewVariable(stack);
        defvar_gf(type_var);
        fprintf(stdout, "TYPE LF@%s LF@%s\n", type_var, cond_result);
        fprintf(stdout, "JUMPIFEQ %s LF@%s string@bool\n", bool_check_label, type_var);
        
        // 3. Not bool -> Truthy -> Jump to THEN
        fprintf(stdout, "JUMP %s\n", then_label);
        
        // 4. Is bool -> Check for false
        fprintf(stdout, "LABEL %s\n", bool_check_label);
        fprintf(stdout,"JUMPIFEQ %s LF@%s bool@false\n", (abs_tree->childrenCount == 3 ? else_label : end_label), cond_result);
        
        fprintf(stdout, "LABEL %s\n", then_label);
        
        // Then block (second child)
        absToIFJCode25(abs_tree->children[1], keywords, stack);
        
        if(abs_tree->childrenCount == 3){
            // Jump over else block
            fprintf(stdout,"JUMP %s\n", end_label);
            fprintf(stdout,"LABEL %s\n", else_label);
            // Else block (third child)
            absToIFJCode25(abs_tree->children[2], keywords, stack);
        }
        
        fprintf(stdout,"LABEL %s\n", end_label);
    }
    return;
}


/**
 * Process while loop nodes.
 * @param abs_tree The abstract syntax tree node representing the while loop.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 */
static void whileBeforePostorder(SyntaktikTreeNode *abs_tree, Node *keywords, vStack *stack){
    // WHILE has 2 children: [condition, body]
    if(abs_tree->childrenCount >= 2){
        int label_id = global_label_counter++;
        char start_label[50], end_label[50];
        char bool_check_label[50], body_label[50];
        snprintf(start_label, sizeof(start_label), "$while%d", label_id);
        snprintf(end_label, sizeof(end_label), "$endwhile%d", label_id);
        snprintf(bool_check_label, sizeof(bool_check_label), "$w_bool_check%d", label_id);
        snprintf(body_label, sizeof(body_label), "$w_body%d", label_id);
        
        // Label for the start of the loop (where the condition is evaluated)
        fprintf(stdout,"LABEL %s\n", start_label);
        
        // Evaluate condition
        absToIFJCode25(abs_tree->children[0], keywords, stack);
        char *cond_result = vStackPop(stack);
        
        // 1. Check for nil
        fprintf(stdout,"JUMPIFEQ %s LF@%s nil@nil\n", end_label, cond_result);
        
        // 2. Check type
        char *type_var = createNewVariable(stack);
        defvar_gf(type_var);
        fprintf(stdout, "TYPE LF@%s LF@%s\n", type_var, cond_result);
        fprintf(stdout, "JUMPIFEQ %s LF@%s string@bool\n", bool_check_label, type_var);
        
        // 3. Not bool -> Truthy -> Jump to BODY
        fprintf(stdout, "JUMP %s\n", body_label);
        
        // 4. Is bool -> Check for false
        fprintf(stdout, "LABEL %s\n", bool_check_label);
        fprintf(stdout,"JUMPIFEQ %s LF@%s bool@false\n", end_label, cond_result);
        
        fprintf(stdout, "LABEL %s\n", body_label);
        
        // Tělo cyklu
        absToIFJCode25(abs_tree->children[1], keywords, stack);
        
        // Jump back to the start (where the condition is re-evaluated)
        fprintf(stdout,"JUMP %s\n", start_label);
        fprintf(stdout,"LABEL %s\n", end_label);
    }
    return;
}


/**
 * Process return statement nodes.
 * @param abs_tree The abstract syntax tree node representing the return statement.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 */
static void returnBeforePostorder(SyntaktikTreeNode *abs_tree, Node *keywords, vStack *stack){
    // Return value is stored in LF@%retval (declared in processFunction)
    if(abs_tree->childrenCount > 0){
        if(abs_tree->children[0]->token && abs_tree->children[0]->token->value &&
            strcmp(abs_tree->children[0]->token->value, "null") == 0){
            fprintf(stdout, "MOVE LF@%%retval nil@nil\n");
        } else if(abs_tree->children[0]->childrenCount == 0 && isVariable(abs_tree->children[0]->token)){
            fprintf(stdout, "MOVE LF@%%retval ");
            printFormatedVariable(abs_tree->children[0]->token);
            fprintf(stdout, "\n");
        } else {
            absToIFJCode25(abs_tree->children[0], keywords, stack);
            char *return_value = vStackPop(stack);
            
            if(return_value){
                if(strcmp(return_value, "nil") == 0){
                    fprintf(stdout, "MOVE LF@%%retval nil@nil\n");
                } else {
                    fprintf(stdout, "MOVE LF@%%retval LF@%s\n", return_value);
                }
            } else {
                fprintf(stdout, "MOVE LF@%%retval nil@nil\n");
            }
        }
    } else {
        fprintf(stdout, "MOVE LF@%%retval nil@nil\n");
    }
    fprintf(stdout,"POPFRAME\n");
    fprintf(stdout,"RETURN\n");
}


/**
 * Process variable declaration nodes.
 * @param abs_tree The abstract syntax tree node representing the variable declaration.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 */
static void processVarNode(SyntaktikTreeNode *abs_tree, Node *keywords, vStack *stack){
    if(abs_tree->childrenCount > 0 && abs_tree->children[0]->token != NULL){
        const char *var_name = abs_tree->children[0]->token->value;
        fprintf(stdout,"MOVE LF@%s nil@nil\n", var_name);
        
        if(abs_tree->childrenCount > 1){
            absToIFJCode25(abs_tree->children[1], keywords, stack);
            char *init_val = vStackPop(stack);
            
            if(init_val){
                if(strcmp(init_val, "nil") == 0){
                    fprintf(stdout,"MOVE LF@%s nil@nil\n", var_name);
                } else {
                    fprintf(stdout,"MOVE LF@%s LF@%s\n", var_name, init_val);
                }
            }
        }
    }
    return;
}


/**
 * Process the 'is' operation, checking if an object is of a specified type.
 * @param abs_tree The abstract syntax tree node representing the 'is' operation.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 * @param result_var The variable to store the result.
 */
static void isOpProcessing(SyntaktikTreeNode *abs_tree, Node *keywords, vStack *stack, char *result_var){
    // Evaluate left operand (child 0)
    absToIFJCode25(abs_tree->children[0], keywords, stack);
    char *obj_var = vStackPop(stack);
    
    // Create result
    result_var = createNewVariable(stack);
    defvar_gf(result_var);
    
    // Determine expected type from right operand
    const char *expected_type = NULL;
    if(abs_tree->children[1]->token && abs_tree->children[1]->token->value){
        expected_type = abs_tree->children[1]->token->value;
    }
    
    if(expected_type && obj_var){
        // Get type of object
        char *type_var = createNewVariable(stack);
        defvar_gf(type_var);
        
        // Format obj_var
        char formatted_obj[300];
        if(strcmp(obj_var, "nil") == 0){
            strcpy(formatted_obj, "nil@nil");
        } else if(strchr(obj_var, '@') != NULL){
            strcpy(formatted_obj, obj_var);
        } else {
            // Assume LF variable
            snprintf(formatted_obj, 300, "LF@%s", obj_var);
        }
        
        fprintf(stdout, "TYPE LF@%s %s\n", type_var, formatted_obj);
        
        int label_id = global_label_counter++;
        char true_label[50], end_label[50];
        snprintf(true_label, sizeof(true_label), "$is_true%d", label_id);
        snprintf(end_label, sizeof(end_label), "$is_end%d", label_id);
        
        if(strcmp(expected_type, "Null") == 0){
            fprintf(stdout, "JUMPIFEQ %s LF@%s string@nil\n", true_label, type_var);
        } else if(strcmp(expected_type, "Num") == 0){
            fprintf(stdout, "JUMPIFEQ %s LF@%s string@int\n", true_label, type_var);
            fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", true_label, type_var);
        } else if(strcmp(expected_type, "String") == 0){
            fprintf(stdout, "JUMPIFEQ %s LF@%s string@string\n", true_label, type_var);
        } else if(strcmp(expected_type, "bool") == 0){ 
            fprintf(stdout, "JUMPIFEQ %s LF@%s string@bool\n", true_label, type_var);
        }
        
        // False case
        fprintf(stdout, "MOVE LF@%s bool@false\n", result_var);
        fprintf(stdout, "JUMP %s\n", end_label);
        
        // True case
        fprintf(stdout, "LABEL %s\n", true_label);
        fprintf(stdout, "MOVE LF@%s bool@true\n", result_var);
        
        fprintf(stdout, "LABEL %s\n", end_label);
    } else {
        // Neznámý typ nebo chyba - default false
        fprintf(stdout, "MOVE LF@%s bool@false\n", result_var);
    }
    
    vStackPush(stack, result_var);
    return;
}


/**
 * Process the 'floor' operation, converting a float to an integer by flooring it.
 * @param abs_tree The abstract syntax tree node representing the 'floor' operation.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 * @param result_var The variable to store the result.
 */
static void processFloor(SyntaktikTreeNode *abs_tree, Node *keywords, vStack *stack, char *result_var){
    // Evaluate argument
    absToIFJCode25(abs_tree->children[0], keywords, stack);
    char *arg_result = vStackPop(stack);
            
    // Create result
    result_var = createNewVariable(stack);
    defvar_gf(result_var);
            
    // Check type using TYPE instruction
    // If float, convert to int; if already int, just copy
    char *type_var = createNewVariable(stack);
    defvar_gf(type_var);
            
    int label_id = global_label_counter++;
    char float_label[50], end_label[50];
    snprintf(float_label, sizeof(float_label), "$floor_float%d", label_id);
    snprintf(end_label, sizeof(end_label), "$floor_end%d", label_id);
            
    // Determine type of argumentq
    fprintf(stdout, "TYPE LF@%s ", type_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
            
    // If float, jump to conversion
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type_var);
            
    // Is int - just copy
    fprintf(stdout, "MOVE LF@%s ", result_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP %s\n", end_label);
            
    // Is float - convert
    fprintf(stdout, "LABEL %s\n", float_label);
    fprintf(stdout, "FLOAT2INT LF@%s ", result_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
            
    fprintf(stdout, "LABEL %s\n", end_label);
            
    vStackPush(stack, result_var);
}


/**
 * Process the 'str' operation, converting a value to its string representation.
 * @param abs_tree The abstract syntax tree node representing the 'str' operation.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 * @param result_var The variable to store the result.
 */
static void processStr(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack, char* result_var){
    // Evaluate argument
    absToIFJCode25(abs_tree->children[0], keywords, stack);
    char *arg_result = vStackPop(stack);
            
    // Create result
    result_var = createNewVariable(stack);
    defvar_gf(result_var);
            
    // Check type using TYPE instruction
    char *type_var = createNewVariable(stack);
    defvar_gf(type_var);
            
    int label_id = global_label_counter++;
    char end_label[50];
    char string_label[50];
    char float_label[50];
            
    snprintf(end_label, sizeof(end_label), "$str_end%d", label_id);
    snprintf(string_label, sizeof(string_label), "$str_string%d", label_id);
    snprintf(float_label, sizeof(float_label), "$str_float%d", label_id);
            
    // Determine type of argument
    fprintf(stdout, "TYPE LF@%s ", type_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
            
    // If string, jump over conversion
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@string\n", string_label, type_var);
            
    // If float, jump to float conversion
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type_var);
            
    // Is int - convert
    fprintf(stdout, "INT2STR LF@%s ", result_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP %s\n", end_label);
            
    // Is float - convert
    fprintf(stdout, "LABEL %s\n", float_label);
    fprintf(stdout, "FLOAT2STR LF@%s ", result_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP %s\n", end_label);
            
    // Is string - just copy
    fprintf(stdout, "LABEL %s\n", string_label);
    fprintf(stdout, "MOVE LF@%s ", result_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
            
    fprintf(stdout, "LABEL %s\n", end_label);
            
    vStackPush(stack, result_var);
}


/**
 * Process the 'strcmp' operation, comparing two strings.
 * @param abs_tree The abstract syntax tree node representing the 'strcmp' operation.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 * @param result_var The variable to store the result.
 */
static void processStrcmp(SyntaktikTreeNode *abs_tree, Node *keywords, vStack *stack, char* result_var){
    // Evaluate arguments
    absToIFJCode25(abs_tree->children[0], keywords, stack);
    absToIFJCode25(abs_tree->children[1], keywords, stack);
    char *arg_result1 = vStackPop(stack);
    char *arg_result2 = vStackPop(stack);
    
    int label_id = global_label_counter++;
    result_var = createNewVariable(stack);
    char *result1 = createNewVariable(stack);
    char *result2 = createNewVariable(stack);
    char *result3 = createNewVariable(stack);
    defvar_gf(result_var);
    defvar_gf(result1);
    defvar_gf(result2);
    defvar_gf(result3);

    fprintf(stdout, "EQ LF@%s ", result1);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result1);
    }
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result2);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "LT LF@%s ", result2);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result1);
    }
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result2);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "GT LF@%s ", result3);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result1);
    }
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result2);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $strcmp_eq%d LF@%s bool@true\n",label_id ,result1);
    fprintf(stdout, "JUMPIFEQ $strcmp_lt%d LF@%s bool@true\n",label_id , result2);
    fprintf(stdout, "JUMPIFEQ $strcmp_gt%d LF@%s bool@true\n",label_id , result3);
    fprintf(stdout, "LABEL $strcmp_eq%d\n",label_id);
    fprintf(stdout, "MOVE LF@%s int@0\n", result_var);
    fprintf(stdout, "JUMP $strcmp_end%d\n",label_id);
    fprintf(stdout, "LABEL $strcmp_lt%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s int@-1\n", result_var);
    fprintf(stdout, "JUMP $strcmp_end%d\n",label_id);
    fprintf(stdout, "LABEL $strcmp_gt%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s int@1\n", result_var);
    fprintf(stdout, "LABEL $strcmp_end%d\n", label_id);
    vStackPush(stack, result_var);
}


/**
 * Process the 'substring' operation, extracting a substring from a given string.
 * @param abs_tree The abstract syntax tree node representing the 'substring' operation.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 * @param result_var The variable to store the result.
 */
static void processSubstring(SyntaktikTreeNode *abs_tree, Node *keywords, vStack *stack, char* result_var){
    absToIFJCode25(abs_tree->children[0], keywords, stack);
    absToIFJCode25(abs_tree->children[1], keywords, stack);
    absToIFJCode25(abs_tree->children[2], keywords, stack);
    char *arg_result1 = vStackPop(stack);
    char *arg_result2 = vStackPop(stack);
    char *arg_result3 = vStackPop(stack);
    
    int label_id = global_label_counter++;
    char* part = createNewVariable(stack);
    result_var = createNewVariable(stack);        
    char* start = createNewVariable(stack);
    char* end = createNewVariable(stack);
    char *type_var1 = createNewVariable(stack);
    char *type_var2 = createNewVariable(stack);
    defvar_gf(part);
    defvar_gf(result_var);
    defvar_gf(start);
    defvar_gf(end);
    defvar_gf(type_var1);
    defvar_gf(type_var2);

    fprintf(stdout, "TYPE LF@%s ", type_var1);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result2);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "TYPE LF@%s ", type_var2);
    if(isVariable(abs_tree->children[2]->token)){
        printFormatedVariable(abs_tree->children[2]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result3);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "JUMPIFEQ $is_float%d LF@%s string@float\n",label_id, type_var1);

    fprintf(stdout, "MOVE LF@%s ", start);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result2);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "LABEL $check_second%d\n", label_id);

    fprintf(stdout, "JUMPIFEQ $is_float2%d LF@%s string@float\n",label_id, type_var2);

    fprintf(stdout, "MOVE LF@%s ", end);
    if(isVariable(abs_tree->children[2]->token)){
        printFormatedVariable(abs_tree->children[2]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result3);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "JUMP $start_substr%d\n", label_id);

    fprintf(stdout, "LABEL $is_float%d\n", label_id);
    fprintf(stdout, "FLOAT2INT LF@%s ", start);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result2);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP $check_second%d\n", label_id);

    fprintf(stdout, "LABEL $is_float2%d\n", label_id);
    fprintf(stdout, "FLOAT2INT LF@%s ", end);
    if(isVariable(abs_tree->children[2]->token)){
        printFormatedVariable(abs_tree->children[2]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result3);
    }
    fprintf(stdout, "\n");

    fprintf(stdout, "LABEL $start_substr%d\n", label_id);
    fprintf(stdout, "GETCHAR LF@%s ", result_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result1);
    }
    fprintf(stdout, " LF@%s\n", start);

    fprintf(stdout, "SUB LF@%s LF@%s int@1\n", end, end);
        
    fprintf(stdout, "LABEL $for_substr%d\n", label_id);
    fprintf(stdout, "ADD LF@%s LF@%s int@1\n", start, start);

    fprintf(stdout, "GETCHAR LF@%s ", part);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result1);
    }
    fprintf(stdout, " LF@%s\n", start);
    fprintf(stdout, "CONCAT LF@%s LF@%s LF@%s\n", result_var, result_var, part);
    fprintf(stdout, "JUMPIFNEQ $for_substr%d LF@%s LF@%s\n", label_id, start, end);
        
    vStackPush(stack, result_var);
}


/**
 * Process the 'chr' operation, converting an integer or float to its corresponding character.
 * @param abs_tree The abstract syntax tree node representing the 'chr' operation.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 * @param result_var The variable to store the result of the 'chr' operation.
 */
static void processChr(SyntaktikTreeNode * abs_tree, Node *keywords, vStack *stack, char* result_var){
    // Evaluate argument
    absToIFJCode25(abs_tree->children[0], keywords, stack);
    char *arg_result = vStackPop(stack);

    int label_id = global_label_counter++;
    char* helping_var = createNewVariable(stack);
    result_var = createNewVariable(stack);
    defvar_gf(helping_var);
    defvar_gf(result_var);
            
    fprintf(stdout, "TYPE LF@%s ", result_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $chr_float%d LF@%s string@float\n", label_id, result_var);
    fprintf(stdout,"JUMP $chr_int%d\n", label_id);

    fprintf(stdout, "LABEL $chr_float%d\n", label_id);
    fprintf(stdout, "FLOAT2INT LF@%s ", helping_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "INT2CHAR LF@%s LF@%s\n", result_var, helping_var);
    fprintf(stdout,"JUMP $end%d\n", label_id);
    fprintf(stdout, "LABEL $chr_int%d\n", label_id);
    fprintf(stdout, "INT2CHAR LF@%s ", result_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "LABEL $end%d\n", label_id);
    vStackPush(stack, result_var);
}


/**
 * Process the 'ord' operation, converting string character to its integer ASCII value.
 * @param abs_tree The abstract syntax tree node representing the 'ord' operation.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 * @param result_var The variable to store the result of the 'ord' operation.
 */
static void processOrd(SyntaktikTreeNode *abs_tree, Node *keywords, vStack *stack, char* result_var){
    absToIFJCode25(abs_tree->children[0], keywords, stack);
    absToIFJCode25(abs_tree->children[1], keywords, stack);
    char *arg_result1 = vStackPop(stack);
    char *arg_result2 = vStackPop(stack);
    
    int label_id = global_label_counter++;
    char* type_var = createNewVariable(stack);
    char* helping_var = createNewVariable(stack);    
    result_var = createNewVariable(stack);
    defvar_gf(type_var);        
    defvar_gf(helping_var);
    defvar_gf(result_var);
    fprintf(stdout, "TYPE LF@%s ", type_var);
    printFormatedVariable(abs_tree->children[1]->token);
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMPIFEQ $is_float%d LF@%s string@float\n",label_id, type_var);
    fprintf(stdout, "MOVE LF@%s ", helping_var);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result2);
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP $start_ord%d\n", label_id);
    fprintf(stdout, "LABEL $is_float%d\n",label_id);
    fprintf(stdout, "FLOAT2INT LF@%s ", helping_var);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result2);
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "LABEL $start_ord%d\n", label_id);
    fprintf(stdout, "STRI2INT LF@%s ", result_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        fprintf(stdout, "LF@%s ", arg_result1);
    }
    fprintf(stdout, " LF@%s\n", helping_var);
    vStackPush(stack, result_var);
}


/**
 * Process division operation, handling int and float types.
 * @param abs_tree The abstract syntax tree node representing the division operation.
 * @param keywords The list of keywords.
 * @param stack The variable stack for managing temporary variables.
 * @param result_var The variable to store the result of the division.
 * @param child_results The results of the child nodes (operands).
 */
static void processDiv(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack, char* result_var, char *child_results[100]){
    // Create variables
    char *float_op1 = createNewVariable(stack);
    char *float_op2 = createNewVariable(stack);
    char *type_var1 = createNewVariable(stack);
    char *type_var2 = createNewVariable(stack);
    char *is_int = createNewVariable(stack);
    defvar_gf(float_op1);
    defvar_gf(float_op2);
    defvar_gf(type_var1);
    defvar_gf(type_var2);
    defvar_gf(is_int);
      
    int label_id = global_label_counter++;
       
    // Process first operand
    fprintf(stdout, "TYPE LF@%s ", type_var1);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
    }
    fprintf(stdout, "\n");
        
    // If int, convert to float; otherwise copy
    fprintf(stdout, "JUMPIFEQ $div_is_float1_%d LF@%s string@float\n", label_id, type_var1);
    fprintf(stdout, "INT2FLOAT LF@%s ", float_op1);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP $div_op1_done_%d\n", label_id);
    fprintf(stdout, "LABEL $div_is_float1_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s ", float_op1);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "LABEL $div_op1_done_%d\n", label_id);
        
    // Process second operand
    fprintf(stdout, "TYPE LF@%s ", type_var2);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
    }
    fprintf(stdout, "\n");
        
    // If int, convert to float; otherwise copy
    fprintf(stdout, "JUMPIFEQ $div_is_float2_%d LF@%s string@float\n", label_id, type_var2);
    fprintf(stdout, "INT2FLOAT LF@%s ", float_op2);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP $div_op2_done_%d\n", label_id);
    fprintf(stdout, "LABEL $div_is_float2_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s ", float_op2);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "LABEL $div_op2_done_%d\n", label_id);
        
    // Perform float division
    fprintf(stdout, "DIV LF@%s LF@%s LF@%s\n", result_var, float_op1, float_op2);
    fprintf(stdout, "ISINT LF@%s LF@%s\n", is_int, result_var);
    fprintf(stdout, "JUMPIFEQ $skip%d LF@%s bool@false\n", label_id, is_int);
    fprintf(stdout, "FLOAT2INT LF@%s LF@%s\n", result_var, result_var);
    fprintf(stdout, "LABEL $skip%d\n", label_id);
    vStackPush(stack, result_var);
}

static void processAritmetik(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack, char* result_var, char *child_results[100]){
    char *op1_var = createNewVariable(stack);
    char *op2_var = createNewVariable(stack);
    char *type1 = createNewVariable(stack);
    char *type2 = createNewVariable(stack);
    char *type3 = createNewVariable(stack);
    char *type4 = createNewVariable(stack);
    defvar_gf(op1_var);
    defvar_gf(op2_var);
    defvar_gf(type1);
    defvar_gf(type2);
    defvar_gf(type3);
    defvar_gf(type4);
        
    // Load operands
    fprintf(stdout, "MOVE LF@%s ", op1_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    }
    else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
    }
    fprintf(stdout, "\n");
       
    fprintf(stdout, "MOVE LF@%s ", op2_var);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    }
    else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
    }
    fprintf(stdout, "\n");
        
    // Get types
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1_var);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2_var);
        
    int label_id = global_label_counter++;
    char float_label[50], end_label[50], string_label[50];
    snprintf(float_label, sizeof(float_label), "$op_float%d", label_id);
    snprintf(end_label, sizeof(end_label), "$op_end%d", label_id);
    snprintf(string_label, sizeof(string_label), "$op_string%d", label_id);
        
    // Check for string (only for PLUS)
    if(abs_tree->token->type == TOKEN_PLUS){
        fprintf(stdout, "JUMPIFEQ %s LF@%s string@string\n", string_label, type1);
        fprintf(stdout, "JUMPIFEQ %s LF@%s string@string\n", string_label, type2);
    }
        
    // Check for string (for MUL)
    if(abs_tree->token->type == TOKEN_MUL){
        fprintf(stdout, "JUMPIFEQ $mul_string_%d LF@%s string@string\n", label_id, type1);
    }
        
    // Check for float
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type1);
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type2);
        
    // Both are int
    if(abs_tree->token->type == TOKEN_PLUS) fprintf(stdout, "ADD LF@%s LF@%s LF@%s\n", result_var, op1_var, op2_var);
    if(abs_tree->token->type == TOKEN_MINUS) fprintf(stdout, "SUB LF@%s LF@%s LF@%s\n", result_var, op1_var, op2_var);
    if(abs_tree->token->type == TOKEN_MUL) fprintf(stdout, "MUL LF@%s LF@%s LF@%s\n", result_var, op1_var, op2_var);
    fprintf(stdout, "JUMP %s\n", end_label);
        
    // Float handling
    fprintf(stdout, "LABEL %s\n", float_label);
    // Convert op1 if needed
    char *f_op1 = createNewVariable(stack);
    defvar_gf(f_op1);
    fprintf(stdout, "JUMPIFEQ $op_conv1_%d LF@%s string@float\n", label_id, type1);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "JUMP $op_conv1_end_%d\n", label_id);
    fprintf(stdout, "LABEL $op_conv1_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "LABEL $op_conv1_end_%d\n", label_id);
       
    // Convert op2 if needed
    char *f_op2 = createNewVariable(stack);
    defvar_gf(f_op2);
    fprintf(stdout, "JUMPIFEQ $op_conv2_%d LF@%s string@float\n", label_id, type2);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "JUMP $op_conv2_end_%d\n", label_id);
    fprintf(stdout, "LABEL $op_conv2_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "LABEL $op_conv2_end_%d\n", label_id);
        
    if(abs_tree->token->type == TOKEN_PLUS) fprintf(stdout, "ADD LF@%s LF@%s LF@%s\n", result_var, f_op1, f_op2);
    if(abs_tree->token->type == TOKEN_MINUS) fprintf(stdout, "SUB LF@%s LF@%s LF@%s\n", result_var, f_op1, f_op2);
    if(abs_tree->token->type == TOKEN_MUL) fprintf(stdout, "MUL LF@%s LF@%s LF@%s\n", result_var, f_op1, f_op2);
    fprintf(stdout, "JUMP %s\n", end_label);
        
    if(abs_tree->token->type == TOKEN_PLUS){
        fprintf(stdout, "LABEL %s\n", string_label);
            
        // Convert op1
        char *s_op1 = createNewVariable(stack);
        defvar_gf(s_op1);
        fprintf(stdout, "JUMPIFEQ $op_sconv1_%d LF@%s string@string\n", label_id, type1);
        fprintf(stdout, "JUMPIFEQ $op_sconv1_float_%d LF@%s string@float\n", label_id, type1);
        fprintf(stdout, "INT2STR LF@%s LF@%s\n", s_op1, op1_var);
        fprintf(stdout, "JUMP $op_sconv1_end_%d\n", label_id);
        fprintf(stdout, "LABEL $op_sconv1_float_%d\n", label_id);
        fprintf(stdout, "FLOAT2STR LF@%s LF@%s\n", s_op1, op1_var);
        fprintf(stdout, "JUMP $op_sconv1_end_%d\n", label_id);
        fprintf(stdout, "LABEL $op_sconv1_%d\n", label_id);
        fprintf(stdout, "MOVE LF@%s LF@%s\n", s_op1, op1_var);
        fprintf(stdout, "LABEL $op_sconv1_end_%d\n", label_id);
           
        // Convert op2
        char *s_op2 = createNewVariable(stack);
        defvar_gf(s_op2);
        fprintf(stdout, "JUMPIFEQ $op_sconv2_%d LF@%s string@string\n", label_id, type2);
        fprintf(stdout, "JUMPIFEQ $op_sconv2_float_%d LF@%s string@float\n", label_id, type2);
        fprintf(stdout, "INT2STR LF@%s LF@%s\n", s_op2, op2_var);
        fprintf(stdout, "JUMP $op_sconv2_end_%d\n", label_id);
        fprintf(stdout, "LABEL $op_sconv2_float_%d\n", label_id);
        fprintf(stdout, "FLOAT2STR LF@%s LF@%s\n", s_op2, op2_var);
        fprintf(stdout, "JUMP $op_sconv2_end_%d\n", label_id);
        fprintf(stdout, "LABEL $op_sconv2_%d\n", label_id);
        fprintf(stdout, "MOVE LF@%s LF@%s\n", s_op2, op2_var);
        fprintf(stdout, "LABEL $op_sconv2_end_%d\n", label_id);
            
        fprintf(stdout, "CONCAT LF@%s LF@%s LF@%s\n", result_var, s_op1, s_op2);
        fprintf(stdout, "JUMP %s\n", end_label);
    }
        
    if(abs_tree->token->type == TOKEN_MUL){
        fprintf(stdout, "LABEL $mul_string_%d\n", label_id);
        // Check op2 type
        fprintf(stdout, "JUMPIFEQ $mul_str_op2_int_%d LF@%s string@int\n", label_id, type2);
        // If not int, try float->int?
        fprintf(stdout, "FLOAT2INT LF@%s LF@%s\n", op2_var, op2_var); 
          
        fprintf(stdout, "LABEL $mul_str_op2_int_%d\n", label_id);
           
        // Initialize result = ""
        fprintf(stdout, "MOVE LF@%s string@\n", result_var);
            
        // Loop counter = op2
        char *counter = createNewVariable(stack);
        defvar_gf(counter);
        fprintf(stdout, "MOVE LF@%s LF@%s\n", counter, op2_var);
            
        fprintf(stdout, "LABEL $mul_loop_%d\n", label_id);
        // If counter <= 0, end
        fprintf(stdout, "GT LF@%s LF@%s int@0\n",type4, counter); 
        fprintf(stdout, "JUMPIFEQ $mul_loop_end_%d LF@%s bool@false\n", label_id, type4);
            
        // result = result + op1
        fprintf(stdout, "CONCAT LF@%s LF@%s LF@%s\n", result_var, result_var, op1_var);
            
        // counter--
        fprintf(stdout, "SUB LF@%s LF@%s int@1\n", counter, counter);
        fprintf(stdout, "JUMP $mul_loop_%d\n", label_id);
            
        fprintf(stdout, "LABEL $mul_loop_end_%d\n", label_id);
        fprintf(stdout, "JUMP %s\n", end_label);
    }
        
    fprintf(stdout, "LABEL %s\n", end_label);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type3, result_var);
    fprintf(stdout, "JUMPIFEQ $skip%d LF@%s string@string\n", label_id, type3);
    fprintf(stdout, "JUMPIFEQ $skip%d LF@%s string@int\n", label_id, type3);
    fprintf(stdout, "ISINT LF@%s LF@%s\n", type3, result_var);
    fprintf(stdout, "JUMPIFEQ $skip%d LF@%s bool@false\n", label_id, type3);
    fprintf(stdout, "FLOAT2INT LF@%s LF@%s\n", result_var, result_var);
    fprintf(stdout, "LABEL $skip%d\n", label_id);
    vStackPush(stack, result_var);
}


/**
 * Process Greater Than or Equal (GE) operator
 * @param abs_tree The abstract syntax tree node representing the GE operation
 * @param keywords The keywords node (not used in this function)
 * @param stack The variable stack for managing temporary variables
 * @param result_var The variable to store the result of the GE operation
 * @param child_results The results of the child nodes
 */
static void processGEOpearotr(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack, char* result_var, char *child_results[100]){
    char *op1_var = createNewVariable(stack);
    char *op2_var = createNewVariable(stack);
    char *type1 = createNewVariable(stack);
    char *type2 = createNewVariable(stack);
    defvar_gf(op1_var);
    defvar_gf(op2_var);
    defvar_gf(type1);
    defvar_gf(type2);
    
    // Load operands
    fprintf(stdout, "MOVE LF@%s ", op1_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "MOVE LF@%s ", op2_var);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
    }
    fprintf(stdout, "\n");
    
    // Get types
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1_var);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2_var);
    
    int label_id = global_label_counter++;
    char float_label[50], end_label[50];
    snprintf(float_label, sizeof(float_label), "$lte_float%d", label_id);
    snprintf(end_label, sizeof(end_label), "$lte_end%d", label_id);
    
    char *gt_result = createNewVariable(stack);
    defvar_gf(gt_result);
    
    // Check for float
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type1);
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type2);
    
    // Both are int
    fprintf(stdout, "GT LF@%s LF@%s LF@%s\n", gt_result, op1_var, op2_var);
    fprintf(stdout, "JUMP %s\n", end_label);
    
    // Float handling
    fprintf(stdout, "LABEL %s\n", float_label);
    // Convert op1
    char *f_op1 = createNewVariable(stack);
    defvar_gf(f_op1);
    fprintf(stdout, "JUMPIFEQ $lte_conv1_%d LF@%s string@float\n", label_id, type1);
    fprintf(stdout, "JUMPIFEQ $lte_conv1_int_%d LF@%s string@int\n", label_id, type1);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "JUMP $lte_conv1_end_%d\n", label_id);
    fprintf(stdout, "LABEL $lte_conv1_int_%d\n", label_id);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "JUMP $lte_conv1_end_%d\n", label_id);
    fprintf(stdout, "LABEL $lte_conv1_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "LABEL $lte_conv1_end_%d\n", label_id);
    
    // Convert op2
    char *f_op2 = createNewVariable(stack);
    defvar_gf(f_op2);
    fprintf(stdout, "JUMPIFEQ $lte_conv2_%d LF@%s string@float\n", label_id, type2);
    fprintf(stdout, "JUMPIFEQ $lte_conv2_int_%d LF@%s string@int\n", label_id, type2);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "JUMP $lte_conv2_end_%d\n", label_id);
    fprintf(stdout, "LABEL $lte_conv2_int_%d\n", label_id);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "JUMP $lte_conv2_end_%d\n", label_id);
    fprintf(stdout, "LABEL $lte_conv2_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "LABEL $lte_conv2_end_%d\n", label_id);
    
    fprintf(stdout, "GT LF@%s LF@%s LF@%s\n", gt_result, f_op1, f_op2);
    
    fprintf(stdout, "LABEL %s\n", end_label);
    fprintf(stdout, "NOT LF@%s LF@%s\n", result_var, gt_result);
    vStackPush(stack, result_var);
}


/**
 * Process Less Than or Equal (LE) operator
 * @param abs_tree The abstract syntax tree node representing the LE operation
 * @param keywords The keywords node (not used in this function)
 * @param stack The variable stack for managing temporary variables
 * @param result_var The variable to store the result of the LE operation
 * @param child_results An array of strings representing the results of child nodes
 */
static void processLEOperator(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack, char* result_var, char *child_results[100]){
    char *op1_var = createNewVariable(stack);
    char *op2_var = createNewVariable(stack);
    char *type1 = createNewVariable(stack);
    char *type2 = createNewVariable(stack);
    defvar_gf(op1_var);
    defvar_gf(op2_var);
    defvar_gf(type1);
    defvar_gf(type2);
    
    // Load operands
    fprintf(stdout, "MOVE LF@%s ", op1_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "MOVE LF@%s ", op2_var);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
    }
    fprintf(stdout, "\n");
    
    // Get types
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1_var);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2_var);
    
    int label_id = global_label_counter++;
    char float_label[50], end_label[50];
    snprintf(float_label, sizeof(float_label), "$gte_float%d", label_id);
    snprintf(end_label, sizeof(end_label), "$gte_end%d", label_id);
    
    char *lt_result = createNewVariable(stack);
    defvar_gf(lt_result);
    
    // Check for float
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type1);
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type2);
    
    // Both are int
    fprintf(stdout, "LT LF@%s LF@%s LF@%s\n", lt_result, op1_var, op2_var);
    fprintf(stdout, "JUMP %s\n", end_label);
    
    // Float handling
    fprintf(stdout, "LABEL %s\n", float_label);
    // Convert op1
    char *f_op1 = createNewVariable(stack);
    defvar_gf(f_op1);
    fprintf(stdout, "JUMPIFEQ $gte_conv1_%d LF@%s string@float\n", label_id, type1);
    fprintf(stdout, "JUMPIFEQ $gte_conv1_int_%d LF@%s string@int\n", label_id, type1);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "JUMP $gte_conv1_end_%d\n", label_id);
    fprintf(stdout, "LABEL $gte_conv1_int_%d\n", label_id);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "JUMP $gte_conv1_end_%d\n", label_id);
    fprintf(stdout, "LABEL $gte_conv1_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "LABEL $gte_conv1_end_%d\n", label_id);
    
    // Convert op2
    char *f_op2 = createNewVariable(stack);
    defvar_gf(f_op2);
    fprintf(stdout, "JUMPIFEQ $gte_conv2_%d LF@%s string@float\n", label_id, type2);
    fprintf(stdout, "JUMPIFEQ $gte_conv2_int_%d LF@%s string@int\n", label_id, type2);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "JUMP $gte_conv2_end_%d\n", label_id);
    fprintf(stdout, "LABEL $gte_conv2_int_%d\n", label_id);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "JUMP $gte_conv2_end_%d\n", label_id);
    fprintf(stdout, "LABEL $gte_conv2_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "LABEL $gte_conv2_end_%d\n", label_id);
    
    fprintf(stdout, "LT LF@%s LF@%s LF@%s\n", lt_result, f_op1, f_op2);
    
    fprintf(stdout, "LABEL %s\n", end_label);
    fprintf(stdout, "NOT LF@%s LF@%s\n", result_var, lt_result);
    vStackPush(stack, result_var);

}

/**
 * Process Not Equal (NE) operator
 * @param abs_tree The abstract syntax tree node representing the NE operation
 * @param keywords The keywords node (not used in this function)
 * @param stack The variable stack for managing temporary variables
 * @param result_var The variable to store the result of the NE operation
 * @param child_results An array of strings representing the results of child nodes
 */
static void processNEOperator(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack, char* result_var, char *child_results[100]){
    char *op1_var = createNewVariable(stack);
    char *op2_var = createNewVariable(stack);
    char *type1 = createNewVariable(stack);
    char *type2 = createNewVariable(stack);
    defvar_gf(op1_var);
    defvar_gf(op2_var);
    defvar_gf(type1);
    defvar_gf(type2);
    
    // Load operands
    fprintf(stdout, "MOVE LF@%s ", op1_var);
    if(isVariable(abs_tree->children[0]->token)){
        printFormatedVariable(abs_tree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "MOVE LF@%s ", op2_var);
    if(isVariable(abs_tree->children[1]->token)){
        printFormatedVariable(abs_tree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
    }
    fprintf(stdout, "\n");
    
    // Get types
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1_var);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2_var);
    
    int label_id = global_label_counter++;
    char float_label[50], end_label[50];
    snprintf(float_label, sizeof(float_label), "$neq_float%d", label_id);
    snprintf(end_label, sizeof(end_label), "$neq_end%d", label_id);
    
    char *eq_result = createNewVariable(stack);
    defvar_gf(eq_result);
    
    // Check for float
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type1);
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type2);
    
    // Both are int (or other)
    fprintf(stdout, "EQ LF@%s LF@%s LF@%s\n", eq_result, op1_var, op2_var);
    fprintf(stdout, "JUMP %s\n", end_label);
    
    // Float handling
    fprintf(stdout, "LABEL %s\n", float_label);
    // Convert op1
    char *f_op1 = createNewVariable(stack);
    defvar_gf(f_op1);
    fprintf(stdout, "JUMPIFEQ $neq_conv1_%d LF@%s string@float\n", label_id, type1);
    fprintf(stdout, "JUMPIFEQ $neq_conv1_int_%d LF@%s string@int\n", label_id, type1);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "JUMP $neq_conv1_end_%d\n", label_id);
    fprintf(stdout, "LABEL $neq_conv1_int_%d\n", label_id);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "JUMP $neq_conv1_end_%d\n", label_id);
    fprintf(stdout, "LABEL $neq_conv1_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op1, op1_var);
    fprintf(stdout, "LABEL $neq_conv1_end_%d\n", label_id);
    
    // Convert op2
    char *f_op2 = createNewVariable(stack);
    defvar_gf(f_op2);
    fprintf(stdout, "JUMPIFEQ $neq_conv2_%d LF@%s string@float\n", label_id, type2);
    fprintf(stdout, "JUMPIFEQ $neq_conv2_int_%d LF@%s string@int\n", label_id, type2);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "JUMP $neq_conv2_end_%d\n", label_id);
    fprintf(stdout, "LABEL $neq_conv2_int_%d\n", label_id);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "JUMP $neq_conv2_end_%d\n", label_id);
    fprintf(stdout, "LABEL $neq_conv2_%d\n", label_id);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op2, op2_var);
    fprintf(stdout, "LABEL $neq_conv2_end_%d\n", label_id);
    
    fprintf(stdout, "EQ LF@%s LF@%s LF@%s\n", eq_result, f_op1, f_op2);
    
    fprintf(stdout, "LABEL %s\n", end_label);
    fprintf(stdout, "NOT LF@%s LF@%s\n", result_var, eq_result);
    vStackPush(stack, result_var);
    return;
}

void absToIFJCode25(SyntaktikTreeNode *abs_tree,Node *keywords,vStack *stack){
    if(abs_tree == NULL){
        return;
    }
    
    int number_of_parametrs = 0;
    char *result_var = NULL;
    char *child_results[100] = {NULL}; 
    bool returns_nil = false; 
    
    // Handle internal nodes
    if(abs_tree->token == NULL){
        for(size_t i = 0; i < abs_tree->childrenCount; i++){
            absToIFJCode25(abs_tree->children[i], keywords, stack);
        }
        return;
    }
    
    // Handle leaf nodes (variables and literals)
    if(abs_tree->childrenCount == 0 && isVariable(abs_tree->token)){
        processLeafNodes(abs_tree, stack);
        return;
    }
    
    // Ignore type nodes
    if(abs_tree->token->value != NULL){
        if(strcmp(abs_tree->token->value, "Num") == 0 ||
           strcmp(abs_tree->token->value, "String") == 0 ||
           strcmp(abs_tree->token->value, "Null") == 0 ||
           strcmp(abs_tree->token->value, "null") == 0){
            return;
        }
    }
    

    // Special handling for control structures and variable declarations
    if(abs_tree->token->value != NULL && strcmp(abs_tree->token->value, "if") == 0){
        ifBeforePostorder(abs_tree, keywords, stack);
        return;
    }
        
    if(abs_tree->token->value != NULL && strcmp(abs_tree->token->value, "while") == 0){
        whileBeforePostorder(abs_tree, keywords, stack);
        return;
    }
    
    if(abs_tree->token->value != NULL && strcmp(abs_tree->token->value, "return") == 0){
        returnBeforePostorder(abs_tree, keywords, stack);
        return;
    }
    
    if(abs_tree->token->value != NULL && strcmp(abs_tree->token->value, "var") == 0){
        processVarNode(abs_tree, keywords, stack);
        return;
    }
    
    if(abs_tree->token->value != NULL && strcmp(abs_tree->token->value, "is") == 0 && abs_tree->childrenCount == 2){
        isOpProcessing(abs_tree, keywords, stack, result_var);
        return;
    }
    
    for(size_t i = 0; i < abs_tree->childrenCount; i++){
        absToIFJCode25(abs_tree->children[i], keywords, stack);
    }
    
    // Check for special functions that affect return values
    if(abs_tree->token->value != NULL){
        if(strcmp(abs_tree->token->value, "Ifj.write") == 0){
            returns_nil = true; // write returns nil
        }
    }
    
    // Special handling for Ifj.read_num and Ifj.read_str
    if(abs_tree->token->value != NULL){
        if(strcmp(abs_tree->token->value, "Ifj.read_num") == 0){
            result_var = createNewVariable(stack);
            defvar_gf(result_var);
            fprintf(stdout, "READ LF@%s int\n", result_var);
            vStackPush(stack, result_var);
            return;
        }
        if(strcmp(abs_tree->token->value, "Ifj.read_str") == 0){
            result_var = createNewVariable(stack);
            defvar_gf(result_var);
            fprintf(stdout, "READ LF@%s string\n", result_var);
            vStackPush(stack, result_var);
            return;
        }
        // Speciální zpracování pro Ifj.floor - musí fungovat pro int i float
        if(strcmp(abs_tree->token->value, "Ifj.floor") == 0 && abs_tree->childrenCount == 1){
            processFloor(abs_tree,keywords,stack,result_var);
            return;
        }

        // Speciální zpracování pro Ifj.str - musí fungovat pro int, float i string
        if(strcmp(abs_tree->token->value, "Ifj.str") == 0 && abs_tree->childrenCount == 1){
            processStr(abs_tree,keywords,stack,result_var);
            return;
        }

        if(strcmp(abs_tree->token->value, "Ifj.strcmp") == 0 && abs_tree->childrenCount == 2){
            processStrcmp(abs_tree,keywords,stack,result_var);
            return;
        }

        if(strcmp(abs_tree->token->value, "Ifj.substring") == 0 && abs_tree->childrenCount == 3){
            processSubstring(abs_tree,keywords,stack,result_var);
            return;
        }

        if(strcmp(abs_tree->token->value, "Ifj.chr") == 0 && abs_tree->childrenCount == 1){
            processChr(abs_tree,keywords,stack,result_var);
            return;
        }

        if(strcmp(abs_tree->token->value, "Ifj.ord") == 0 && abs_tree->childrenCount == 2){
            processOrd(abs_tree,keywords,stack,result_var);
            return;
        }
    }
    
    // Special handling for function/getter calls (identifier with children = call)
    if(abs_tree->token->type == TOKEN_IDENTIFIER && abs_tree->childrenCount > 0){
        const char *func_name = abs_tree->token->value;
        int arg_count = abs_tree->childrenCount;
        
        // Resolve arguments
        for(size_t i = 0; i < abs_tree->childrenCount; i++){
            absToIFJCode25(abs_tree->children[i], keywords, stack);
            
            // If child is a simple variable/literal, create a temporary variable
            if(abs_tree->children[i]->childrenCount == 0 && isVariable(abs_tree->children[i]->token)){
                char *temp_var = createNewVariable(stack);
                defvar_gf(temp_var);
                fprintf(stdout, "MOVE LF@%s ", temp_var);
                printFormatedVariable(abs_tree->children[i]->token);
                fprintf(stdout, "\n");
                vStackPush(stack, temp_var);
            }
        }
        
        // Pass arguments through TF before calling
        fprintf(stdout, "CREATEFRAME\n");
        for(int i = arg_count - 1; i >= 0; i--){
            char *arg_result = vStackPop(stack);
            if(arg_result){
                fprintf(stdout, "DEFVAR TF@%%arg%d\n", i);
                if(strstr(arg_result, "int@") != NULL){
                    fprintf(stdout, "MOVE TF@%%arg%d %s\n", i, arg_result);
                }
                else{
                    if(strstr(arg_result, "float@") != NULL){
                        fprintf(stdout, "MOVE TF@%%arg%d %s\n", i, arg_result);
                    }
                    else{
                        if(strstr(arg_result, "string@") != NULL){
                            fprintf(stdout, "MOVE TF@%%arg%d %s\n", i, arg_result);
                        }
                        else{
                            fprintf(stdout, "MOVE TF@%%arg%d LF@%s\n", i, arg_result);
                        }
                    }
                }
            }
        }
        
        // Call function
        fprintf(stdout, "CALL $Program$%s$%d\n", func_name, arg_count);
        
        // Call result
        result_var = createNewVariable(stack);
        defvar_gf(result_var);
        fprintf(stdout, "MOVE LF@%s TF@%%retval\n", result_var);
        vStackPush(stack, result_var);
        return;
    }
    
    // Special handling for getter calls (identifier without children in expression context)
    // Getters are called as values (without parentheses): myValue = unicorn
    if(abs_tree->token->type == TOKEN_IDENTIFIER && abs_tree->childrenCount == 0){
        // Check if a getter with this name exists
        const char *name = abs_tree->token->value;
        char getter_key[256];
        snprintf(getter_key, sizeof(getter_key), "%s{get}", name);
        
        // We assume the Program class for getter lookup
        Node *class_table = getClassTable("Program");
        if(class_table) {
            Node *getter_node = search(class_table, getter_key);
            
            if(getter_node && getter_node->data && getter_node->data->type == TYPE_GETTER){
                // It is a getter - call it
                fprintf(stdout, "CREATEFRAME\n");
                fprintf(stdout, "CALL $Program$%s$get\n", name);
                
                // Getter result
                result_var = createNewVariable(stack);
                defvar_gf(result_var);
                fprintf(stdout, "MOVE LF@%s TF@%%retval\n", result_var);
                vStackPush(stack, result_var);
                return;
            }
        }
    }
    
    // Get results of children from stack (in reverse order - LIFO)
    for(int i = abs_tree->childrenCount - 1; i >= 0; i--){
        // Pop result from stack, IF child is an expression (pushes value)
        if(isExpression(abs_tree->children[i])){
            child_results[i] = vStackPop(stack);
        }
    }
    
    // Check if ASSIGN is a setter call (before printing MOVE)
    bool is_setter_call = false;
    const char *setter_name = NULL;
    
    if(abs_tree->token->type == TOKEN_ASSIGN && abs_tree->childrenCount > 0 && abs_tree->children[0]->token){
        setter_name = abs_tree->children[0]->token->value;
        char setter_key[256];
        snprintf(setter_key, sizeof(setter_key), "%s{set}", setter_name);
        
        Node *class_table = getClassTable("Program");
        if(class_table) {
            Node *setter_node = search(class_table, setter_key);
            
            if(setter_node && setter_node->data && setter_node->data->type == TYPE_SETTER){
                is_setter_call = true;
            }
        }
    }
    
    if(is_setter_call){
        // Setter call: unicorn = 5
        // Right side (value) is already evaluated as child_results[1]
        fprintf(stdout, "CREATEFRAME\n");
        fprintf(stdout, "DEFVAR TF@%%arg0\n");
        
        if(child_results[1] != NULL){
            if(strcmp(child_results[1], "nil") == 0){
                fprintf(stdout, "MOVE TF@%%arg0 nil@nil\n");
            } else {
                if(strstr(child_results[1], "int@") != NULL){
                    fprintf(stdout, "MOVE TF@%%arg0 %s\n", child_results[1]);
                }
                else{
                    if(strstr(child_results[1], "float@") != NULL){
                        fprintf(stdout, "MOVE TF@%%arg0 %s\n", child_results[1]);
                    }
                    else{
                        if(strstr(child_results[1], "string@") != NULL){
                            fprintf(stdout, "MOVE TF@%%arg0 %s\n", child_results[1]);
                        }
                        else{
                            fprintf(stdout, "MOVE TF@%%arg0 LF@%s\n", child_results[1]);
                        }
                    }
                }
            }
        }
        
        fprintf(stdout, "CALL $Program$%s$set\n", setter_name);
        fprintf(stdout, "\n");
        return; // Setter call is a complete statement, does not return a value
    }
    
    // For expressions that are not leaves and not variables, create a temporary variable for the result
    // EXCEPT for assignments (ASSIGN) and functions returning nil
    if(abs_tree->childrenCount > 0 && !isVariable(abs_tree->token) && 
       abs_tree->token->type != TOKEN_ASSIGN && !returns_nil){
        result_var = createNewVariable(stack);
        defvar_gf(result_var);
    }
    
    // Special handling for arithmetic operators (+, -, *)
    if((abs_tree->token->type == TOKEN_PLUS || 
        abs_tree->token->type == TOKEN_MINUS || 
        abs_tree->token->type == TOKEN_MUL) && abs_tree->childrenCount == 2){
        processAritmetik(abs_tree, keywords, stack, result_var, child_results);
        return;
    }
    
    // Special handling for / operator (DIV requires float operands)
    if(abs_tree->token->type == TOKEN_DIV && abs_tree->childrenCount == 2){
        processDiv(abs_tree, keywords, stack, result_var,child_results);
        return;
    }
    
    // Special handling for != operator (must negate the result of EQ)
    if(abs_tree->token->type == TOKEN_NEQ && abs_tree->childrenCount == 2){
        processNEOperator(abs_tree, keywords, stack, result_var, child_results);
        return;
    }
    
    // Special handling for <= operator (a <= b  =>  NOT (a GT b))
    if(abs_tree->token->type == TOKEN_LTE && abs_tree->childrenCount == 2){
        processGEOpearotr(abs_tree, keywords, stack, result_var, child_results);
        return;
    }
    
    // Special handling for >= operator (a >= b  =>  NOT (a LT b))
    if(abs_tree->token->type == TOKEN_GTE && abs_tree->childrenCount == 2){
        processLEOperator(abs_tree, keywords, stack, result_var, child_results);
        return;
    }
    
    // Special handling for <, >, ==
    if((abs_tree->token->type == TOKEN_LT || 
        abs_tree->token->type == TOKEN_GT ||
        abs_tree->token->type == TOKEN_EQ) && abs_tree->childrenCount == 2){
        
        char *op1_var = createNewVariable(stack);
        char *op2_var = createNewVariable(stack);
        char *type1 = createNewVariable(stack);
        char *type2 = createNewVariable(stack);
        defvar_gf(op1_var);
        defvar_gf(op2_var);
        defvar_gf(type1);
        defvar_gf(type2);
        
        // Load operands
        fprintf(stdout, "MOVE LF@%s ", op1_var);
        if(isVariable(abs_tree->children[0]->token)){
            printFormatedVariable(abs_tree->children[0]->token);
        } else {
            printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
        }
        fprintf(stdout, "\n");
        
        fprintf(stdout, "MOVE LF@%s ", op2_var);
        if(isVariable(abs_tree->children[1]->token)){
            printFormatedVariable(abs_tree->children[1]->token);
        } else {
            printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
        }
        fprintf(stdout, "\n");
        
        // Get types
        fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1_var);
        fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2_var);
        
        int label_id = global_label_counter++;
        char float_label[50], end_label[50];
        snprintf(float_label, sizeof(float_label), "$cmp_float%d", label_id);
        snprintf(end_label, sizeof(end_label), "$cmp_end%d", label_id);
        
        // Check for float
        fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type1);
        fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", float_label, type2);
        
        // Both are int (or string/nil/bool - EQ handles them, LT/GT might error if not numbers but we assume semantic check passed or interpreter handles it)
        
        if(abs_tree->token->type == TOKEN_LT) fprintf(stdout, "LT LF@%s LF@%s LF@%s\n", result_var, op1_var, op2_var);
        if(abs_tree->token->type == TOKEN_GT) fprintf(stdout, "GT LF@%s LF@%s LF@%s\n", result_var, op1_var, op2_var);
        if(abs_tree->token->type == TOKEN_EQ) fprintf(stdout, "EQ LF@%s LF@%s LF@%s\n", result_var, op1_var, op2_var);
        fprintf(stdout, "JUMP %s\n", end_label);
        
        // Float handling
        fprintf(stdout, "LABEL %s\n", float_label);
        // Convert op1 if needed
        char *f_op1 = createNewVariable(stack);
        defvar_gf(f_op1);
        fprintf(stdout, "JUMPIFEQ $cmp_conv1_%d LF@%s string@float\n", label_id, type1);
        
        fprintf(stdout, "JUMPIFEQ $cmp_conv1_int_%d LF@%s string@int\n", label_id, type1);
        // Not int, not float (so string/nil/bool) -> keep as is
        fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op1, op1_var);
        fprintf(stdout, "JUMP $cmp_conv1_end_%d\n", label_id);
        
        fprintf(stdout, "LABEL $cmp_conv1_int_%d\n", label_id);
        fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op1, op1_var);
        fprintf(stdout, "JUMP $cmp_conv1_end_%d\n", label_id);
        
        fprintf(stdout, "LABEL $cmp_conv1_%d\n", label_id);
        fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op1, op1_var);
        fprintf(stdout, "LABEL $cmp_conv1_end_%d\n", label_id);
        
        // Convert op2 if needed
        char *f_op2 = createNewVariable(stack);
        defvar_gf(f_op2);
        fprintf(stdout, "JUMPIFEQ $cmp_conv2_%d LF@%s string@float\n", label_id, type2);
        
        fprintf(stdout, "JUMPIFEQ $cmp_conv2_int_%d LF@%s string@int\n", label_id, type2);
        fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op2, op2_var);
        fprintf(stdout, "JUMP $cmp_conv2_end_%d\n", label_id);
        
        fprintf(stdout, "LABEL $cmp_conv2_int_%d\n", label_id);
        fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", f_op2, op2_var);
        fprintf(stdout, "JUMP $cmp_conv2_end_%d\n", label_id);
        
        fprintf(stdout, "LABEL $cmp_conv2_%d\n", label_id);
        fprintf(stdout, "MOVE LF@%s LF@%s\n", f_op2, op2_var);
        fprintf(stdout, "LABEL $cmp_conv2_end_%d\n", label_id);
        
        if(abs_tree->token->type == TOKEN_LT) fprintf(stdout, "LT LF@%s LF@%s LF@%s\n", result_var, f_op1, f_op2);
        if(abs_tree->token->type == TOKEN_GT) fprintf(stdout, "GT LF@%s LF@%s LF@%s\n", result_var, f_op1, f_op2);
        if(abs_tree->token->type == TOKEN_EQ) fprintf(stdout, "EQ LF@%s LF@%s LF@%s\n", result_var, f_op1, f_op2);
        
        fprintf(stdout, "LABEL %s\n", end_label);
        vStackPush(stack, result_var);
        return;
    }
    
    // Get number of parameters for the instruction
    number_of_parametrs = printOperator(abs_tree->token, keywords);
    
    if(number_of_parametrs > 0){
        // Print parameters based on instruction type
        if(number_of_parametrs == 1){
            // Unar operations (1 parameter: instruction + 1 operand)
            if(abs_tree->childrenCount > 0){
                if(isVariable(abs_tree->children[0]->token)){
                    printFormatedVariable(abs_tree->children[0]->token);
                } else {
                    printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
                }
            }
            fprintf(stdout,"\n");
            
            // If the function returns nil, push a special marker
            if(returns_nil){
                vStackPush(stack, "nil");
            }
        }
        else if(number_of_parametrs == 2){
            // Binary instructions (2 parameters: instruction + 1 operand)
            // E.g. INT2STR, FLOAT2STR, INT2CHAR
            if(abs_tree->token->type == TOKEN_ASSIGN){
                // MOVE <destination> <source> (setter was handled above)
                // Normal assignment MOVE <destination> <source>
                if(abs_tree->childrenCount > 0){
                    printFormatedVariable(abs_tree->children[0]->token);
                }
                
                if(abs_tree->childrenCount > 1){
                    // If child_results[1] exists, use it (right side was evaluated as an expression)
                    if(child_results[1] != NULL){
                        if(strcmp(child_results[1], "nil") == 0){
                            fprintf(stdout,"nil@nil ");
                        } else {
                            printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
                        }
                        // ASSIGN is an expression, must return a value (push onto stack)
                        vStackPush(stack, child_results[1]);
                    } else if(isVariable(abs_tree->children[1]->token)){
                        // Otherwise, if it's a variable/literal, print directly
                        printFormatedVariable(abs_tree->children[1]->token);
                        // And push it onto the stack (as a string)
                        if(abs_tree->children[1]->token->type == TOKEN_INT){
                            char val[256];
                            snprintf(val, 256, "int@%s", abs_tree->children[1]->token->value);
                            vStackPush(stack, val);
                        } else if(abs_tree->children[1]->token->type == TOKEN_FLOAT){
                            char val[256];
                            double d = atof(abs_tree->children[1]->token->value);
                            snprintf(val, 256, "float@%a", d);
                            vStackPush(stack, val);
                        } else if(abs_tree->children[1]->token->type == TOKEN_STRING){
                             // String literal handling is complex (escaping), assume handled elsewhere or copy logic?
                             // For simplicity, assume child_results[1] is populated if we fix the loop.
                             // If I fix the loop to pop for variables, child_results[1] WILL be populated.
                             // So this else branch might be unreachable if I fix the loop correctly.
                        } else {
                            vStackPush(stack, abs_tree->children[1]->token->value);
                        }
                    }
 }
                fprintf(stdout,"\n");
            } else {
                // Other binary operations (FLOAT2STR, INT2CHAR, etc.)
                // FORMAT: <instruction> <result> <operand>
                if(result_var != NULL){
                    fprintf(stdout,"LF@%s ", result_var);
                }
                
                if(abs_tree->childrenCount > 0){
                    if(isVariable(abs_tree->children[0]->token)){
                        printFormatedVariable(abs_tree->children[0]->token);
                    } else {
                        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
                    }
                }
                fprintf(stdout,"\n");
            }
        }
        else if(number_of_parametrs == 3){
            // Ternary instructions (e.g. ADD, SUB, MUL, DIV, LT, GT)
            // FORMAT: <instruction> <result> <operand1> <operand2>
            if(result_var != NULL){
                fprintf(stdout,"LF@%s ", result_var);
            }
            
            // First operand
            if(abs_tree->childrenCount > 0){
                if(isVariable(abs_tree->children[0]->token)){
                    printFormatedVariable(abs_tree->children[0]->token);
                } else {
                    printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[0]));
                }
            }
            
            // Second operand
            if(abs_tree->childrenCount > 1){
                if(isVariable(abs_tree->children[1]->token)){
                    printFormatedVariable(abs_tree->children[1]->token);
                } else {
                    printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, child_results[1]));
                }
            }
            fprintf(stdout,"\n");
        }
    }
    
    // Push result_var onto the stack for the parent (if it exists)
    if(result_var != NULL){
        vStackPush(stack, result_var);
    }
}

// Processing class - generates LABEL for all functions/getters/setters
void processClass(SyntaktikTreeNode *class_node, Node *keywords, vStack *stack){
    if(!class_node || !class_node->token || !class_node->token->value) return;
    
    const char *class_name = class_node->token->value;
    
    // Static table of generated functions (to avoid duplicate labels)
    static char generated_functions[1000][256] = {0};
    static int generated_count = 0;
    
    // Iterate over all class members (functions, getters, setters)
    for(size_t i = 0; i < class_node->childrenCount; i++){
        SyntaktikTreeNode *member = class_node->children[i];
        if(!member || !member->token) continue;
        
        // Skip import nodes
        if(member->token->type == TOKEN_KEYWORD && member->token->value &&
           strcmp(member->token->value, "import") == 0) continue;
        
        // Function, getter, or setter
        if(member->token->type == TOKEN_IDENTIFIER){
            const char *func_name = member->token->value;
            
            // Count parameters to get the key for the symbol table
            int param_count = 0;
            bool has_assign = false;
            bool has_round_bracket = false;
            
            for(size_t j = 0; j < member->childrenCount; j++){
                if(member->children[j] && member->children[j]->token){
                    if(member->children[j]->token->type == TOKEN_ASSIGN){
                        has_assign = true;
                    }
                    if(member->children[j]->token->type == TOKEN_ROUND_BRACKET_START){
                        has_round_bracket = true;
                    }
                    if(member->children[j]->token->type != TOKEN_CURLY_BRACKET_START){
                        if(member->children[j]->token->type == TOKEN_IDENTIFIER){
                            param_count++;
                        }
                    } else {
                        break; // Found block start, stop counting
                    }
                }
            }
            
            // Create key - recognize getter/setter based on AST structure
            char func_signature[256] = {0};
            
            // Setter has ASSIGN, function has ROUND_BRACKET, getter has neither
            if(has_assign){
                // It's a setter
                snprintf(func_signature, sizeof(func_signature), "%s::%s{set}", class_name, func_name);
            } else if(!has_round_bracket){
                // It's a getter
                snprintf(func_signature, sizeof(func_signature), "%s::%s{get}", class_name, func_name);
            } else {
                // It's a normal function (has parentheses)
                snprintf(func_signature, sizeof(func_signature), "%s::%s{%d}", class_name, func_name, param_count);
            }
            
            // Check if it has already been generated
            bool already_generated = false;
            for(int j = 0; j < generated_count; j++){
                if(strcmp(generated_functions[j], func_signature) == 0){
                    already_generated = true;
                    break;
                }
            }
            
            if(!already_generated){
                processFunction(member, class_name, keywords, stack);
                // Remember that it has been generated
                if(generated_count < 1000){
                    strncpy(generated_functions[generated_count++], func_signature, 255);
                }
            }
        }
    }
}


void processFunction(SyntaktikTreeNode *func_node, const char *class_name, Node *keywords, vStack *stack){
    if(!func_node || !func_node->token || !func_node->token->value) return;
    
    // Reset declared variables for the new function
    reset_declared_vars();
    
    const char *func_name = func_node->token->value;
    
    Node *class_table = getClassTable(class_name);
    bool is_getter = false;
    bool is_setter = false;
    
    if(class_table){
        char getter_key[256], setter_key[256];
        snprintf(getter_key, sizeof(getter_key), "%s{get}", func_name);
        snprintf(setter_key, sizeof(setter_key), "%s{set}", func_name);
        
        Node *getter_node = search(class_table, getter_key);
        Node *setter_node = search(class_table, setter_key);
        
        if(getter_node && getter_node->data && getter_node->data->type == TYPE_GETTER){
            is_getter = true;
        } else if(setter_node && setter_node->data && setter_node->data->type == TYPE_SETTER){
            is_setter = true;
        }
    }
    

    size_t block_index = 0;
    int param_count = 0;
    
    if(is_getter){
        for(size_t i = 0; i < func_node->childrenCount; i++){
            if(func_node->children[i] && func_node->children[i]->token &&
               func_node->children[i]->token->type == TOKEN_CURLY_BRACKET_START){
                block_index = i;
                break;
            }
        }
    } else if(is_setter){

        bool found_assign = false;
        for(size_t i = 0; i < func_node->childrenCount; i++){
            SyntaktikTreeNode *child = func_node->children[i];
            if(child && child->token){
                if(child->token->type == TOKEN_ASSIGN){
                    found_assign = true;
                } else if(found_assign && child->token->type == TOKEN_CURLY_BRACKET_START){
                    block_index = i;
                    break;
                }
            }
        }
    } else {
        for(size_t i = 0; i < func_node->childrenCount; i++){
            SyntaktikTreeNode *child = func_node->children[i];
            if(child && child->token){
                if(child->token->type == TOKEN_CURLY_BRACKET_START){
                    block_index = i;
                    break;
                } else if(child->token->type == TOKEN_IDENTIFIER){
                    param_count++;
                }
            }
        }
    }
    
    if(is_getter){
        fprintf(stdout, "LABEL $%s$%s$get\n", class_name, func_name);
    } else if(is_setter){
        fprintf(stdout, "LABEL $%s$%s$set\n", class_name, func_name);
    } else {
        fprintf(stdout, "LABEL $%s$%s$%d\n", class_name, func_name, param_count);
    }
    
    fprintf(stdout, "PUSHFRAME\n");
    
    global_var_counter = 0;
    
    if(is_setter){
        // Setter has 1 parameter: val
        fprintf(stdout, "DEFVAR LF@val\n");
        fprintf(stdout, "MOVE LF@val LF@%%arg0\n");
    } else if(!is_getter) {
        for(int i = 0; i < param_count; i++){
            SyntaktikTreeNode *param = func_node->children[i];
            if(param && param->token && param->token->value){
                fprintf(stdout, "DEFVAR LF@%s\n", param->token->value);
                fprintf(stdout, "MOVE LF@%s LF@%%arg%d\n", param->token->value, i);
            }
        }
    }
    
    if(block_index < func_node->childrenCount){
        SyntaktikTreeNode *block = func_node->children[block_index];
        if(block){
            collect_variable_declarations(block);
            
            for(int i = 0; i < declared_vars_count; i++){
                fprintf(stdout, "DEFVAR LF@%s\n", declared_vars[i]);
            }
        }
    }
    
    for(int i = 1; i <= 1000; i++){
        fprintf(stdout, "DEFVAR LF@x%d\n", i);
    }
    
    fprintf(stdout, "DEFVAR LF@%%retval\n");
    fprintf(stdout, "MOVE LF@%%retval nil@nil\n");
    
    if(block_index < func_node->childrenCount){
        SyntaktikTreeNode *block = func_node->children[block_index];
        if(block){
            for(size_t i = 0; i < block->childrenCount; i++){
                absToIFJCode25(block->children[i], keywords, stack);
            }
        }
    }
    
    fprintf(stdout, "POPFRAME\n");
    fprintf(stdout, "RETURN\n");
}

void declareGlobalVariables(Node*global_table){
    if(global_table == NULL){
        return;
    }
    
    declareGlobalVariables(global_table->left);
    
    if(global_table->data != NULL && global_table->data->type == TYPE_VARIABLE){
        if(global_table->key != NULL && global_table->key[0] == '_' && global_table->key[1] == '_'){
            fprintf(stdout,"DEFVAR GF@%s\n",global_table->key);
            fprintf(stdout,"MOVE GF@%s nil@nil\n",global_table->key);
        }
    }
    
    declareGlobalVariables(global_table->right);
}