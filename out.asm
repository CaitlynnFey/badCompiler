global _start

_start:
	;tokendeclident
	sub rsp, 8
	call func
	push rax
	;tokenassign
	pop rax
	mov [rsp + 8], rax
	;tokenident
	push QWORD [rsp + 8]
	;return (main)
	pop rdi
	mov rax, 60
	syscall
func:
	;intlit
	push 3
	;return
	pop rax
	sub rsp, 0
	ret
