global _start

_start:
	;tokendeclident
	sub rsp, 8
	;intlit
	push 4
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
	;tokendeclident
	sub rsp, 8
	;intlit
	push 3
	;tokenassign
	pop rax
	mov [rsp + 0], rax
	;tokenident
	push QWORD [rsp + 16]
	;tokenident
	push QWORD [rsp + 8]
	;tokenmul
	pop rax
	pop rbx
	mul rbx
	push rax
	;return
	pop rax
	add rsp, 8
	ret
