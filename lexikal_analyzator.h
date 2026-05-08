/* 
 * lexikal_analyzator.h
 * ============================================================
 * Header file for the lexical analyzer implementation for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Jakub Kosinka xkosinj00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */


#ifndef LEXIKAL_ANALYZATOR_H
#define LEXIKAL_ANALYZATOR_H

#include "token.h"
#include "error_codes.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>



/** 
 * Creates and returns the next token from the input file.
 * @param retVal Pointer to integer for error codes, returns 0 on success. Can be NULL, but its strongly not recommended.
 * @param f Pointer to the input FILE. If NULL, stdin is used.
 * @param lineNumber Pointer to the current line number in the input file, will be updated as tokens are read. Used for error reporting.
 * @param charNumber Pointer to the current character number in the current line of the input file, will be updated as tokens are read. Used for error reporting.
 * @return A pointer to the next token generated from the input file, can be NULL in case of error.
 */
Token* getNextToken(int *retVal, FILE *f, size_t *lineNumber, size_t *charNumber);


#endif