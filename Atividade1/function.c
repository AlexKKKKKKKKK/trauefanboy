#include "proto.h"

#define MAX 1024

typedef struct {
    int top;
    float items[MAX];
} FloatStack;

typedef struct {
    int top;
    char items[MAX];
} CharStack;

void initFloatStack(FloatStack* stack) {
    stack->top = -1;
}

int isFloatEmpty(FloatStack* stack) {
    return stack->top == -1;
}

void pushFloat(FloatStack* stack, float value) {
    if (stack->top < MAX - 1)
        stack->items[++stack->top] = value;
}

float popFloat(FloatStack* stack) {
    if (!isFloatEmpty(stack))
        return stack->items[stack->top--];
    return FLT_MAX;  // Error
}

float peekFloat(FloatStack* stack) {
    return stack->items[stack->top];
}

void initCharStack(CharStack* stack) {
    stack->top = -1;
}

int isCharEmpty(CharStack* stack) {
    return stack->top == -1;
}

void pushChar(CharStack* stack, char c) {
    if (stack->top < MAX - 1)
        stack->items[++stack->top] = c;
}

char popChar(CharStack* stack) {
    if (!isCharEmpty(stack))
        return stack->items[stack->top--];
    return '\0';  // Error
}

char peekChar(CharStack* stack) {
    return stack->items[stack->top];
}

int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

int isOperator(char c) {
    return (c == '+' || c == '-' || c == '*' || c == '/');
}

void infixToPostfix(const char* infix, char* postfix) {
    CharStack operators;
    initCharStack(&operators);
    int i = 0, k = 0;

    while (infix[i] != '\0') {
        if (isspace(infix[i])) {
            i++;
            continue;
        }

        if (isdigit(infix[i]) || infix[i] == '.') {
            while (isdigit(infix[i]) || infix[i] == '.') {
                postfix[k++] = infix[i++];
            }
            postfix[k++] = ' ';
            continue;
        }

        if (infix[i] == '(') {
            pushChar(&operators, infix[i]);
        } else if (infix[i] == ')') {
            while (!isCharEmpty(&operators) && peekChar(&operators) != '(') {
                postfix[k++] = popChar(&operators);
                postfix[k++] = ' ';
            }
            if (!isCharEmpty(&operators) && peekChar(&operators) == '(')
                popChar(&operators);
            else {
                postfix[0] = '\0';
                return;
            }
        } else if (isOperator(infix[i])) {
            while (!isCharEmpty(&operators) && precedence(peekChar(&operators)) >= precedence(infix[i])) {
                postfix[k++] = popChar(&operators);
                postfix[k++] = ' ';
            }
            pushChar(&operators, infix[i]);
        } else {
            postfix[0] = '\0';
            return;
        }

        i++;
    }

    while (!isCharEmpty(&operators)) {
        if (peekChar(&operators) == '(') {
            postfix[0] = '\0';
            return;
        }
        postfix[k++] = popChar(&operators);
        postfix[k++] = ' ';
    }

    postfix[k - 1] = '\0';
}

float evaluatePostfix(const char* postfix) {
    FloatStack stack;
    initFloatStack(&stack);

    int i = 0;
    while (postfix[i] != '\0') {
        if (isspace(postfix[i])) {
            i++;
            continue;
        }

        if (isdigit(postfix[i]) || postfix[i] == '.') {
            float num = 0.0f;
            int decimalPointSeen = 0;
            float decimalPlace = 0.1f;

            while (isdigit(postfix[i]) || (postfix[i] == '.' && !decimalPointSeen)) {
                if (postfix[i] == '.') {
                    decimalPointSeen = 1;
                } else if (decimalPointSeen) {
                    num += (postfix[i] - '0') * decimalPlace;
                    decimalPlace /= 10;
                } else {
                    num = num * 10 + (postfix[i] - '0');
                }
                i++;
            }
            pushFloat(&stack, num);
        } else if (isOperator(postfix[i])) {
            float b = popFloat(&stack);
            float a = popFloat(&stack);
            float result = 0.0f;

            switch (postfix[i]) {
                case '+': result = a + b; break;
                case '-': result = a - b; break;
                case '*': result = a * b; break;
                case '/':
                    if (b == 0.0f) {
                        return FLT_MAX;
                    }
                    result = a / b;
                    break;
            }
            pushFloat(&stack, result);
            i++;
        } else {
            return FLT_MAX;
        }
    }

    return popFloat(&stack);
}

float calculate(const char* infix) {
    char postfix[MAX * 2];
    infixToPostfix(infix, postfix);
    return evaluatePostfix(postfix);
}
