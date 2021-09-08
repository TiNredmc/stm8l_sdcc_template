.module UTIL
.globl _fast_memcpy
.area CODE
_fast_memcpy:
    ldw x, (0x03, sp) ; dest
    ldw y, (0x05, sp) ; src
loop0$:
    tnz (0x07, sp)    ; if (len == 0)
    jreq loop0_end$
    ld a, (y)         ; loop body
    ld (x), a
    incw x
    incw y
    dec (0x07, sp)    ; len--
    jra loop0$
loop0_end$:
    ret