#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool startswith(char* p, char* q) {
	return strncmp(p, q, strlen(q)) == 0;
}

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
	int len;        // Token length
};

// Input string
static char* current_input;

// Reports an error and exit.
static void error(char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// Reports an error location and exit.
static void verror_at(char* loc, char* fmt, va_list ap) {
	int pos = loc - current_input;
	fprintf(stderr, "%s\n", current_input);
	fprintf(stderr, "%*s", pos, ""); // print pos spaces.
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

static void error_at(char* loc, char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(loc, fmt, ap);
}

static void error_tok(Token* tok, char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(tok->loc, fmt, ap);
}

// Consumes the current token if it matches `s`.
static bool equal(Token* tok, char* op) {
	return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is `s`.
static Token* skip(Token* tok, char* s) {
	if (!equal(tok, s))
		error_tok(tok, "expected '%s'", s);
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

// Read a punctuator token from p and returns its length.
static int read_punct(char* p) {
	if (startswith(p, "==") || startswith(p, "!=") ||
		startswith(p, "<=") || startswith(p, ">="))
		return 2;

	return ispunct(*p) ? 1 : 0;
}

Token* tokenize(void)
{
	char* p = current_input; 
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
		int len = read_punct(p);
		if (len > 0) {
			curr = curr->next = new_token(TK_PUNCT, p, p + len);
			p += curr->len;
			continue;
		}

		error_at(p, "invalid token");
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

//
// Parser
//

typedef enum {
	ND_ADD, // +
	ND_SUB, // -
	ND_MUL, // *
	ND_DIV, // /
	ND_NUM, // Integer
	ND_NEG, // unary -
	ND_EQ,  // ==
	ND_NE,  // !=
	ND_LT,  // <
	ND_LE,  // <=
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
	NodeKind kind; // Node kind
	Node* lhs;     // Left-hand side
	Node* rhs;     // Right-hand side
	int val;       // Used if kind == ND_NUM
};

static Node* new_node(NodeKind kind) {
	Node* node = calloc(1, sizeof(Node));
	node->kind = kind;
	return node;
}

static Node* new_binary(NodeKind kind, Node* lhs, Node* rhs) {
	Node* node = new_node(kind);
	node->lhs = lhs;
	node->rhs = rhs;
	return node;
}

static Node* new_num(int val) {
	Node* node = new_node(ND_NUM);
	node->val = val;
	return node;
}

static Node* new_unary(NodeKind kind, Node* expr)
{
	Node* n = new_node(kind);
	n->lhs = expr;
	return n;
}

static Node* expr(Token** rest, Token* tok);
static Node* equality(Token** rest, Token* tok);
static Node* relational(Token** rest, Token* tok);
static Node* add(Token** rest, Token* tok);
static Node* mul(Token** rest, Token* tok);
static Node* unary(Token** rest, Token* tok);
static Node* primary(Token** rest, Token* tok);


// expr = equality
static Node* expr(Token** rest, Token* tok)
{
	return equality(rest, tok);
}

// equality = relational ("==" relational | "!=" relational)*
static Node* equality(Token** rest, Token* tok)
{
	Node* node = relational(&tok, tok);
	for (;;) {
		if (equal(tok, "==")) {
			node = new_binary(ND_EQ, node, relational(&tok, tok->next));
			continue;
		}

		if (equal(tok, "!=")) {
			node = new_binary(ND_NE, node, relational(&tok, tok->next));
			continue;
		}
		*rest = tok;
		return node;
	}
}
// relational = add ("<" add | "<=" add | ">" add | ">=" add)*
static Node* relational(Token** rest, Token* tok)
{
	Node* node = add(&tok, tok);
	for (;;) {
		if (equal(tok, "<")) {
			node = new_binary(ND_LT, node, add(&tok, tok->next));
			continue;
		}

		if (equal(tok, "<=")) {
			node = new_binary(ND_LE, node, add(&tok, tok->next));
			continue;
		}

		if (equal(tok, ">")) {
			//brilliant
			node = new_binary(ND_LT, add(&tok, tok->next), node);
			continue;
		}

		if (equal(tok, ">=")) {
			//brilliant
			node = new_binary(ND_LE, add(&tok, tok->next), node);
			continue;
		}

		*rest = tok;
		return node;
	}
}
//// add = mul ("+" mul | "-" mul)*
static Node* add(Token** rest, Token* tok)
{
	Node* node = mul(&tok, tok);
	for (;;) {
		if (equal(tok, "+")) {
			node = new_binary(ND_ADD, node, mul(&tok, tok->next));
			continue;
		}

		if (equal(tok, "-")) {
			node = new_binary(ND_SUB, node, mul(&tok, tok->next));
			continue;
		}

		*rest = tok;
		return node;
	}
}
// mul = unary ("*" unary | "/" unary)*
static Node* mul(Token** rest, Token* tok)
{
	Node* node = unary(&tok, tok);
	for (;;) {
		if (equal(tok, "*")) {
			node = new_binary(ND_MUL, node, unary(&tok, tok->next));
			continue;
		}

		if (equal(tok, "/")) {
			node = new_binary(ND_DIV, node, unary(&tok, tok->next));
			continue;
		}

		*rest = tok;
		return node;
	}
}

// unary = ("+" | "-") unary
//       | primary
// assert 10 '-10+20'
// assert 10 '- -10'
// assert 10 '- - +10'
static Node* unary(Token** rest, Token* tok)
{
	if (equal(tok, "+")) {
		return unary(rest, tok->next);
	}

	if (equal(tok, "-")) {
		return new_unary(ND_NEG, unary(rest, tok->next));
	}
	return primary(rest, tok);
}

// primary = "(" expr ")" | num
static Node* primary(Token** rest, Token* tok)
{
	if (equal(tok, "(")) {
		Node* d = expr(&tok, tok->next);
		*rest = skip(tok, ")");
		return d;
	}

	if (tok->kind == TK_NUM) {
		Node* d = new_num(tok->val);
		*rest = tok->next;
		return d;
	}

	error_tok(tok, "expected an expression");
}

//code generator
static int depth;
static void push()
{
	printf("    push rax\n");
	++depth;
}

static void pop(char* reg)
{
	printf("    pop %s\n", reg);
	--depth;
}

static void gen_expr(Node* node)
{
	if (node->kind == ND_NUM) {
		printf("    mov rax, %d\n", node->val);
		return;
	}
	if (node->kind == ND_NEG) {
		gen_expr(node->lhs);
		printf("  neg rax\n");
		return;
	}
	gen_expr(node->rhs);
	push();
	gen_expr(node->lhs);
	//left value in rax
	pop("rdi"); //right value in rdi

	switch (node->kind)
	{
		case ND_ADD:
			printf("    add rax, rdi\n");
			return;
		case ND_SUB:
			printf("    sub rax, rdi\n");
			return;
		case ND_MUL:
			printf("    imul rax, rdi\n");
			return;
		case ND_DIV:
			printf("    cqo\n");
			printf("    idiv rdi\n");
			return;
		case ND_EQ:
		case ND_NE:
		case ND_LT:
		case ND_LE:
			// Compares the first source operand with the second source operand
		  // and sets the status flags in the EFLAGS register according to
		  // the results.
		  // The comparison is performed by subtracting the second operand from the
		  // first operand and then setting the status flags in the same manner as
		  // the SUB instruction. When an immediate value is used as an operand,
		  // it is sign-extended to the length of the first operand
			printf("  cmp rax, rdi\n");

			if (node->kind == ND_EQ) {
				//The sete instruction (and its equivalent, setz)
				//sets its argument to 1 if the zero flag is set or to 0 otherwise
				printf("  sete al\n");
			}
			else if (node->kind == ND_NE) {
				// 	SETNE r/m8 	Set byte if not equal (ZF=0).
				printf("  setne al\n");
			}
			else if (node->kind == ND_LT) {
				//SETL r/m8 	Set byte if less (SF<>OF).
				printf("  setl al\n");
			}
			else if (node->kind == ND_LE) {
				// 	SETLE r/m8 	Set byte if less or equal (ZF=1 or SF<>OF).
				printf("  setle al\n");
			}

			//printf("  movzb %%al, %%rax\n");
			printf("  movzx rax, al\n");
			return;
	}
	error("invalid expression");
}


int main(int argc, char **argv)
{
	if (argc < 1) {
		puts("Usage: c_compiler <expression>");
		exit(0);
	}

	current_input = argv[1];
	Token* tok = tokenize();
	Node *node = expr(&tok, tok);

	if (tok->kind != TK_EOF)
		error_tok(tok, "extra token");

	puts("global main");
	puts("section .text");
	puts("main:");
	// Traverse the AST to emit assembly.
	gen_expr(node);
	puts("ret");
	assert(depth == 0);
	return 0;
}