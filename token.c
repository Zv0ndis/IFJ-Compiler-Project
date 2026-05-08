/* 
* token.c
* ============================================================
 * Token implementation for the IFJ25 project.
 * As bonus includes helper function for testing and debugging.
 * ============================================================
 * Authors: 
 * Aleš Obr xobral00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "token.h"

static char *xstrdup(const char *s) {
    if (s == NULL) return NULL;
    size_t n = strlen(s) + 1;
    char *r = malloc(n);
    if (r) memcpy(r, s, n);
    return r;
}

void freeToken(Token *token) {
    if (!token) return;
    if (token->value) {
        free(token->value);
        token->value = NULL;
    }
    free(token);
}

Token *createToken(TokenType type, const char *value) {
    Token *token = (Token *)malloc(sizeof(Token));
    if (!token) return NULL;
    token->type = type;
    if (value) {
        token->value = (char*)(value);
        if (!token->value) {
            free(token);
            return NULL;
        }
    } else {
        token->value = NULL;
    }
    return token;
}

const char *tokenTypeName(TokenType t) {
    switch(t) {
        case TOKEN_KEYWORD: return "TOKEN_KEYWORD";
        case TOKEN_IDENTIFIER: return "TOKEN_IDENTIFIER";
        case TOKEN_NUMBER: return "TOKEN_NUMBER";
        case TOKEN_INT: return "TOKEN_INT";
        case TOKEN_FLOAT: return "TOKEN_FLOAT";
        case TOKEN_PLUS: return "TOKEN_PLUS";
        case TOKEN_MINUS: return "TOKEN_MINUS";
        case TOKEN_MUL: return "TOKEN_MUL";
        case TOKEN_DIV: return "TOKEN_DIV";
        case TOKEN_LT: return "TOKEN_LT";
        case TOKEN_LTE: return "TOKEN_LTE";
        case TOKEN_GT: return "TOKEN_GT";
        case TOKEN_GTE: return "TOKEN_GTE";
        case TOKEN_EQ: return "TOKEN_EQ";
        case TOKEN_NEQ: return "TOKEN_NEQ";
        case TOKEN_ASSIGN: return "TOKEN_ASSIGN";
        case TOKEN_SYMBOL: return "TOKEN_SYMBOL";
        case TOKEN_STRING: return "TOKEN_STRING";
        case TOKEN_END_OF_FILE: return "TOKEN_END_OF_FILE";
        case TOKEN_NEWLINE: return "TOKEN_NEWLINE";
        case TOKEN_BLOCK_START: return "TOKEN_BLOCK_START";
        case TOKEN_BLOCK_END: return "TOKEN_BLOCK_END";
        case TOKEN_COMMENT: return "TOKEN_COMMENT";
        case TOKEN_ROUND_BRACKET_START: return "TOKEN_ROUND_BRACKET_START";
        case TOKEN_ROUND_BRACKET_END: return "TOKEN_ROUND_BRACKET_END";
        case TOKEN_SQUARE_BRACKET_START: return "TOKEN_SQUARE_BRACKET_START";
        case TOKEN_SQUARE_BRACKET_END: return "TOKEN_SQUARE_BRACKET_END";
        case TOKEN_CURLY_BRACKET_START: return "TOKEN_CURLY_BRACKET_START";
        case TOKEN_CURLY_BRACKET_END: return "TOKEN_CURLY_BRACKET_END";
        case TOKEN_UNKNOWN: return "TOKEN_UNKNOWN";
        case TOKEN_EOF: return "TOKEN_EOF";
        case TOKEN_COMMA: return "TOKEN_COMMA";
        case TOKEN_GLOBAL_IDENTIFIER : return "TOKEN_GLOBAL_IDENTIFIER";
        default: return "TOKEN_INVALID";
    }
}



char *tokenToString(const Token *token) {
    if (!token) return NULL;
    const char *typeName = tokenTypeName(token->type);
    if (token->value) {
        size_t n = strlen(typeName) + 2 + strlen(token->value) + 1;
        char *s = malloc(n);
        if (!s) return NULL;
        snprintf(s, n, "%s(%s)", typeName, token->value);
        return s;
    } else {
        return xstrdup(typeName);
    }
}

