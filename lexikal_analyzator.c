/* 
* lexikal_analyzator.c
 * ============================================================
 * Lexical analyzer implementation for the IFJ25 project.
 * ============================================================
 * Authors: 
 * Jakub Kosinka xkosinj00 FIT VUT v Brně
 * ============================================================
 * Date: 2025
 */

#include "lexikal_analyzator.h"

typedef enum {
    STATE_START,
    FSTATE_IDENTIFIER,
    FSTATE_IFJ_KEYWORD,
    STATE_GIDENTIFIER_CANDIDATE,
    STATE_GIDENTIFIER_EMPTY,
    FSTATE_GIDENTIFIER,
    FSTATE_ZERO,
    FSTATE_INT,
    STATE_HEX_INT_EMPTY,
    FSTATE_HEX_INT,
    FSTATE_FLOAT_DOT_EMPTY,
    FSTATE_FLOAT_DOT,
    STATE_FLOAT_EXPONENT_EMPTY,
    FSTATE_FLOAT_EXPONENT,
    STATE_FLOAT_EXPONENT_SIGN_EMPTY,
    FSTATE_FLOAT_EXPONENT_SIGN,
    FSTATE_STRING,
    STATE_STRING_READ,
    STATE_STRING_EMPTY,
    FSTATE_STRING_DOUBLE_QUOTE,
    STATE_STRING_MULTI_LINE_READ,
    STATE_STRING_MULTI_LINE_QUOTE,
    STATE_STRING_MULTI_LINE_DOUBLE_QUOTE,
    FSTATE_STRING_MULTI_LINE,
    FSTATE_ASSIGNMENT,
    FSTATE_EQ,
    FSTATE_GREATER,
    FSTATE_LESSER,
    FSTATE_GTE,
    FSTATE_LTE,
    STATE_NEG,
    FSTATE_NEQ,
    FSTATE_PLUS,
    FSTATE_MINUS,
    FSTATE_MUL,
    FSTATE_DIV,
    FSTATE_COMMENT_SINGLE_LINE,
    STATE_COMMENT_MULTI_LINE,
    STATE_COMMENT_MULTI_LINE_SLASH,
    STATE_COMMENT_MULTI_LINE_ASTERISK,
    STATE_MULTI_LINE_COMMENT_END,
    STATE_ERROR,
    STATE_ERROR_LEXICAL,
    FSTATE_NEWLINE,
    FSTATE_CURLY_BRACKET_START,
    FSTATE_CURLY_BRACKET_END,
    FSTATE_ROUND_BRACKET_START,
    FSTATE_ROUND_BRACKET_END,
    FSTATE_SQUARE_BRACKET_START,
    FSTATE_SQUARE_BRACKET_END,
    FSTATE_COMMA,
    FSTATE_END
} LexerState;

char keywords[][10] = {
    "class",
    "if",
    "else",
    "is",
    "null",
    "return",
    "var",
    "while",
    "static",
    "import",
    "for",
    "Num",
    "String",
    "Null"
};

char oneCharLexems[] = {'{', '}', '(', ')', '[', ']', '\n', EOF, '-', '+', '*', '/', '=', '<', '>', '!', ','};

size_t line = 1;
size_t character = 0;

/**
 * @brief Checks if a string is a keyword
 * @param str The string to check
 * @return true if the string is a keyword, false otherwise
 */
static bool isKeyword(const char *str) {
    size_t keywordsCount = sizeof(keywords) / sizeof(keywords[0]);
    for (size_t i = 0; i < keywordsCount; i++) {
        if (strcmp(str, keywords[i]) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Reads a character and updates line/character counters
 * @param f File pointer to read from
 * @return The read character, or EOF on end of file
 */
static int readChar(FILE *f) {
    int c = fgetc(f);
    // Update line and character counters
    if (c == '\n') {
        line++;
        character = 0;
    } else {
        character++;
    }
    return c;
}

/**
 * @brief Unreads a character and updates line/character counters
 * @param c The character to unread
 * @param f File pointer to unread from
 */
static void unReadChar(int c, FILE *f) {
    // Update line and character counters
    if (c == '\n') {
        line--;
        // Character reset is not exact but acceptable for unreading
        character = 0; 
    } else {
        character--;
    }
    ungetc(c, f);
}

/**
 * @brief Reads a character with Wren-style escape sequences
 * @param f File pointer to read from
 * @return The read character, or EOF on end of file
 */
static int read_wren_escaped_char(FILE *f) {
    int c = readChar(f);
    if (c == EOF) return EOF;

    if (c != '\\') {
        return c;
    }

    c = readChar(f);
    if (c == EOF) return EOF;

    switch (c) {
        case '"': return '"';
        case '\\': return '\\';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'x': {
            int val = 0;
            for (int i = 0; i < 2; i++) {
                int d = readChar(f);
                if (d == EOF || !isxdigit(d)) {
                    if (d != EOF) unReadChar(d, f);
                    break;
                }
                val = val * 16 + (isdigit(d) ? d - '0' : tolower(d) - 'a' + 10);
            }
            return val & 0xFF;
        }
        default:
            return -2; // Invalid escape sequence
    }
}

/**
 * @brief Skips whitespace characters in the input file
 * @param f File pointer to read from
 */
static void skipWhitespace(FILE *f) {
    int c = readChar(f);
    // Skip whitespace characters except newlines
    while (isspace(c) && c != '\n') {
        c = readChar(f);
    }
    unReadChar(c, f);
}

/**
 * @brief Determines the next state of the lexer based on the current state and input character
 * @param state Pointer to the current state of the lexer
 * @param c The current input character
 * @param ml_com_count Pointer to multi-line comment nesting counter
 * @return true if a final state is reached, false otherwise
 */
static bool setNextState(LexerState *state, int c, size_t *ml_com_count) {
    switch (*state)
    {
    case STATE_START:
        if(c == EOF){
            *state = FSTATE_END;
        }else if(c == '_'){
            *state = STATE_GIDENTIFIER_CANDIDATE;
        }else if(c == '0'){
            *state = FSTATE_ZERO;
        }else if(c == '"'){
            *state = STATE_STRING_EMPTY;
        }else if(c == '='){
            *state = FSTATE_ASSIGNMENT;
        }else if(c == '>'){
            *state = FSTATE_GREATER;
        }else if(c == '<'){
            *state = FSTATE_LESSER;
        }else if(c == '!'){
            *state = STATE_NEG;
        }else if(c == '+'){
            *state = FSTATE_PLUS;
        }else if(c == '-'){
            *state = FSTATE_MINUS;
        }else if(c == '*'){
            *state = FSTATE_MUL;
        }else if(c == '/'){
            *state = FSTATE_DIV;
        }else if(c == '\n'){
            *state = FSTATE_NEWLINE;
        }else if(c == '{'){
            *state = FSTATE_CURLY_BRACKET_START;
        }else if(c == '}'){
            *state = FSTATE_CURLY_BRACKET_END;
        }else if(c == '('){
            *state = FSTATE_ROUND_BRACKET_START;
        }else if(c == ')'){
            *state = FSTATE_ROUND_BRACKET_END;
        }else if(c == '['){
            *state = FSTATE_SQUARE_BRACKET_START;
        }else if(c == ']'){
            *state = FSTATE_SQUARE_BRACKET_END;
        }else if(c == ','){
            *state = FSTATE_COMMA;
        }else if(c >= '1' && c <= '9'){
            *state = FSTATE_INT;
        }else if(isalpha(c)){
            *state = FSTATE_IDENTIFIER;
        }else{
            *state = STATE_ERROR_LEXICAL;
        }
        break;
    case STATE_GIDENTIFIER_CANDIDATE:
        if(c == '_'){
            *state = STATE_GIDENTIFIER_EMPTY;
        }else{
            *state = STATE_ERROR_LEXICAL;
            return true;
        }
        break;
    case STATE_GIDENTIFIER_EMPTY:
        if(isalnum(c) || c == '_'){
            *state = FSTATE_GIDENTIFIER;
        }else{
            *state = FSTATE_GIDENTIFIER;
            return true;
        }
        break;
    case FSTATE_GIDENTIFIER:
        if(isalnum(c) || c == '_'){
            *state = FSTATE_GIDENTIFIER;
        }else{
            return true;
        }
        break;
    case FSTATE_IDENTIFIER:
        if(isalnum(c) || c == '_'){
            *state = FSTATE_IDENTIFIER;
        }else{
            return true;
        }
        break;
    case FSTATE_IFJ_KEYWORD:
        if(isalnum(c) || c == '_' || c == ' ' || c == '.'){
            *state = FSTATE_IFJ_KEYWORD;
        }else{
            return true;
        }
        break;
    case FSTATE_ZERO:
        if(c == 'x' || c == 'X'){
            *state = STATE_HEX_INT_EMPTY;
        }else if(c == '.'){
            *state = FSTATE_FLOAT_DOT_EMPTY;
        }else if(c == 'e' || c == 'E'){
            *state = STATE_FLOAT_EXPONENT_EMPTY;
        }else if (isdigit(c)){
            // Leading zeros are not allowed (e.g., 01, 007)
            *state = STATE_ERROR_LEXICAL;
        }else{
            return true;
        }
        break;
    case FSTATE_INT:
        if(isdigit(c)){
            *state = FSTATE_INT;
        }else if(c == '.'){
            *state = FSTATE_FLOAT_DOT_EMPTY;
        }else if(c == 'e' || c == 'E'){
            *state = STATE_FLOAT_EXPONENT_EMPTY;
        }else{
            return true;
        }
        break;
    case STATE_HEX_INT_EMPTY:
        if(isxdigit(c) ){
            *state = FSTATE_HEX_INT;
        }else{
            *state = STATE_ERROR_LEXICAL;
            return true;
        }
        break;
    case FSTATE_HEX_INT:
        if(isxdigit(c)){
            *state = FSTATE_HEX_INT;
        }else{
            return true;
        }
        break;
    case FSTATE_FLOAT_DOT_EMPTY:
        if(isdigit(c)){
            *state = FSTATE_FLOAT_DOT;
        }else{
            // Allow trailing dot (e.g., "123." is valid as "123.0")
            return true;
        }
        break;
    case FSTATE_FLOAT_DOT:
        if(isdigit(c)){
            *state = FSTATE_FLOAT_DOT;
        }else if(c == 'e' || c == 'E'){
            *state = STATE_FLOAT_EXPONENT_EMPTY;
        }else{
            return true;
        }
        break;
    case STATE_FLOAT_EXPONENT_EMPTY:
        if(isdigit(c)){
            *state = FSTATE_FLOAT_EXPONENT;
        }else if(c == '+' || c == '-'){
            *state = STATE_FLOAT_EXPONENT_SIGN_EMPTY;
        }else{
            *state = STATE_ERROR_LEXICAL;
            return true;
        }
        break;
    case FSTATE_FLOAT_EXPONENT:
        if(isdigit(c)){
            *state = FSTATE_FLOAT_EXPONENT;
        }else{
            return true;
        }
        break;
    case STATE_FLOAT_EXPONENT_SIGN_EMPTY:
        if(isdigit(c)){
            *state = FSTATE_FLOAT_EXPONENT_SIGN;
        }else{
            *state = STATE_ERROR_LEXICAL;
            return true;
        }
        break;
    case FSTATE_FLOAT_EXPONENT_SIGN:
        if(isdigit(c)){
            *state = FSTATE_FLOAT_EXPONENT_SIGN;
        }else{
            return true;
        }
        break;
    case STATE_STRING_EMPTY:
        if(c == '"'){
            *state = FSTATE_STRING_DOUBLE_QUOTE;
        }else if(c == '\n' || c == EOF){
            *state = STATE_ERROR_LEXICAL;
            return true;
        }else{
            *state = STATE_STRING_READ;
        }
        break;
    case FSTATE_STRING_DOUBLE_QUOTE:
        if(c == '"'){
            *state = STATE_STRING_MULTI_LINE_READ;
        }else{
            *state = FSTATE_STRING;
            return true;
        }
        break;
    case STATE_STRING_READ:
        if(c == '"'){
            *state = FSTATE_STRING;
        }else if(c == '\n' || c == EOF){
            *state = STATE_ERROR_LEXICAL;
            return true;
        }else{
            *state = STATE_STRING_READ;
        }
        break;
    case STATE_STRING_MULTI_LINE_READ:
        if(c == '"'){
            *state = STATE_STRING_MULTI_LINE_QUOTE;
        }else{
            *state = STATE_STRING_MULTI_LINE_READ;
        }
        break;
    case STATE_STRING_MULTI_LINE_QUOTE:
        if(c == '"'){
            *state = STATE_STRING_MULTI_LINE_DOUBLE_QUOTE;
        }else{
            *state = STATE_STRING_MULTI_LINE_READ;
        }
        break;
    case STATE_STRING_MULTI_LINE_DOUBLE_QUOTE:
        if(c == '"'){
            *state = FSTATE_STRING_MULTI_LINE;
        }else{
            *state = STATE_STRING_MULTI_LINE_READ;
        }
        break;
    case FSTATE_ASSIGNMENT:
        if(c == '='){
            *state = FSTATE_EQ;
        }else{
            return true;
        }
        break;
    case FSTATE_GREATER:
        if(c == '='){
            *state = FSTATE_GTE;
        }else{
            return true;
        }
        break;
    case FSTATE_LESSER:
        if(c == '='){
            *state = FSTATE_LTE;
        }else{
            return true;
        }
        break;
    case STATE_NEG:
        if(c == '='){
            *state = FSTATE_NEQ;
        }else{
            return true;
        }
        break;
    case FSTATE_DIV:
        if(c == '/'){
            *state = FSTATE_COMMENT_SINGLE_LINE;
        }else if(c == '*'){
            *ml_com_count = *ml_com_count + 1;
            *state = STATE_COMMENT_MULTI_LINE;
        }else{
            return true;
        }
        break;
    case FSTATE_COMMENT_SINGLE_LINE:
        if(c == '\n' || c == EOF){
            return true;
        }else{
            *state = FSTATE_COMMENT_SINGLE_LINE;
        }
        break;
    case STATE_COMMENT_MULTI_LINE:
        if(c == '*'){
            *state = STATE_COMMENT_MULTI_LINE_ASTERISK;
        }else if (c == '/'){
            *state = STATE_COMMENT_MULTI_LINE_SLASH;
        }else if(c == EOF){
            *state = STATE_ERROR_LEXICAL;
            return true;
        }else{
            *state = STATE_COMMENT_MULTI_LINE;
        }
        break;
    case STATE_COMMENT_MULTI_LINE_SLASH:
        if(c == '*'){
            *ml_com_count = *ml_com_count + 1;
            *state = STATE_COMMENT_MULTI_LINE;
        }else if(c == '/'){
            *state = STATE_COMMENT_MULTI_LINE_SLASH;
        }else if(c == EOF){
            *state = STATE_ERROR_LEXICAL;
            return true;
        }else{
            *state = STATE_COMMENT_MULTI_LINE;
        }
        break;
    case STATE_COMMENT_MULTI_LINE_ASTERISK:
        if(c == '/'){
            *state = STATE_MULTI_LINE_COMMENT_END;
        }else if(c == '*'){
            *state = STATE_COMMENT_MULTI_LINE_ASTERISK;
        }else if(c == EOF){
            *state = STATE_ERROR_LEXICAL;
            return true;
        }else{
            *state = STATE_COMMENT_MULTI_LINE;
        }
        break;
    case STATE_MULTI_LINE_COMMENT_END:
        *ml_com_count = *ml_com_count - 1;
        if(*ml_com_count > 0){
            *state = STATE_COMMENT_MULTI_LINE;
            return false;
        }
        *state = STATE_START;
        return false;
        break;
    default:
        return true;
        break;
    }
    return false;
}

/**
 * Final state automata to read characters until a final state is reached
 * @param state Pointer to the current state of the lexer
 * @param buffPTR Pointer to the buffer to store the lexeme
 * @param buffSize Pointer to the size of the buffer
 * @param f File pointer to read from
 */
static void finalStateAutomata(LexerState *state, char **buffPTR, size_t *buffSize, FILE *f) {
    int c = readChar(f);
    char *buff = *buffPTR;
    bool end = false;
    size_t ml_com_count = 0;

    while (!end){
        end = setNextState(state, c, &ml_com_count);
        if(*state == STATE_START){
            if(isspace(c) && c != '\n'){
                skipWhitespace(f);
                c = readChar(f);
            }
            buff[0] = '\0';
            end = setNextState(state, c, &ml_com_count);
            *buffSize = 1;
            buff[0] = c;
            buff[1] = '\0';
            c = readChar(f);
            continue;
        }

        bool wasEscaped = false;
        if((*state == STATE_STRING_READ || *state == STATE_STRING_MULTI_LINE_READ) && c == '\\'){
            unReadChar(c, f);
            int escapedChar = read_wren_escaped_char(f);
            if (escapedChar == -2 || escapedChar == EOF){
                *state = STATE_ERROR_LEXICAL;
            }
            c = escapedChar;
            wasEscaped = true;
        }

        
        
        if (!end){
            if((!(*state == FSTATE_IFJ_KEYWORD && c == ' ')) && !(*state == FSTATE_COMMENT_SINGLE_LINE || *state == STATE_COMMENT_MULTI_LINE || *state == STATE_COMMENT_MULTI_LINE_ASTERISK || *state == STATE_COMMENT_MULTI_LINE_SLASH || *state == STATE_MULTI_LINE_COMMENT_END)){
                buff[*buffSize] = (char)c;
                c = readChar(f);
                *buffSize = (*buffSize) + 1;
                buff = realloc(buff, (*buffSize) + 1);
                if (buff == NULL){
                    error_exit(ERROR_INTERNAL, "ERROR: Memory allocation failed in lexer.\n");
                    *state = STATE_ERROR;
                    return;
                }
                buff[*buffSize] = '\0';
            }else{
                c = readChar(f);
            }
        }

        if(*state == FSTATE_IDENTIFIER){
            if(strncmp(buff, "Ifj", 3) == 0){
                *state = FSTATE_IFJ_KEYWORD;
            }
        }
        
        if(c == EOF){
            break;
        }
    }
    if(c != EOF){
        unReadChar(c, f);
    }
    *buffPTR = buff;

}


/**
 * Create a token based on the final state of the lexer
 * @param state The final state of the lexer
 * @param buff The buffer containing the lexeme
 * @param buffSize The size of the buffer
 * @param err Pointer to an integer to store error code (if any)
 * @return Pointer to the created Token, or NULL on error 
 */
static Token *createTokenFromState(LexerState state, char *buff, size_t buffSize, int *err){
    Token *token = NULL;
    switch (state)
    {
    case FSTATE_IDENTIFIER:
        if(isKeyword(buff)){
            token = createToken(TOKEN_KEYWORD, buff);
        }else{
            token = createToken(TOKEN_IDENTIFIER, buff);
        }
        break;
    case FSTATE_IFJ_KEYWORD:
        token = createToken(TOKEN_KEYWORD, buff);
        break;
    case FSTATE_GIDENTIFIER:
        token = createToken(TOKEN_GLOBAL_IDENTIFIER, buff);
        break;
    case FSTATE_ZERO:
        token = createToken(TOKEN_INT, buff);
        break;
    case FSTATE_INT:
        token = createToken(TOKEN_INT, buff);
        break;
    case FSTATE_HEX_INT:
        token = createToken(TOKEN_INT, buff);
        break;
    case FSTATE_FLOAT_DOT:
        token = createToken(TOKEN_FLOAT, buff);
        break;
    case FSTATE_FLOAT_DOT_EMPTY:
        token = createToken(TOKEN_FLOAT, buff);
        break;
    case FSTATE_FLOAT_EXPONENT:
        token = createToken(TOKEN_FLOAT, buff);
        break;
    case FSTATE_FLOAT_EXPONENT_SIGN:
        token = createToken(TOKEN_FLOAT, buff);
        break;
    case FSTATE_STRING:
        token = createToken(TOKEN_STRING, buff);
        break;
    case FSTATE_STRING_DOUBLE_QUOTE:
        token = createToken(TOKEN_STRING, buff);
        break;
    case FSTATE_STRING_MULTI_LINE:
        token = createToken(TOKEN_STRING, buff);
        break;
    case FSTATE_ASSIGNMENT:
        token = createToken(TOKEN_ASSIGN, NULL);
        free(buff);
        break;
    case FSTATE_EQ:
        token = createToken(TOKEN_EQ, NULL);
        free(buff);
        break;
    case FSTATE_GREATER:
        token = createToken(TOKEN_GT, NULL);
        free(buff);
        break;
    case FSTATE_GTE:
        token = createToken(TOKEN_GTE, NULL);
        free(buff);
        break;
    case FSTATE_LESSER:
        token = createToken(TOKEN_LT, NULL);
        free(buff);
        break;
    case FSTATE_LTE:
        token = createToken(TOKEN_LTE, NULL);
        free(buff);
        break;
    case FSTATE_NEQ:
        token = createToken(TOKEN_NEQ, NULL);
        free(buff);
        break;
    case FSTATE_PLUS:
        token = createToken(TOKEN_PLUS, NULL);
        free(buff);
        break;
    case FSTATE_MINUS:
        token = createToken(TOKEN_MINUS, NULL);
        free(buff);
        break;
    case FSTATE_MUL:
        token = createToken(TOKEN_MUL, NULL);
        free(buff);
        break;
    case FSTATE_DIV:
        token = createToken(TOKEN_DIV, NULL);
        free(buff);
        break;
    case FSTATE_COMMENT_SINGLE_LINE:
        token = createToken(TOKEN_NEWLINE, NULL);
        free(buff);
        break;
    case FSTATE_NEWLINE:
        token = createToken(TOKEN_NEWLINE, NULL);
        free(buff);
        break;
    case FSTATE_CURLY_BRACKET_START:
        token = createToken(TOKEN_CURLY_BRACKET_START, NULL);
        free(buff);
        break;
    case FSTATE_CURLY_BRACKET_END:
        token = createToken(TOKEN_CURLY_BRACKET_END, NULL);
        free(buff);
        break;
    case FSTATE_ROUND_BRACKET_START:
        token = createToken(TOKEN_ROUND_BRACKET_START, NULL);
        free(buff);
        break;
    case FSTATE_ROUND_BRACKET_END:   
        token = createToken(TOKEN_ROUND_BRACKET_END, NULL);
        free(buff);
        break;
    case FSTATE_SQUARE_BRACKET_START:
        token = createToken(TOKEN_SQUARE_BRACKET_START, NULL);
        free(buff);
        break;
    case FSTATE_SQUARE_BRACKET_END:
        token = createToken(TOKEN_SQUARE_BRACKET_END, NULL);
        free(buff);
        break;
    case FSTATE_COMMA:
        token = createToken(TOKEN_COMMA, NULL);
        free(buff);
        break;
    case FSTATE_END:
        token = createToken(TOKEN_END_OF_FILE, NULL);
        free(buff);
        break;
    case STATE_ERROR:
        free(buff);
        if(err != NULL)
            *err = ERROR_INTERNAL;
        error_exit(ERROR_INTERNAL, "ERROR: Internal lexer error.\n");
        return NULL;
        break;
    default:
        free(buff);
        if(err != NULL)
            *err = ERROR_LEXICAL;
        error_exit(ERROR_LEXICAL, "ERROR: Lexical error.\n");
        return NULL;
        break;
    }
    if(token == NULL){
        free(buff);
        if (err != NULL)
            *err = ERROR_INTERNAL;
        error_exit(ERROR_INTERNAL, "ERROR: Token creation failed in lexer.\n");
        return NULL;
    }
    if(err != NULL)
        *err = 0;
    return token;
}


Token* getNextToken(int *retVal, FILE *f, size_t* lineNumber, size_t *charNumber){
    // Initialization
    char *buff = malloc(1);
    if (buff == NULL) {
        if (retVal != NULL) {
            *retVal = ERROR_INTERNAL; // Memory allocation error
        }
        error_exit(ERROR_INTERNAL, "ERROR: Memory allocation failed in lexer.\n");
        return NULL;
    }
    buff[0] = '\0';
    size_t buffSize = 0;
    Token *token = NULL;
    LexerState state = STATE_START;
    
    if (f == NULL) {
        f = stdin;
    }

    skipWhitespace(f);

    // State machine logic to be implemented here
    finalStateAutomata(&state, &buff, &buffSize, f);
    
    // If we ended in START state (e.g. after a block comment), recursively get next token
    // Also handle MULTI_LINE_COMMENT_END which transitions to START
    if (state == STATE_START || state == STATE_MULTI_LINE_COMMENT_END) {
        free(buff);
        return getNextToken(retVal, f, lineNumber, charNumber);
    }
    
    token = createTokenFromState(state, buff, buffSize, retVal);

    // Update line and character numbers
    if(lineNumber != NULL){
        *lineNumber = line;
    }
    if(charNumber != NULL){
        *charNumber = character;
    }

    return token;
}
