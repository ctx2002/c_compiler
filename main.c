#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef enum {
	TK_PUNCT, // Punctuators
	TK_NUM,   // Numeric literals
	TK_EOF,   // End-of-file markers
} TokenKind;

// Token type
typedef struct Token Token;
struct Token {
	TokenKind kind; // Token kind
	Token* next;    // Next token
	int val;        // If kind is TK_NUM, its value
	char* loc;      // Token location
	int64_t len;        // Token length
};

// Reports an error and exit.
static void error(char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// Consumes the current token if it matches `s`.
static bool equal(Token* tok, char* op) {
	return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is `s`.
static Token* skip(Token* tok, char* s) {
	if (!equal(tok, s))
		error("expected '%s'", s);
	return tok->next;
}

static Token* new_token(TokenKind kind, char* start, char* end)
{
	Token* t = calloc(1, sizeof(Token));
	if (t == NULL) {
		error("Token memory reservation failed.");
	}
	t->kind = kind;
	t->loc = start;
	t->len = end - start;
	return t;
}

Token* tokenize(char* p)
{
	Token head = {.next = NULL};
	Token* curr = &head;

	while (*p) {
		if (isspace(*p)) {
			p++;
			continue;
		}

		if (isdigit(*p)) {
			curr = curr->next = new_token(TK_NUM, p, p);
			char* q = p;
			curr->val = strtoul(p, &p, 10);
			curr->len = p - q;
			continue;
		}

		if (*p == '+' || *p == '-') {
			curr = curr->next = new_token(TK_PUNCT, p, p + 1);
			++p;
			continue;
		}

		error("invalid token");
	}
	curr = curr->next = new_token(TK_EOF, p, p);
	return head.next;
}

int getNumber(Token* tok)
{
	if (tok->kind != TK_NUM) {
		error("not a number");
	}
	return tok->val;
}


int main(int argc, char **argv)
{
	if (argc < 1) {
		puts("Usage: c_compiler <expression>");
		exit(0);
	}

	char* p = argv[1];
	Token* tok = tokenize(p);

	puts("global main");
	puts("section .text");
	puts("main:");

	printf("    mov rax, %ld\n", getNumber(tok));
	tok = tok->next;
	while (tok->kind != TK_EOF) {
        //next must be a + or -
		if (equal(tok, "+")) {
			tok = tok->next;
			printf("    add rax, %ld\n", getNumber(tok));
			tok = tok->next;
			continue;
		}

		tok = skip(tok, "-");
		printf("    sub rax, % ld\n", getNumber(tok));
		tok = tok->next;
	}
	puts("ret");
	return 0;
}