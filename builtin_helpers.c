
static bool handleReadFunctions(SyntaktikTreeNode *absTree, vStack *stack) {
	char *resultVar = NULL;
	if(strcmp(absTree->token->value, "Ifj.read_num") == 0){
		resultVar = createNewVariable(stack);
		defvar_gf(resultVar);
		fprintf(stdout, "READ LF@%s int\n", resultVar);
		vStackPush(stack, resultVar);
		return true;
	}
	if(strcmp(absTree->token->value, "Ifj.read_str") == 0){
		resultVar = createNewVariable(stack);
		defvar_gf(resultVar);
		fprintf(stdout, "READ LF@%s string\n", resultVar);
		vStackPush(stack, resultVar);
		return true;
	}
	if(strcmp(absTree->token->value, "Ifj.read_bool") == 0){
		resultVar = createNewVariable(stack);
		defvar_gf(resultVar);
		fprintf(stdout, "READ LF@%s bool\n", resultVar);
		vStackPush(stack, resultVar);
		return true;
	}
	return false;
}

static bool handleMathConversionFunctions(SyntaktikTreeNode *absTree, Node *keywords, vStack *stack) {
	char *resultVar = NULL;
	// Ifj.floor
	if(strcmp(absTree->token->value, "Ifj.floor") == 0 && absTree->childrenCount == 1){
		absToIFJCode25(absTree->children[0], keywords, stack);
		char *argResult = vStackPop(stack);
		
		resultVar = createNewVariable(stack);
		defvar_gf(resultVar);
		
		char *typeVar = createNewVariable(stack);
		defvar_gf(typeVar);
		
		int labelId = global_label_counter++;
		char floatLabel[50], endLabel[50];
		snprintf(floatLabel, sizeof(floatLabel), "$floor_float%d", labelId);
		snprintf(endLabel, sizeof(endLabel), "$floor_end%d", labelId);
		
		fprintf(stdout, "TYPE LF@%s ", typeVar);
		if(isVariable(absTree->children[0]->token)){
			printFormatedVariable(absTree->children[0]->token);
		} else {
			fprintf(stdout, "LF@%s ", argResult);
		}
		fprintf(stdout, "\n");
		
		fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, typeVar);
		
		fprintf(stdout, "MOVE LF@%s ", resultVar);
		if(isVariable(absTree->children[0]->token)){
			printFormatedVariable(absTree->children[0]->token);
		} else {
			fprintf(stdout, "LF@%s ", argResult);
		}
		fprintf(stdout, "\n");
		fprintf(stdout, "JUMP %s\n", endLabel);
		
		fprintf(stdout, "LABEL %s\n", floatLabel);
		fprintf(stdout, "FLOAT2INT LF@%s ", resultVar);
		if(isVariable(absTree->children[0]->token)){
			printFormatedVariable(absTree->children[0]->token);
		} else {
			fprintf(stdout, "LF@%s ", argResult);
		}
		fprintf(stdout, "\n");
		
		fprintf(stdout, "LABEL %s\n", endLabel);
		
		vStackPush(stack, resultVar);
		return true;
	}
	
	// Ifj.str
	if(strcmp(absTree->token->value, "Ifj.str") == 0 && absTree->childrenCount == 1){
		absToIFJCode25(absTree->children[0], keywords, stack);
		char *argResult = vStackPop(stack);
		
		resultVar = createNewVariable(stack);
		defvar_gf(resultVar);
		
		char *typeVar = createNewVariable(stack);
		defvar_gf(typeVar);
		
		int labelId = global_label_counter++;
		char endLabel[50], stringLabel[50], floatLabel[50];
		
		snprintf(endLabel, sizeof(endLabel), "$str_end%d", labelId);
		snprintf(stringLabel, sizeof(stringLabel), "$str_string%d", labelId);
		snprintf(floatLabel, sizeof(floatLabel), "$str_float%d", labelId);
		
		fprintf(stdout, "TYPE LF@%s ", typeVar);
		if(isVariable(absTree->children[0]->token)){
			printFormatedVariable(absTree->children[0]->token);
		} else {
			fprintf(stdout, "LF@%s ", argResult);
		}
		fprintf(stdout, "\n");
		
		fprintf(stdout, "JUMPIFEQ %s LF@%s string@string\n", stringLabel, typeVar);
		fprintf(stdout, "JUMPIFEQ %s LF@%s string@float\n", floatLabel, typeVar);
		
		fprintf(stdout, "INT2STR LF@%s ", resultVar);
		if(isVariable(absTree->children[0]->token)){
			printFormatedVariable(absTree->children[0]->token);
		} else {
			fprintf(stdout, "LF@%s ", argResult);
		}
		fprintf(stdout, "\n");
		fprintf(stdout, "JUMP %s\n", endLabel);
		
		fprintf(stdout, "LABEL %s\n", floatLabel);
		fprintf(stdout, "FLOAT2STR LF@%s ", resultVar);
		if(isVariable(absTree->children[0]->token)){
			printFormatedVariable(absTree->children[0]->token);
		} else {
			fprintf(stdout, "LF@%s ", argResult);
		}
		fprintf(stdout, "\n");
		fprintf(stdout, "JUMP %s\n", endLabel);
		
		fprintf(stdout, "LABEL %s\n", stringLabel);
		fprintf(stdout, "MOVE LF@%s ", resultVar);
		if(isVariable(absTree->children[0]->token)){
			printFormatedVariable(absTree->children[0]->token);
		} else {
			fprintf(stdout, "LF@%s ", argResult);
		}
		fprintf(stdout, "\n");
		
		fprintf(stdout, "LABEL %s\n", endLabel);
		
		vStackPush(stack, resultVar);
		return true;
	}
	return false;
}

static bool handleCharConversionFunctions(SyntaktikTreeNode *absTree, vStack *stack) {
	char *resultVar = NULL;
	if(strcmp(absTree->token->value, "Ifj.chr") == 0 && absTree->childrenCount == 1){
		int labelId = global_label_counter++;
		char* helpingVar = createNewVariable(stack);
		defvar_gf(helpingVar);
		resultVar = createNewVariable(stack);
		defvar_gf(resultVar);

		if(absTree->children[0]->token->type == TOKEN_INT || absTree->children[0]->token->type == TOKEN_FLOAT || absTree->children[0]->token->type == TOKEN_IDENTIFIER || absTree->children[0]->token->type == TOKEN_GLOBAL_IDENTIFIER){
			if(absTree->children[0]->token->type == TOKEN_FLOAT){
				char* stringFloat = absTree->children[0]->token->value;
				while(*stringFloat != '.' && *stringFloat != '\0'){
					stringFloat++;
				}
				if(*stringFloat == '.'){
					stringFloat++;
					if(*stringFloat != '0'){
						global_error = 26;
						return true;
					}
				}
			}
			fprintf(stdout, "TYPE LF@%s ", resultVar);
			printFormatedVariable(absTree->children[0]->token);
			fprintf(stdout, "\n");
			fprintf(stdout, "JUMPIFEQ $chr_float%d LF@%s string@float\n", labelId, resultVar);
			fprintf(stdout,"JUMP $chr_int%d\n", labelId);

			fprintf(stdout, "LABEL $chr_float%d\n", labelId);
			fprintf(stdout, "FLOAT2INT LF@%s ", helpingVar);
			printFormatedVariable(absTree->children[0]->token);
			fprintf(stdout, "\n");
			fprintf(stdout, "INT2CHAR LF@%s LF@%s\n", resultVar, helpingVar);
			fprintf(stdout,"JUMP $end%d\n", labelId);
			fprintf(stdout, "LABEL $chr_int%d\n", labelId);
			fprintf(stdout, "INT2CHAR LF@%s ", resultVar);
			printFormatedVariable(absTree->children[0]->token);
			fprintf(stdout, "\n");
			fprintf(stdout, "LABEL $end%d\n", labelId);
			vStackPush(stack, resultVar);
			return true;
		}
		else{
			global_error = 25;
			return true;
		}
	}

	if(strcmp(absTree->token->value, "Ifj.ord") == 0 && absTree->childrenCount == 2){
		int labelId = global_label_counter++;
		char* typeVar = createNewVariable(stack);
		defvar_gf(typeVar);
		char* helpingVar = createNewVariable(stack);
		defvar_gf(helpingVar);
		resultVar = createNewVariable(stack);
		defvar_gf(resultVar);

		if(absTree->children[0]->token->type == TOKEN_STRING || absTree->children[0]->token->type == TOKEN_IDENTIFIER || absTree->children[0]->token->type == TOKEN_GLOBAL_IDENTIFIER){
			if(absTree->children[1]->token->type == TOKEN_INT || absTree->children[1]->token->type == TOKEN_FLOAT || absTree->children[1]->token->type == TOKEN_IDENTIFIER || absTree->children[1]->token->type == TOKEN_GLOBAL_IDENTIFIER){
				if(absTree->children[1]->token->type == TOKEN_FLOAT){
					char* stringFloat = absTree->children[0]->token->value;
					while(*stringFloat != '.' && *stringFloat != '\0'){
						stringFloat++;
					}
					if(*stringFloat == '.'){
						stringFloat++;
						if(*stringFloat != '0'){
							global_error = 26;
							return true;
						}
					}
				}
				fprintf(stdout, "TYPE LF@%s ", typeVar);
				printFormatedVariable(absTree->children[1]->token);
				fprintf(stdout, "\n");
				fprintf(stdout, "JUMPIFEQ $is_float%d LF@%s string@float\n",labelId, typeVar);
				fprintf(stdout, "MOVE LF@%s ", helpingVar);
				printFormatedVariable(absTree->children[1]->token);
				fprintf(stdout, "\n");
				fprintf(stdout, "JUMP $start_ord%d\n", labelId);

				fprintf(stdout, "LABEL $is_float%d\n",labelId);
				fprintf(stdout, "FLOAT2INT LF@%s ", helpingVar);
				printFormatedVariable(absTree->children[1]->token);
				fprintf(stdout, "\n");

				fprintf(stdout, "LABEL $start_ord%d\n", labelId);
				fprintf(stdout, "STRI2INT LF@%s ", resultVar);
				printFormatedVariable(absTree->children[0]->token);
				fprintf(stdout, " LF@%s\n", helpingVar);
				vStackPush(stack, resultVar);
				return true;
			}
			else{
				global_error = 25;
				return true;
			}
		}
		else{
			global_error = 25;
			return true;
		}
	}
	return false;
}

static bool handleStrcmp(SyntaktikTreeNode *absTree, vStack *stack) {
	char *resultVar = NULL;
	if(strcmp(absTree->token->value, "Ifj.strcmp") == 0 && absTree->childrenCount == 2){
		int labelId = global_label_counter++;
		resultVar = createNewVariable(stack);
		defvar_gf(resultVar);
		char *result1 = createNewVariable(stack);
		defvar_gf(result1);
		char *result2 = createNewVariable(stack);
		defvar_gf(result2);
		char *result3 = createNewVariable(stack);
		defvar_gf(result3);

		if(absTree->children[0]->token->type == TOKEN_STRING || absTree->children[0]->token->type == TOKEN_IDENTIFIER || absTree->children[0]->token->type == TOKEN_GLOBAL_IDENTIFIER){
			if(absTree->children[1]->token->type == TOKEN_STRING || absTree->children[1]->token->type == TOKEN_IDENTIFIER || absTree->children[1]->token->type == TOKEN_GLOBAL_IDENTIFIER){
				fprintf(stdout, "EQ LF@%s ", result1);
				printFormatedVariable(absTree->children[0]->token);
				printFormatedVariable(absTree->children[1]->token);
				fprintf(stdout, "\n");

				fprintf(stdout, "LT LF@%s ", result2);
				printFormatedVariable(absTree->children[0]->token);
				printFormatedVariable(absTree->children[1]->token);
				fprintf(stdout, "\n");

				fprintf(stdout, "GT LF@%s ", result3);
				printFormatedVariable(absTree->children[0]->token);
				printFormatedVariable(absTree->children[1]->token);
				fprintf(stdout, "\n");
				fprintf(stdout, "JUMPIFEQ $strcmp_eq%d LF@%s bool@true\n",labelId ,result1);
				fprintf(stdout, "JUMPIFEQ $strcmp_lt%d LF@%s bool@true\n",labelId , result2);
				fprintf(stdout, "JUMPIFEQ $strcmp_gt%d LF@%s bool@true\n",labelId , result3);
				fprintf(stdout, "LABEL $strcmp_eq%d\n",labelId);
				fprintf(stdout, "MOVE LF@%s int@0\n", resultVar);
				fprintf(stdout, "JUMP $strcmp_end%d\n",labelId);
				fprintf(stdout, "LABEL $strcmp_lt%d\n", labelId);
				fprintf(stdout, "MOVE LF@%s int@-1\n", resultVar);
				fprintf(stdout, "JUMP $strcmp_end%d\n",labelId);
				fprintf(stdout, "LABEL $strcmp_gt%d\n", labelId);
				fprintf(stdout, "MOVE LF@%s int@1\n", resultVar);
				fprintf(stdout, "LABEL $strcmp_end%d\n", labelId);
				vStackPush(stack, resultVar);
				return true;
			}
			else{
				global_error = 25;
				return true;
			}
		}
		else{
			global_error = 25;
			return true;
		}
	}
	return false;
}

static bool handleSubstring(SyntaktikTreeNode *absTree, vStack *stack) {
	char *resultVar = NULL;
	if(strcmp(absTree->token->value, "Ifj.substring") == 0 && absTree->childrenCount == 3){
		int labelId = global_label_counter++;
		char* part = createNewVariable(stack);
		defvar_gf(part);
		resultVar = createNewVariable(stack);
		defvar_gf(resultVar);
		char* start = createNewVariable(stack);
		defvar_gf(start);
		char* end = createNewVariable(stack);
		defvar_gf(end);
		char *typeVar1 = createNewVariable(stack);
		defvar_gf(typeVar1);
		char *typeVar2 = createNewVariable(stack);
		defvar_gf(typeVar2);
		char *isBadValue = createNewVariable(stack);
		defvar_gf(isBadValue);
		char *length = createNewVariable(stack);
		defvar_gf(length);

		if(absTree->children[0]->token->type == TOKEN_STRING || absTree->children[0]->token->type == TOKEN_IDENTIFIER || absTree->children[0]->token->type == TOKEN_GLOBAL_IDENTIFIER){
			if(absTree->children[1]->token->type == TOKEN_INT || absTree->children[1]->token->type == TOKEN_FLOAT || absTree->children[1]->token->type == TOKEN_IDENTIFIER || absTree->children[1]->token->type == TOKEN_GLOBAL_IDENTIFIER){
				if(absTree->children[1]->token->type == TOKEN_FLOAT){
					char* stringFloat = absTree->children[0]->token->value;
					while(*stringFloat != '.' && *stringFloat != '\0'){
						stringFloat++;
					}
					if(*stringFloat == '.'){
						stringFloat++;
						if(*stringFloat != '0'){
							global_error = 26;
							return true;
						}
					}
				}
				if(absTree->children[2]->token->type == TOKEN_INT || absTree->children[2]->token->type == TOKEN_FLOAT || absTree->children[2]->token->type == TOKEN_IDENTIFIER || absTree->children[2]->token->type == TOKEN_GLOBAL_IDENTIFIER){
					if(absTree->children[2]->token->type == TOKEN_FLOAT){
						char* stringFloat = absTree->children[0]->token->value;
						while(*stringFloat != '.' && *stringFloat != '\0'){
							stringFloat++;
						}
						if(*stringFloat == '.'){
							stringFloat++;
							if(*stringFloat != '0'){
								global_error = 26;
								return true;
							}
						}
					}
					fprintf(stdout, "TYPE LF@%s ", typeVar1);
					printFormatedVariable(absTree->children[1]->token);
					fprintf(stdout, "\n");
					fprintf(stdout, "TYPE LF@%s ", typeVar2);
					printFormatedVariable(absTree->children[2]->token);
					fprintf(stdout, "\n");

					fprintf(stdout, "JUMPIFEQ $is_float%d LF@%s string@float\n",labelId, typeVar1);

					fprintf(stdout, "MOVE LF@%s ", start);
					printFormatedVariable(absTree->children[1]->token);
					fprintf(stdout, "\n");

					fprintf(stdout, "LT LF@%s LF@%s int@0\n",isBadValue, start);
					fprintf(stdout, "JUMPIFNEQ $next_if%d LF@%s bool@true\n",labelId,isBadValue);
					fprintf(stdout, "MOVE LF@%s nil@nil\n",resultVar);
					fprintf(stdout, "JUMP $end%d\n",labelId);

					fprintf(stdout, "LABEL $next_if%d\n",labelId);
					fprintf(stdout, "STRLEN LF@%s ",length);
					printFormatedVariable(absTree->children[0]->token);
					fprintf(stdout,"\n");

					fprintf(stdout, "GT LF@%s LF@%s LF@%s\n",isBadValue,start,length);
					fprintf(stdout, "JUMPIFNEQ $check_second%d LF@%s bool@true\n",labelId,isBadValue);
					fprintf(stdout, "MOVE LF@%s nil@nil\n",resultVar);
					fprintf(stdout, "JUMP $end%d\n",labelId);

					fprintf(stdout, "LABEL $check_second%d\n", labelId);

					fprintf(stdout, "JUMPIFEQ $is_float2%d LF@%s string@float\n",labelId, typeVar2);

					fprintf(stdout, "MOVE LF@%s ", end);
					printFormatedVariable(absTree->children[2]->token);
					fprintf(stdout, "\n");

					fprintf(stdout, "LT LF@%s LF@%s int@0\n",isBadValue, end);
					fprintf(stdout, "JUMPIFNEQ $next__if%d LF@%s bool@true\n",labelId,isBadValue);
					fprintf(stdout, "MOVE LF@%s nil@nil\n",resultVar);
					fprintf(stdout, "JUMP $end%d\n",labelId);

					fprintf(stdout, "LABEL $next__if%d\n",labelId);
					fprintf(stdout, "STRLEN LF@%s ",length);
					printFormatedVariable(absTree->children[0]->token);
					fprintf(stdout,"\n");

					fprintf(stdout, "GT LF@%s LF@%s LF@%s\n",isBadValue,end,length);
					fprintf(stdout, "JUMPIFNEQ $start_substr%d LF@%s bool@true\n",labelId,isBadValue);
					fprintf(stdout, "MOVE LF@%s nil@nil\n",resultVar);
					fprintf(stdout, "JUMP $end%d\n",labelId);

					fprintf(stdout, "JUMP $start_substr%d\n", labelId);

					fprintf(stdout, "LABEL $is_float%d\n", labelId);
					fprintf(stdout, "FLOAT2INT LF@%s ", start);
					printFormatedVariable(absTree->children[1]->token);
					fprintf(stdout, "\n");
					fprintf(stdout, "JUMP $check_second%d\n", labelId);

					fprintf(stdout, "LABEL $is_float2%d\n", labelId);
					fprintf(stdout, "FLOAT2INT LF@%s ", end);
					printFormatedVariable(absTree->children[2]->token);
					fprintf(stdout, "\n");

					fprintf(stdout, "LABEL $start_substr%d\n", labelId);
					fprintf(stdout, "GETCHAR LF@%s ", resultVar);
					printFormatedVariable(absTree->children[0]->token);
					fprintf(stdout, " LF@%s\n", start);

					fprintf(stdout, "SUB LF@%s LF@%s int@1\n", end, end);
	
					fprintf(stdout, "LABEL $for_substr%d\n", labelId);
					fprintf(stdout, "ADD LF@%s LF@%s int@1\n", start, start);

					fprintf(stdout, "GETCHAR LF@%s ", part);
					printFormatedVariable(absTree->children[0]->token);
					fprintf(stdout, " LF@%s\n", start);
					fprintf(stdout, "\n");
					fprintf(stdout, "CONCAT LF@%s LF@%s LF@%s\n", resultVar, resultVar, part);
					fprintf(stdout, "JUMPIFNEQ $for_substr%d LF@%s LF@%s\n", labelId, start, end);
					fprintf(stdout,"LABEL $end%d\n",labelId);
	
					vStackPush(stack, resultVar);
					return true;
				}
				else{
					global_error = 25;
					return true;
				}
			}
			else{
				global_error = 25;
				return true;
			}
		}
		else{
			global_error = 25;
			return true;
		}
	}
	return false;
}

static bool handleBuiltinFunctions(SyntaktikTreeNode *absTree, Node *keywords, vStack *stack) {
	if (absTree->token->value == NULL) return false;
	
	if (handleReadFunctions(absTree, stack)) return true;
	if (handleMathConversionFunctions(absTree, keywords, stack)) return true;
	if (handleCharConversionFunctions(absTree, stack)) return true;
	if (handleStrcmp(absTree, stack)) return true;
	if (handleSubstring(absTree, stack)) return true;
	
	return false;
}
