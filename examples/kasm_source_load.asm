/* This is a modern multi-line comment that the kasm assembler ignores*/
start:
    mov ax, 10
    mov bx, 0b1010
    mov cx, 0xA
    copy 0xA into al 


    nop ; This is a traditional single-line assemblycomment
halt_cpu:
    hlt  // This is a modern single-line assembly comment
