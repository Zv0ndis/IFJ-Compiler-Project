

/** Maximum length of variable name in characters */
#define MAX_LEN_OF_VARIABLE 256
/** Stack capacity for variable storage */
#define CAPACITY 100

/**
 * Stack structure for storing variable names
 * Used for managing temporary variables during code generation
 */
typedef struct{
	char var[CAPACITY][MAX_LEN_OF_VARIABLE];
	int top;
	int capacity;
}vStack;
/**
 * Initialize variable stack
 * @param stack pointer to vStack structure
 */
void vStackInit(vStack *stack);

/**
 * Push variable name onto stack
 * @param stack pointer to vStack structure
 * @param data variable name to push
 */
void vStackPush(vStack *stack, char* data);

/**
 * Pop variable name from stack
 * @param stack pointer to vStack structure
 * @return popped variable name or NULL if empty
 */
char* vStackPop(vStack *stack);

/**
 * Dispose and clear variable stack
 * @param stack pointer to vStack structure
 */
void disposeVStack(vStack *stack);

