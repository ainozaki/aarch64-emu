    .text
    .global main
main:
    stp x29, x30, [sp, -16]!
    mov x2, 11
    adrp    x1, .msg
    mov x29, sp
    add x1, x1, :lo12:.msg
    mov w0, 1
    mov w8, 64
    svc #0
    ldp x29, x30, [sp], 16
    ret
.msg:
    .string "Hello word\n"
