#include "chibicc.h"
//code generator
static int depth;

// Compute the absolute address of a given node.
// It's an error if a given node does not reside in memory.
static void gen_addr(Node* node) {
	if (node->kind == ND_VAR) {
		int offset = (node->name - 'a' + 1) * 8;
		printf("    lea rax, %d[rbp]\n", -offset);
		return;
	}

	error("not an lvalue");
}

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
		printf("    neg rax\n");
		return;
	}

	switch (node->kind) {

		case ND_VAR:
			gen_addr(node);
			printf("    mov rax, [rax]\n");
			return;
		case ND_ASSIGN:
			gen_addr(node->lhs);
			push();
			gen_expr(node->rhs);
			pop("rdi");
			printf("    mov [rdi], rax\n");
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
		printf("    cmp rax, rdi\n");

		if (node->kind == ND_EQ) {
			//The sete instruction (and its equivalent, setz)
			//sets its argument to 1 if the zero flag is set or to 0 otherwise
			printf("    sete al\n");
		}
		else if (node->kind == ND_NE) {
			// 	SETNE r/m8 	Set byte if not equal (ZF=0).
			printf("    setne al\n");
		}
		else if (node->kind == ND_LT) {
			//SETL r/m8 	Set byte if less (SF<>OF).
			printf("    setl al\n");
		}
		else if (node->kind == ND_LE) {
			// 	SETLE r/m8 	Set byte if less or equal (ZF=1 or SF<>OF).
			printf("    setle al\n");
		}

		//printf("  movzb %%al, %%rax\n");
		printf("    movzx rax, al\n");
		return;
	}
	error("invalid expression");
}

static void gen_stmt(Node* node) {
	if (node->kind == ND_EXPR_STMT) {
		gen_expr(node->lhs);
		return;
	}

	error("invalid statement");
}

void codegen(Node* node) {
	puts("global main");
	puts("section .text");
	puts("main:");
	// Prologue
	printf("    push rbp\n");
	printf("    mov rbp, rsp\n");
	printf("    sub rsp, 208\n");
	// Traverse the AST to emit assembly.
	for (Node* n = node; n; n = n->next) {
		gen_stmt(n);
		assert(depth == 0);
	}
	puts("    mov rsp, rbp");
	puts("    pop rbp");
	puts("ret");
}