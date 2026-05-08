/* 
 * error_codes.h
 * ============================================================
 * Defines error codes used throughout the project.
 * ============================================================
 * Authors: 
 * Aleš Obr xobrale00 FIT VUT v Brně
 * Jakub Kosinka xkosinj00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#ifndef ERROR_CODES_H
#define ERROR_CODES_H

#define ERROR_SUCCESS 0
#define ERROR_LEXICAL 1
#define ERROR_SYNTAX 2
#define ERROR_SEMANTIC_UNDEFINED 3
#define ERROR_SEMANTIC_REDEFINITION 4
#define ERROR_SEMANTIC_ARGUMENTS 5
#define ERROR_SEMANTIC_TYPE_MISMATCH 6
#define ERROR_RUNTIME_OTHER 10
#define ERROR_INTERNAL 99

#define ERROR_SEMANTIC_BAD_PARAM 25
#define ERROR_SEMANTIC_ERROR_IN_EXPRESSION 26




#endif // ERROR_CODES_H