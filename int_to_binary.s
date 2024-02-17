.text 

.global int_to_binary 
int_to_binary:
xorq %rax, %rax
xorq %rdx, %rdx
movl %edi, %eax
xorq %rcx, %rcx
movl $4, %ecx
movl $256, %r9d

div:
idivl %r9d
decq %rcx
movb %dl, (%rsi,%rcx, 1)
xorq %rdx, %rdx
cmpl $0, %eax
jnz  div 

cmp $0, %rcx
jz retorno

cero:
decq %rcx
movb $0, (%rsi,%rcx, 1)
cmp $0, %rcx
jnz cero

retorno:
ret

