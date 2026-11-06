/* This is a modern multi-line comment that the kasm assembler ignores*/
start:
    mov ax, 10
    mov bx, 0b1010
    mov cx, 0xA
    mov dx, 0o12
    mov ax, 1010b
    mov bx, 0Ah

    nop ; This is a traditional single-line assemblycomment
halt_cpu:
    hlt  // This is a modern single-line assembly comment
