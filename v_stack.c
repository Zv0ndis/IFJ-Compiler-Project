#include "v_stack.h"
#include <stdlib.h>
#include <string.h>

void vStackInit(vStack *stack){
    stack->capacity = CAPACITY;
    stack->top = -1;
    for(int i = 0;i < CAPACITY;i++){
        stack->var[i][0] = ' ';
        stack->var[i][1] = '\0';
    }
}
void vStackPush(vStack *stack, char* data){
    stack->top = stack->top + 1;
    strcpy(stack->var[stack->top],data);
}
char* vStackPop(vStack *stack){
    if(stack->top > -1){
        stack->top = stack->top - 1;
        return stack->var[stack->top + 1];
    }
    else{
        return NULL;
    }
}

char* vStackPeek(vStack *stack){
    if(stack->top > -1){
        return stack->var[stack->top];
    }
    else{
        return NULL;
    }
}
void disposeVStack(vStack *stack){
    stack->capacity = CAPACITY;
    stack->top = -1;
    for(int i = 0;i < CAPACITY;i++){
        stack->var[i][0] = ' ';
        stack->var[i][1] = '\0';
    }
}