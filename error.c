/* 
* error.c
* ============================================================
 * Error handling implementation for the IFJ25 project. The code is from project in IJC made by author.
 * ============================================================
 * Authors: Jakub Kosinka xkosinj00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "error.h"
#include "error_codes.h"

void warning(const char *fmt, ...)
{   
    // Vypisuje vargs na stderr
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Warning: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
}

void error_exit(int err,const char *fmt, ...)
{
    // Vypisuje vargs na stderr
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "Error: ");
    vfprintf(stderr, fmt, args);
    va_end(args);
    // Ukončí program s chybou
    exit(err);
}