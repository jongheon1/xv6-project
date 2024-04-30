# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save the current registers on the stack, creating
# a struct context, and save its address in *old.
# Switch stacks to new and pop previously-saved registers.

.globl swtch
swtch:
# 인자로 받은 old와 new를 각각 eax, edx에 저장
  movl 4(%esp), %eax # struct context** old
  movl 8(%esp), %edx # struct context* new

# Save old registers
  # Save old callee-saved registers
  pushl %ebp
  pushl %ebx
  pushl %esi
  pushl %edi

# %esp에는 현재 스택의 top을 가리키고 있음. 그것을 (%eax), 즉 old(context*)에 저장
# 그리고 %edx, 즉 new(context*)의 sp를 %esp(현재 스택 포인터)에 저장
  # Switch stacks
  movl %esp, (%eax)
  movl %edx, %esp

# Load new registers
  # Load new callee-saved registers
  popl %edi
  popl %esi
  popl %ebx
  popl %ebp
  ret
