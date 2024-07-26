global _start

_start:
	;intlit
	push 1
	;intlit
	push 2
	;intlit
	push 3
	call func
	push rax
	;return (main)
	pop rdi
	mov rax, 60
	syscall
func:
	;tokenident
	push QWORD [rsp + 24]
	;tokenident
	push QWORD [rsp + 24]
	;tokenident
	push QWORD [rsp + 24]
	;tokenplus
	pop rax
	pop rbx
	add rax, rbx
	push rax
	;tokenplus
	pop rax
	pop rbx
	add rax, rbx
	push rax
	;return
	pop rax
	add rsp, 0
	ret
