global _start

_start:
	;tokendeclident
	sub rsp, 8
	;intlit
	push 3
	;tokenassign
	pop rax
	mov [rsp + 8], rax
	;tokenident
	push QWORD [rsp + 8]
	;return
	pop rdi
	mov rax, 60
	syscall
