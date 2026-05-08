/* 
 * machine_code_generator.c
 * ============================================================
 * Machine code generator implementation for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Vojtěch Kadlev xkadlev00 FIT VUT v Brně
 * Aleš Obr xobrale00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */
#include "symtable.h"
#include "lexikal_analyzator.h"
#include "syntaktic_analyzator.h"
#include "semantic_functions.h"
#include "machine_code_generator.h"
#include <stdio.h>
#include <string.h>

int main(void) {
    /* Always read from stdin */
    int res = generateIFJCode25(stdin);
    fprintf(stderr, "Result: %d\n", res);
    return res;
}
