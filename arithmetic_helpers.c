
static bool handleBasicMathOps(SyntaktikTreeNode *absTree, vStack *stack, char **childResults, char *resultVar) {
    if (!((absTree->token->type == TOKEN_PLUS || 
           absTree->token->type == TOKEN_MINUS || 
           absTree->token->type == TOKEN_MUL) && absTree->childrenCount == 2)) {
        return false;
    }

    char *op1Var = createNewVariable(stack);
    char *op2Var = createNewVariable(stack);
    char *type1 = createNewVariable(stack);
    char *type2 = createNewVariable(stack);
    char *type3 = createNewVariable(stack);
    char *type4 = createNewVariable(stack);
    defvar_gf(op1Var);
    defvar_gf(op2Var);
    defvar_gf(type1);
    defvar_gf(type2);
    defvar_gf(type3);
    defvar_gf(type4);
    
    // Load operands
    fprintf(stdout, "MOVE LF@%s ", op1Var);
    if(isVariable(absTree->children[0]->token)){
        printFormatedVariable(absTree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[0]));
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "MOVE LF@%s ", op2Var);
    if(isVariable(absTree->children[1]->token)){
        printFormatedVariable(absTree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[1]));
    }
    fprintf(stdout, "\n");
    
    // Get types
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1Var);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2Var);
    
    int labelId = global_label_counter++;
    char floatLabel[50], endLabel[50], stringLabel[50];
    snprintf(floatLabel, sizeof(floatLabel), "$op_float%d", labelId);
    snprintf(endLabel, sizeof(endLabel), "$op_end%d", labelId);
    snprintf(stringLabel, sizeof(stringLabel), "$op_string%d", labelId);
    
    // Check for string (only for PLUS)
    if(absTree->token->type == TOKEN_PLUS){
         fprintf(stdout, "JUMPIFEQ %s LF@%s string@string\n", stringLabel, type1);
         fprintf(stdout, "JUMPIFEQ %s LF@%s string@string\n", stringLabel, type2);
    }
    
    // Check for string (for MUL)
    if(absTree->token->type == TOKEN_MUL){
         fprintf(stdout, "JUMPIFEQ $mul_string_%d LF@%s string@string\n", labelId, type1);
    }
    
    // Check for float
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type1);
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type2);
    
    // Both are int
    if(absTree->token->type == TOKEN_PLUS) fprintf(stdout, "ADD LF@%s LF@%s LF@%s\n", resultVar, op1Var, op2Var);
    if(absTree->token->type == TOKEN_MINUS) fprintf(stdout, "SUB LF@%s LF@%s LF@%s\n", resultVar, op1Var, op2Var);
    if(absTree->token->type == TOKEN_MUL) fprintf(stdout, "MUL LF@%s LF@%s LF@%s\n", resultVar, op1Var, op2Var);
    fprintf(stdout, "JUMP %s\n", endLabel);
    
    // Float handling
    fprintf(stdout, "LABEL %s\n", floatLabel);
    // Convert op1 if needed
    char *fOp1 = createNewVariable(stack);
    defvar_gf(fOp1);
    fprintf(stdout, "JUMPIFEQ $op_conv1_%d LF@%s string@float\n", labelId, type1);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "JUMP $op_conv1_end_%d\n", labelId);
    fprintf(stdout, "LABEL $op_conv1_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "LABEL $op_conv1_end_%d\n", labelId);
    
    // Convert op2 if needed
    char *fOp2 = createNewVariable(stack);
    defvar_gf(fOp2);
    fprintf(stdout, "JUMPIFEQ $op_conv2_%d LF@%s string@float\n", labelId, type2);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "JUMP $op_conv2_end_%d\n", labelId);
    fprintf(stdout, "LABEL $op_conv2_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "LABEL $op_conv2_end_%d\n", labelId);
    
    if(absTree->token->type == TOKEN_PLUS) fprintf(stdout, "ADD LF@%s LF@%s LF@%s\n", resultVar, fOp1, fOp2);
    if(absTree->token->type == TOKEN_MINUS) fprintf(stdout, "SUB LF@%s LF@%s LF@%s\n", resultVar, fOp1, fOp2);
    if(absTree->token->type == TOKEN_MUL) fprintf(stdout, "MUL LF@%s LF@%s LF@%s\n", resultVar, fOp1, fOp2);
    fprintf(stdout, "JUMP %s\n", endLabel);
    
    if(absTree->token->type == TOKEN_PLUS){
        fprintf(stdout, "LABEL %s\n", stringLabel);
        
        // Convert op1
        char *sOp1 = createNewVariable(stack);
        defvar_gf(sOp1);
        fprintf(stdout, "JUMPIFEQ $op_sconv1_%d LF@%s string@string\n", labelId, type1);
        fprintf(stdout, "JUMPIFEQ $op_sconv1_float_%d LF@%s string@float\n", labelId, type1);
        fprintf(stdout, "INT2STR LF@%s LF@%s\n", sOp1, op1Var);
        fprintf(stdout, "JUMP $op_sconv1_end_%d\n", labelId);
        fprintf(stdout, "LABEL $op_sconv1_float_%d\n", labelId);
        fprintf(stdout, "FLOAT2STR LF@%s LF@%s\n", sOp1, op1Var);
        fprintf(stdout, "JUMP $op_sconv1_end_%d\n", labelId);
        fprintf(stdout, "LABEL $op_sconv1_%d\n", labelId);
        fprintf(stdout, "MOVE LF@%s LF@%s\n", sOp1, op1Var);
        fprintf(stdout, "LABEL $op_sconv1_end_%d\n", labelId);
        
        // Convert op2
        char *sOp2 = createNewVariable(stack);
        defvar_gf(sOp2);
        fprintf(stdout, "JUMPIFEQ $op_sconv2_%d LF@%s string@string\n", labelId, type2);
        fprintf(stdout, "JUMPIFEQ $op_sconv2_float_%d LF@%s string@float\n", labelId, type2);
        fprintf(stdout, "INT2STR LF@%s LF@%s\n", sOp2, op2Var);
        fprintf(stdout, "JUMP $op_sconv2_end_%d\n", labelId);
        fprintf(stdout, "LABEL $op_sconv2_float_%d\n", labelId);
        fprintf(stdout, "FLOAT2STR LF@%s LF@%s\n", sOp2, op2Var);
        fprintf(stdout, "JUMP $op_sconv2_end_%d\n", labelId);
        fprintf(stdout, "LABEL $op_sconv2_%d\n", labelId);
        fprintf(stdout, "MOVE LF@%s LF@%s\n", sOp2, op2Var);
        fprintf(stdout, "LABEL $op_sconv2_end_%d\n", labelId);
        
        fprintf(stdout, "CONCAT LF@%s LF@%s LF@%s\n", resultVar, sOp1, sOp2);
        fprintf(stdout, "JUMP %s\n", endLabel);
    }
    
    if(absTree->token->type == TOKEN_MUL){
        fprintf(stdout, "LABEL $mul_string_%d\n", labelId);
        // Check op2 type
        fprintf(stdout, "JUMPIFEQ $mul_str_op2_int_%d LF@%s string@int\n", labelId, type2);
        // If not int, try float->int?
        fprintf(stdout, "FLOAT2INT LF@%s LF@%s\n", op2Var, op2Var); 
        
        fprintf(stdout, "LABEL $mul_str_op2_int_%d\n", labelId);
        
        // Initialize result = ""
        fprintf(stdout, "MOVE LF@%s string@\n", resultVar);
        
        // Loop counter = op2
        char *counter = createNewVariable(stack);
        defvar_gf(counter);
        fprintf(stdout, "MOVE LF@%s LF@%s\n", counter, op2Var);
        
        fprintf(stdout, "LABEL $mul_loop_%d\n", labelId);
        // If counter <= 0, end
        fprintf(stdout, "GT LF@%s LF@%s int@0\n",type4, counter); 
        fprintf(stdout, "JUMPIFEQ $mul_loop_end_%d LF@%s bool@false\n", labelId, type4);
        
        // result = result + op1
        fprintf(stdout, "CONCAT LF@%s LF@%s LF@%s\n", resultVar, resultVar, op1Var);
        
        // counter--
        fprintf(stdout, "SUB LF@%s LF@%s int@1\n", counter, counter);
        fprintf(stdout, "JUMP $mul_loop_%d\n", labelId);
        
        fprintf(stdout, "LABEL $mul_loop_end_%d\n", labelId);
        fprintf(stdout, "JUMP %s\n", endLabel);
    }
    
    fprintf(stdout, "LABEL %s\n", endLabel);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type3, resultVar);
    fprintf(stdout, "JUMPIFEQ $skip%d LF@%s string@string\n", labelId, type3);
    fprintf(stdout, "JUMPIFEQ $skip%d LF@%s string@int\n", labelId, type3);
    fprintf(stdout, "ISINT LF@%s LF@%s\n", type3, resultVar);
    fprintf(stdout, "JUMPIFEQ $skip%d LF@%s bool@false\n", labelId, type3);
    fprintf(stdout, "FLOAT2INT LF@%s LF@%s\n", resultVar, resultVar);
    fprintf(stdout, "LABEL $skip%d\n", labelId);
    vStackPush(stack, resultVar);
    return true;
}

static bool handleDivisionOp(SyntaktikTreeNode *absTree, vStack *stack, char **childResults, char *resultVar) {
    if (!(absTree->token->type == TOKEN_DIV && absTree->childrenCount == 2)) {
        return false;
    }
    
    // Create temporary float variables
    char *floatOp1 = createNewVariable(stack);
    char *floatOp2 = createNewVariable(stack);
    char *typeVar1 = createNewVariable(stack);
    char *typeVar2 = createNewVariable(stack);
    char *isInt = createNewVariable(stack);
    defvar_gf(floatOp1);
    defvar_gf(floatOp2);
    defvar_gf(typeVar1);
    defvar_gf(typeVar2);
    defvar_gf(isInt);
    
    int labelId = global_label_counter++;
    
    // Process first operand
    fprintf(stdout, "TYPE LF@%s ", typeVar1);
    if(isVariable(absTree->children[0]->token)){
        printFormatedVariable(absTree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[0]));
    }
    fprintf(stdout, "\n");
    
    // If int convert; otherwise copy
    fprintf(stdout, "JUMPIFEQ $div_is_float1_%d LF@%s string@float\n", labelId, typeVar1);
    fprintf(stdout, "INT2FLOAT LF@%s ", floatOp1);
    if(isVariable(absTree->children[0]->token)){
        printFormatedVariable(absTree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[0]));
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP $div_op1_done_%d\n", labelId);
    fprintf(stdout, "LABEL $div_is_float1_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s ", floatOp1);
    if(isVariable(absTree->children[0]->token)){
        printFormatedVariable(absTree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[0]));
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "LABEL $div_op1_done_%d\n", labelId);
    
    // Process second operand
    fprintf(stdout, "TYPE LF@%s ", typeVar2);
    if(isVariable(absTree->children[1]->token)){
        printFormatedVariable(absTree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[1]));
    }
    fprintf(stdout, "\n");
    
    // If int convert; otherwise copy
    fprintf(stdout, "JUMPIFEQ $div_is_float2_%d LF@%s string@float\n", labelId, typeVar2);
    fprintf(stdout, "INT2FLOAT LF@%s ", floatOp2);
    if(isVariable(absTree->children[1]->token)){
        printFormatedVariable(absTree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[1]));
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "JUMP $div_op2_done_%d\n", labelId);
    fprintf(stdout, "LABEL $div_is_float2_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s ", floatOp2);
    if(isVariable(absTree->children[1]->token)){
        printFormatedVariable(absTree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[1]));
    }
    fprintf(stdout, "\n");
    fprintf(stdout, "LABEL $div_op2_done_%d\n", labelId);
    
    // Perform float division
    fprintf(stdout, "DIV LF@%s LF@%s LF@%s\n", resultVar, floatOp1, floatOp2);
    fprintf(stdout, "ISINT LF@%s LF@%s\n", isInt, resultVar);
    fprintf(stdout, "JUMPIFEQ $skip%d LF@%s bool@false\n", labelId, isInt);
    fprintf(stdout, "FLOAT2INT LF@%s LF@%s\n", resultVar, resultVar);
    fprintf(stdout, "LABEL $skip%d\n", labelId);
    vStackPush(stack, resultVar);
    return true;
}

static bool handleNeqOp(SyntaktikTreeNode *absTree, vStack *stack, char **childResults, char *resultVar) {
    if (!(absTree->token->type == TOKEN_NEQ && absTree->childrenCount == 2)) {
        return false;
    }
    
    char *op1Var = createNewVariable(stack);
    char *op2Var = createNewVariable(stack);
    char *type1 = createNewVariable(stack);
    char *type2 = createNewVariable(stack);
    defvar_gf(op1Var);
    defvar_gf(op2Var);
    defvar_gf(type1);
    defvar_gf(type2);
    
    // Load operands
    fprintf(stdout, "MOVE LF@%s ", op1Var);
    if(isVariable(absTree->children[0]->token)){
        printFormatedVariable(absTree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[0]));
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "MOVE LF@%s ", op2Var);
    if(isVariable(absTree->children[1]->token)){
        printFormatedVariable(absTree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[1]));
    }
    fprintf(stdout, "\n");
    
    // Get types
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1Var);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2Var);
    
    int labelId = global_label_counter++;
    char floatLabel[50], endLabel[50];
    snprintf(floatLabel, sizeof(floatLabel), "$neq_float%d", labelId);
    snprintf(endLabel, sizeof(endLabel), "$neq_end%d", labelId);
    
    char *eqResult = createNewVariable(stack);
    defvar_gf(eqResult);
    
    // Check for float
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type1);
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type2);
    
    // Both are int (or other)
    fprintf(stdout, "EQ LF@%s LF@%s LF@%s\n", eqResult, op1Var, op2Var);
    fprintf(stdout, "JUMP %s\n", endLabel);
    
    // Float handling
    fprintf(stdout, "LABEL %s\n", floatLabel);
    // Convert op1
    char *fOp1 = createNewVariable(stack);
    defvar_gf(fOp1);
    fprintf(stdout, "JUMPIFEQ $neq_conv1_%d LF@%s string@float\n", labelId, type1);
    fprintf(stdout, "JUMPIFEQ $neq_conv1_int_%d LF@%s string@int\n", labelId, type1);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "JUMP $neq_conv1_end_%d\n", labelId);
    fprintf(stdout, "LABEL $neq_conv1_int_%d\n", labelId);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "JUMP $neq_conv1_end_%d\n", labelId);
    fprintf(stdout, "LABEL $neq_conv1_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "LABEL $neq_conv1_end_%d\n", labelId);
    
    // Convert op2
    char *fOp2 = createNewVariable(stack);
    defvar_gf(fOp2);
    fprintf(stdout, "JUMPIFEQ $neq_conv2_%d LF@%s string@float\n", labelId, type2);
    fprintf(stdout, "JUMPIFEQ $neq_conv2_int_%d LF@%s string@int\n", labelId, type2);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "JUMP $neq_conv2_end_%d\n", labelId);
    fprintf(stdout, "LABEL $neq_conv2_int_%d\n", labelId);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "JUMP $neq_conv2_end_%d\n", labelId);
    fprintf(stdout, "LABEL $neq_conv2_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "LABEL $neq_conv2_end_%d\n", labelId);
    
    fprintf(stdout, "EQ LF@%s LF@%s LF@%s\n", eqResult, fOp1, fOp2);
    
    fprintf(stdout, "LABEL %s\n", endLabel);
    fprintf(stdout, "NOT LF@%s LF@%s\n", resultVar, eqResult);
    vStackPush(stack, resultVar);
    return true;
}

static bool handleLteOp(SyntaktikTreeNode *absTree, vStack *stack, char **childResults, char *resultVar) {
    if (!(absTree->token->type == TOKEN_LTE && absTree->childrenCount == 2)) {
        return false;
    }
    
    char *op1Var = createNewVariable(stack);
    char *op2Var = createNewVariable(stack);
    char *type1 = createNewVariable(stack);
    char *type2 = createNewVariable(stack);
    defvar_gf(op1Var);
    defvar_gf(op2Var);
    defvar_gf(type1);
    defvar_gf(type2);
    
    // Load operands
    fprintf(stdout, "MOVE LF@%s ", op1Var);
    if(isVariable(absTree->children[0]->token)){
        printFormatedVariable(absTree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[0]));
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "MOVE LF@%s ", op2Var);
    if(isVariable(absTree->children[1]->token)){
        printFormatedVariable(absTree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[1]));
    }
    fprintf(stdout, "\n");
    
    // Get types
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1Var);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2Var);
    
    int labelId = global_label_counter++;
    char floatLabel[50], endLabel[50];
    snprintf(floatLabel, sizeof(floatLabel), "$lte_float%d", labelId);
    snprintf(endLabel, sizeof(endLabel), "$lte_end%d", labelId);
    
    char *gtResult = createNewVariable(stack);
    defvar_gf(gtResult);
    
    // Check for float
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type1);
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type2);
    
    // Both are int
    fprintf(stdout, "GT LF@%s LF@%s LF@%s\n", gtResult, op1Var, op2Var);
    fprintf(stdout, "JUMP %s\n", endLabel);
    
    // Float handling
    fprintf(stdout, "LABEL %s\n", floatLabel);
    // Convert op1
    char *fOp1 = createNewVariable(stack);
    defvar_gf(fOp1);
    fprintf(stdout, "JUMPIFEQ $lte_conv1_%d LF@%s string@float\n", labelId, type1);
    fprintf(stdout, "JUMPIFEQ $lte_conv1_int_%d LF@%s string@int\n", labelId, type1);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "JUMP $lte_conv1_end_%d\n", labelId);
    fprintf(stdout, "LABEL $lte_conv1_int_%d\n", labelId);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "JUMP $lte_conv1_end_%d\n", labelId);
    fprintf(stdout, "LABEL $lte_conv1_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "LABEL $lte_conv1_end_%d\n", labelId);
    
    // Convert op2
    char *fOp2 = createNewVariable(stack);
    defvar_gf(fOp2);
    fprintf(stdout, "JUMPIFEQ $lte_conv2_%d LF@%s string@float\n", labelId, type2);
    fprintf(stdout, "JUMPIFEQ $lte_conv2_int_%d LF@%s string@int\n", labelId, type2);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "JUMP $lte_conv2_end_%d\n", labelId);
    fprintf(stdout, "LABEL $lte_conv2_int_%d\n", labelId);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "JUMP $lte_conv2_end_%d\n", labelId);
    fprintf(stdout, "LABEL $lte_conv2_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "LABEL $lte_conv2_end_%d\n", labelId);
    
    fprintf(stdout, "GT LF@%s LF@%s LF@%s\n", gtResult, fOp1, fOp2);
    
    fprintf(stdout, "LABEL %s\n", endLabel);
    fprintf(stdout, "NOT LF@%s LF@%s\n", resultVar, gtResult);
    vStackPush(stack, resultVar);
    return true;
}

static bool handleGteOp(SyntaktikTreeNode *absTree, vStack *stack, char **childResults, char *resultVar) {
    if (!(absTree->token->type == TOKEN_GTE && absTree->childrenCount == 2)) {
        return false;
    }
    
    char *op1Var = createNewVariable(stack);
    char *op2Var = createNewVariable(stack);
    char *type1 = createNewVariable(stack);
    char *type2 = createNewVariable(stack);
    defvar_gf(op1Var);
    defvar_gf(op2Var);
    defvar_gf(type1);
    defvar_gf(type2);
    
    // Load operands
    fprintf(stdout, "MOVE LF@%s ", op1Var);
    if(isVariable(absTree->children[0]->token)){
        printFormatedVariable(absTree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[0]));
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "MOVE LF@%s ", op2Var);
    if(isVariable(absTree->children[1]->token)){
        printFormatedVariable(absTree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[1]));
    }
    fprintf(stdout, "\n");
    
    // Get types
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1Var);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2Var);
    
    int labelId = global_label_counter++;
    char floatLabel[50], endLabel[50];
    snprintf(floatLabel, sizeof(floatLabel), "$gte_float%d", labelId);
    snprintf(endLabel, sizeof(endLabel), "$gte_end%d", labelId);
    
    char *ltResult = createNewVariable(stack);
    defvar_gf(ltResult);
    
    // Check for float
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type1);
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type2);
    
    // Both are int
    fprintf(stdout, "LT LF@%s LF@%s LF@%s\n", ltResult, op1Var, op2Var);
    fprintf(stdout, "JUMP %s\n", endLabel);
    
    // Float handling
    fprintf(stdout, "LABEL %s\n", floatLabel);
    // Convert op1
    char *fOp1 = createNewVariable(stack);
    defvar_gf(fOp1);
    fprintf(stdout, "JUMPIFEQ $gte_conv1_%d LF@%s string@float\n", labelId, type1);
    fprintf(stdout, "JUMPIFEQ $gte_conv1_int_%d LF@%s string@int\n", labelId, type1);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "JUMP $gte_conv1_end_%d\n", labelId);
    fprintf(stdout, "LABEL $gte_conv1_int_%d\n", labelId);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "JUMP $gte_conv1_end_%d\n", labelId);
    fprintf(stdout, "LABEL $gte_conv1_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "LABEL $gte_conv1_end_%d\n", labelId);
    
    // Convert op2
    char *fOp2 = createNewVariable(stack);
    defvar_gf(fOp2);
    fprintf(stdout, "JUMPIFEQ $gte_conv2_%d LF@%s string@float\n", labelId, type2);
    fprintf(stdout, "JUMPIFEQ $gte_conv2_int_%d LF@%s string@int\n", labelId, type2);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "JUMP $gte_conv2_end_%d\n", labelId);
    fprintf(stdout, "LABEL $gte_conv2_int_%d\n", labelId);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "JUMP $gte_conv2_end_%d\n", labelId);
    fprintf(stdout, "LABEL $gte_conv2_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "LABEL $gte_conv2_end_%d\n", labelId);
    
    fprintf(stdout, "LT LF@%s LF@%s LF@%s\n", ltResult, fOp1, fOp2);
    
    fprintf(stdout, "LABEL %s\n", endLabel);
    fprintf(stdout, "NOT LF@%s LF@%s\n", resultVar, ltResult);
    vStackPush(stack, resultVar);
    return true;
}

static bool handleBasicCompareOps(SyntaktikTreeNode *absTree, vStack *stack, char **childResults, char *resultVar) {
    if (!((absTree->token->type == TOKEN_LT || 
           absTree->token->type == TOKEN_GT ||
           absTree->token->type == TOKEN_EQ) && absTree->childrenCount == 2)) {
        return false;
    }
    
    char *op1Var = createNewVariable(stack);
    char *op2Var = createNewVariable(stack);
    char *type1 = createNewVariable(stack);
    char *type2 = createNewVariable(stack);
    defvar_gf(op1Var);
    defvar_gf(op2Var);
    defvar_gf(type1);
    defvar_gf(type2);
    
    // Load operands
    fprintf(stdout, "MOVE LF@%s ", op1Var);
    if(isVariable(absTree->children[0]->token)){
        printFormatedVariable(absTree->children[0]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[0]));
    }
    fprintf(stdout, "\n");
    
    fprintf(stdout, "MOVE LF@%s ", op2Var);
    if(isVariable(absTree->children[1]->token)){
        printFormatedVariable(absTree->children[1]->token);
    } else {
        printFormatedVariable(createToken(TOKEN_GENERATOR_VAR, childResults[1]));
    }
    fprintf(stdout, "\n");
    
    // Get types
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type1, op1Var);
    fprintf(stdout, "TYPE LF@%s LF@%s\n", type2, op2Var);
    
    int labelId = global_label_counter++;
    char floatLabel[50], endLabel[50];
    snprintf(floatLabel, sizeof(floatLabel), "$cmp_float%d", labelId);
    snprintf(endLabel, sizeof(endLabel), "$cmp_end%d", labelId);
    
    // Check for float
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type1);
    fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, type2);
    
    // Both are int (or string/nil/bool - EQ handles them, LT/GT might error if not numbers but we assume semantic check passed or interpreter handles it)
    
    if(absTree->token->type == TOKEN_LT) fprintf(stdout, "LT LF@%s LF@%s LF@%s\n", resultVar, op1Var, op2Var);
    if(absTree->token->type == TOKEN_GT) fprintf(stdout, "GT LF@%s LF@%s LF@%s\n", resultVar, op1Var, op2Var);
    if(absTree->token->type == TOKEN_EQ) fprintf(stdout, "EQ LF@%s LF@%s LF@%s\n", resultVar, op1Var, op2Var);
    fprintf(stdout, "JUMP %s\n", endLabel);
    
    // Float handling
    fprintf(stdout, "LABEL %s\n", floatLabel);
    // Convert op1 if needed
    char *fOp1 = createNewVariable(stack);
    defvar_gf(fOp1);
    fprintf(stdout, "JUMPIFEQ $cmp_conv1_%d LF@%s string@float\n", labelId, type1);
    
    fprintf(stdout, "JUMPIFEQ $cmp_conv1_int_%d LF@%s string@int\n", labelId, type1);
    // Not int, not float (so string/nil/bool) -> keep as is
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "JUMP $cmp_conv1_end_%d\n", labelId);
    
    fprintf(stdout, "LABEL $cmp_conv1_int_%d\n", labelId);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "JUMP $cmp_conv1_end_%d\n", labelId);
    
    fprintf(stdout, "LABEL $cmp_conv1_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp1, op1Var);
    fprintf(stdout, "LABEL $cmp_conv1_end_%d\n", labelId);
    
    // Convert op2 if needed
    char *fOp2 = createNewVariable(stack);
    defvar_gf(fOp2);
    fprintf(stdout, "JUMPIFEQ $cmp_conv2_%d LF@%s string@float\n", labelId, type2);
    
    fprintf(stdout, "JUMPIFEQ $cmp_conv2_int_%d LF@%s string@int\n", labelId, type2);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "JUMP $cmp_conv2_end_%d\n", labelId);
    
    fprintf(stdout, "LABEL $cmp_conv2_int_%d\n", labelId);
    fprintf(stdout, "INT2FLOAT LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "JUMP $cmp_conv2_end_%d\n", labelId);
    
    fprintf(stdout, "LABEL $cmp_conv2_%d\n", labelId);
    fprintf(stdout, "MOVE LF@%s LF@%s\n", fOp2, op2Var);
    fprintf(stdout, "LABEL $cmp_conv2_end_%d\n", labelId);
    
    if(absTree->token->type == TOKEN_LT) fprintf(stdout, "LT LF@%s LF@%s LF@%s\n", resultVar, fOp1, fOp2);
    if(absTree->token->type == TOKEN_GT) fprintf(stdout, "GT LF@%s LF@%s LF@%s\n", resultVar, fOp1, fOp2);
    if(absTree->token->type == TOKEN_EQ) fprintf(stdout, "EQ LF@%s LF@%s LF@%s\n", resultVar, fOp1, fOp2);
    
    fprintf(stdout, "LABEL %s\n", endLabel);
    vStackPush(stack, resultVar);
    return true;
}
