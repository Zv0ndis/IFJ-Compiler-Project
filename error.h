 /* 
* error.h
* ============================================================
 * Error handling implementation for the IFJ25 project. The code is from project in IJC made by author.
 * ============================================================
 * Authors: Jakub Kosinka xkosinj00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */
 
 #ifndef ERROR_H
 #define ERROR_H

 #include "error_codes.h"

    /**
     * @brief Funkce pro výpis varovné hlášky na stderr
     * @param fmt formátovaný řetězec
     * @param ... proměnný počet argumentů
     * @return void
     */
    extern void warning(const char *fmt, ...);

    /**
     * @brief Funkce pro výpis chybové hlášky na stderr a ukončení programu s návratovým kódem 1
     * @param fmt formátovaný řetězec
     * @param ... proměnný počet argumentů
     * @return void
     */
    extern void error_exit(int err,const char *fmt, ...);

 #endif