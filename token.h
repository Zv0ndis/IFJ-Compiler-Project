/* 
* token.h
* ============================================================
 * Token header for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Aleš Obr xobral00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#ifndef TOKEN_H
#define TOKEN_H

#include <string.h>
#include <stdlib.h>

/**
 * @file token.h
 * @brief Token type and helpers for lexical analysis.
 */
typedef enum TokenType {
    TOKEN_KEYWORD,              // e.g., if, else, while -> num // 0
    TOKEN_IDENTIFIER,           // e.g., variable names -> str // 1
    TOKEN_GLOBAL_IDENTIFIER,    // e.g., __dummy -> str // 2
    TOKEN_NUMBER,               // e.g., 123, 45.67 -> num // 3 - unused
    TOKEN_INT,                  // e.g., 123 -> num // 4 
    TOKEN_FLOAT,                 // e.g., 45.67 -> num // 5
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MUL,
    TOKEN_DIV,
    TOKEN_LT,
    TOKEN_LTE,
    TOKEN_GT,
    TOKEN_GTE,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_ASSIGN,
    TOKEN_SYMBOL,
    TOKEN_STRING,
    TOKEN_END_OF_FILE,
    TOKEN_NEWLINE,
    TOKEN_BLOCK_START,
    TOKEN_BLOCK_END,
    TOKEN_COMMENT,
    TOKEN_ROUND_BRACKET_START,
    TOKEN_ROUND_BRACKET_END,
    TOKEN_SQUARE_BRACKET_START,
    TOKEN_SQUARE_BRACKET_END,
    TOKEN_CURLY_BRACKET_START,
    TOKEN_CURLY_BRACKET_END,
    TOKEN_UNKNOWN,
    TOKEN_EOF,
    TOKEN_VAR,
    TOKEN_COMMA,
    TOKEN_GENERATOR_VAR
} TokenType;

/**
 * @brief Represents a lexical token with its type and value.
 *
 * The token and its value are allocated on the heap by createToken and
 * must be freed by freeToken when no longer needed.
 */
typedef struct Token {
    /** The type of the token. */
    TokenType type;
    /** The value of the token as a NUL-terminated string. */
    char *value;
} Token;

/**
 * @brief Allocate and return a new Token on the heap.
 *
 * Copies the provided value string. Caller must call freeToken() to release.
 *
 * @param type Token type
 * @param value NUL-terminated string value (may be NULL)
 * @return Pointer to allocated Token, or NULL on allocation failure
 */
Token *createToken(TokenType type, const char *value);

/**
 * @brief Free a Token previously returned by createToken.
 *
 * Safe to call with NULL.
 *
 * @param token Pointer to Token to free
 */
void freeToken(Token *token);

/**
 * @brief Return the enum name for a TokenType as a NUL-terminated string.
 *
 * The returned pointer is a pointer to a static string and must not be freed.
 */
const char *tokenTypeName(TokenType t);

/**
 * @brief Produce a short human-readable representation of a token.
 *
 * The returned string is heap-allocated and must be freed by the caller.
 */
char *tokenToString(const Token *token);

#endif // TOKEN_H