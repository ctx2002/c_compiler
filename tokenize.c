#include "chibicc.h"
// Input string
static char* current_input;

static bool startswith(char* p, char* q) {
	return strncmp(p, q, strlen(q)) == 0;
}
// Reports an error and exit.
void error(char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

// Reports an error location and exit.
void verror_at(char* loc, char* fmt, va_list ap) {
	int pos = loc - current_input;
	fprintf(stderr, "%s\n", current_input);
	fprintf(stderr, "%*s", pos, ""); // print pos spaces.
	fprintf(stderr, "^ ");
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	exit(1);
}

void error_at(char* loc, char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(loc, fmt, ap);
}

void error_tok(Token* tok, char* fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	verror_at(tok->loc, fmt, ap);
}

// Consumes the current token if it matches `s`.
bool equal(Token* tok, char* op) {
	return memcmp(tok->loc, op, tok->len) == 0 && op[tok->len] == '\0';
}

// Ensure that the current token is `s`.
Token* skip(Token* tok, char* s) {
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

Token* tokenize(char* p)
{
	current_input = p;
	Token head = { .next = NULL };
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