BITS 16
org 0x7C00 ; BIOS drops us at this address

%include "consts.asm"

main:
    ; segments
    xor ax, ax
    mov ds, ax ; ds is used by lods/stos
    mov ss, ax
    mov sp, STACK_TOP

    ; di always contains the current index to write to
    mov di, PROGRAM_MEM_START

    ; TODO: probably need to zero out the ident->memory map

    ; Initialize serial
    ; mov dx, COM1_PORT + 3
    ; push dx
    ; mov al, 0x80
    ; out dx, al       ; Enable DLAB

    ; mov dx, COM1_PORT
    ; mov al, 0x01 ; 115200 baud
    ; out dx, al

    ; ; 0x00 baud high byte divisor
    ; inc dx
    ; dec ax ; ax contains the 0x01 divisor, dec to 0x00
    ; out dx, al

    ; pop dx ; PORT + 3
    ; mov al, 0x03 ; 8 bits no parity 1 stop
    ; out dx, al

    ; dec dx ; PORT + 2
    ; mov al, 0b0000_0111 ; Enable FIFO and clear the FIFO buffers
    ; out dx, al

    mov si, PROGRAM
    call compiler_entry
    call 0x8002

    jmp $

CompilerState:
    .declaration: equ 1 << 0
    .in_procedure: equ 1 << 1

db "COMP_START"

compiler_entry:
    .loop:
        call next_token ; read the type
        or si, si       ; next_token sets si to 0x0000 if there's no more tokens
        jne .no_eof
        ret

        .no_eof:
        ; get the name of the variable or function
        call next_token
        ; set up an entry for memory for this declaration
        call _get_token_addr
        mov word gs:[bx], di

        push ax ; save the ident for the next step

        ; to distinguish a function and a variable declaration, look for a `()` following it.
        ; functions MUST have those characters while variables MUST have a `;`
        call next_token
        pop cx                    ; get the ident name for handling
        cmp ax, TokenKind.FN_ARGS
        jne .var
        
        call _fn_decl
        jmp .loop

        .var:
        ; just need to create space in the program memory for this variable
        ; there's already an entry in the ident table, so just increment the next memory idx
        add di, VAR_SIZE
        jmp .loop

_fn_decl:
    ; TODO: fn declaration
    call next_token ; skip over the " {" that should be there

        ; get the start of the next statement
    .stmt_loop:
        call next_token
        cmp ax, TokenKind.CLOSE_BRACE
        je .end
        cmp ax, TokenKind.IF_START
        je _if_state
        cmp ax, TokenKind.WHILE_START
        je _while_state
        ; if the statement is not an if or a while, it's either an assignment or a function call
        ; this requires looking at the next token to see if it is `();` or `=`.
        mov cx, ax
        call next_token
        cmp ax, TokenKind.FN_CALL
        push .stmt_loop  ; these return into the top of the loop
        je _fn_call      ;
        jmp _assign_expr ;

    .end:
    ; emit a ret
    mov al, 0xC3
    stosb
    ret

_if_state:
_while_state:
    jmp $

; cx holds the ident to assign to
_assign_expr:
    push cx
    call _expr ; NOTE: this will eat the ; if it exists
    pop cx

    ; codegen the store
    mov dx, 0x0789 ; mov [bx], ax (note: little endian)
    jmp _fn_call.shared


; cx holds the ident of the function to call
_fn_call:
    mov dx, 0x17FF ; call [bx] (note: little endian)

    .shared: ; this code is shared with the assignment code, with the `call` replaced with a `mov`
    push dx
    mov bx, cx           ; | get the entry for the memory allocated for the ident
    call _get_token_addr ; | that is being called
    mov cx, word gs:[bx] ; | cx holds the final address of the ident being called

    ; here we want to emit a sequence:
    ; mov bx, <const>
    ; call [bx]
    mov al, 0xBB ; 0xBB encodes `mov bx, ...`
    stosb
    mov ax, cx ; load the constant to call/write to
    stosw
    pop ax ; write the action
    stosw

    ret

; emits an expression
; expressions return their value in ax
; binary expressions use ax for the LHS and cx for the RHS
_expr:
    call _unary

    call next_token
    mov bx, ._arith_binop_codes
    mov cx, 7
    ._binop_loop:
        cmp ax, word [bx]
        je ._binop_eq
        add bx, 4
        loop ._binop_loop

    ; if it wasn't a binop, the next_token ate the token, which would be the ;
    ret

    ._binop_eq:
        ; db "MEOW"
        ; the previous unary codegen put something in ax already, so save it
        ; to cx, emit another unary and then swap back
        mov al, 0x91 ; xchg cx, ax
        stosb

        push ax ; we need another xchg after this
        push bx ; save the index into the binop codes
        call _unary ; now the first value is in cx, and the second is in ax
        pop bx

        pop ax ; xchg cx, ax
        stosb

        inc bx ; |
        inc bx ; | advance to the bytes to emit
        mov ax, word [bx]
        stosw

        call next_token ; eat the ; after a binop
        ret

    ._arith_binop_codes:
        dw TokenKind.PLUS 
            db 0x01, 0xC8 ; add ax, cx
        dw TokenKind.MINUS
            db 0x29, 0xC8 ; sub ax, cx
        dw TokenKind.AND
            db 0x21, 0xC8 ; and ax, cx
        dw TokenKind.OR
            db 0x09, 0xC8 ; or  ax, cx
        dw TokenKind.XOR
            db 0x31, 0xC8 ; xor ax, cx
        dw TokenKind.SHL
            db 0xD3, 0xE0 ; shl ax, cl
        dw TokenKind.SHR
            db 0xD3, 0xE8 ; shr ax, cl

_unary:
    call next_token

    cmp ax, TokenKind.STAR
    je ._star
    cmp ax, TokenKind.AND
    je ._addr_of

    ; TODO: paren expr

    ; something is an ident if it has a non-zero entry in the ident map
    ; otherwise it's considered to be a number
    call _get_token_addr
    mov cx, word gs:[bx]
    or cx, cx
    jz ._num

    ; mov bx, <ADDR>
    ; mov ax, word [bx]
    ._ident:
        mov al, 0xBB ; mov bx, imm16
        stosb
        mov ax, cx ; addr of the variable
        stosw
        mov ax, 0x078B ; mov ax, word [bx] (note: little endian)
        stosw
        ret

    ._num:
        push ax
        mov al, 0xB8 ; mov ax, imm16
        stosb
        pop ax ; the constant
        stosw
        ret

    ; get next ident and then use its addr
    ._star:
        call next_token
        call _get_token_addr

        ; mov bx, <ADDR>
        ; mov bx, word [bx]
        ; mov ax, word [bx]
        mov al, 0xBB ; mov bx, imm16
        stosb
        mov ax, word gs:[bx] ; addr of the variable
        stosw
        mov ax, 0x1F8B ; mov bx, word [bx] (note: little endian)
        stosw
        mov ax, 0x078B ; mov ax, word [bx] (note: little endian)
        stosw
        ret

    ; get next ident and then use its addr
    ._addr_of:
        call next_token
        call _get_token_addr
        mov cx, word gs:[bx]

        mov al, 0xB8 ; mov ax, imm16
        stosb
        mov ax, cx ; addr of the variable
        stosw
        ret

; token in bx
; returns the address allocated for the token in bx, and sets gs appropriately for the access
; clobbers dx
_get_token_addr:
    ; set gs to correctly address the highest nibble of the index table
    ; this is either 0x1000 for the low half, or 0x2000 for the high half
    clc        ; make sure a 0 gets rotated in
    rcl bx, 1  ; rotate the high bit of bx into the carry register
    setc dl    ; |
    inc dl     ; | dl holds 2 if the high half is needed, 1 otherwise
    shl dx, 12 ; turn the 1 or 2 in dl into 0x1000 or 0x2000 in dx
    mov gs, dx
    ; we don't restore bx here, because we need to multiply by 2 anyway, the rcl did that
    ; really this whole thing just forms a 17 bit address with a constant offset
    ; of 0x1_0000, if you think about it
    ret


; si must hold the address of the current position in the text
; returns the 16 bit token value in ax AND bx (for indexing), increments si to the end of the token
; if a token was not found, sets si to 0x0000
next_token:
    ; al holds the current byte of the program, and ah must be 0 for the 16 bit addition
    xor ax, ax
    ; bx holds the accumulated 16 bit identifier
    xor bx, bx
    ._tokenizer_start:
    lodsb
    cmp al, 0x00 ; if a 0 byte is found at the start of a token, return EoF
    jne .no_end
    mov si, 0x0000
    ret

    .no_end:
    ; at the beginning of a token, skip anything up to and including space
    ; as that's various whitespace and non-printing characters
    cmp al, " "
    jbe ._tokenizer_start
    
    ._tokenizer_add_char:
        imul bx, bx, 10
        add bx, ax  ; | add the character in a way that ASCII numbers end up working out
        sub bx, '0' ; | to be their numerical value

        lodsb
        ; at this point, if a space or lower is encountered, end
        cmp al, " "
        ja ._tokenizer_add_char
    
    mov ax, bx
    ret

; print_char:
;     push ax
; .wait:
;     mov dx, COM1_PORT + 5
;     in al, dx
;     and al, 0x20
;     jz .wait
;     mov dx, COM1_PORT
;     pop ax
;     out dx, al
;     ret

PROGRAM: INCBIN "program.c"
db 0


TIMES 0x1BE-($-$$) db 0x00

; Set up partition table
; This is because the hardware in use needs this to dectect as a "USB key"

; Starts at the first sector, is 8 sectors long, allowing 7 sectors of user data (0xE00 bytes).
%assign PART0_LBA_START 0
%assign PART0_LBA_SIZE  8

; ==============================================================================
; actual impl here, but above this is the Abstraction that's all nice and fuzzy
; ==============================================================================

; Geometry of the target disk.
; The USB that is in use has 1015 cylinders, 124 heads, and 62 sectors and contains 7810176 sectors total.
%define NUM_CYLINDERS 1015
%define NUM_HEADS      124
%define NUM_SECTORS     62

; Converts a LBA passed as an integer to a sequence of 3 CHS bytes in BIOS format.
%macro LBA_TO_CHS_BYTES 1
    ; Head number.
    db (%1 / NUM_SECTORS) % NUM_HEADS
    ; Cylinder is split across the next two bytes.
    %assign %%cylinder_val %1 / (NUM_HEADS * NUM_SECTORS)

    ; High 2 bits of cylinder and all 6 bits of sector.
    db (%%cylinder_val >> 2) | ((%1 % NUM_SECTORS) + 1)

    ; Truncate to get low 8 bits of cylinder.
    db %%cylinder_val
%endmacro

part0:
.attributes: db 0x80 ; Active boot partition.
.CHS_start:  LBA_TO_CHS_BYTES PART0_LBA_START
.system_id:  db 0x00
.CHS_end:    LBA_TO_CHS_BYTES PART0_LBA_START + PART0_LBA_SIZE
.LBA_start:  dd PART0_LBA_START
.LBA_count:  dd PART0_LBA_SIZE

; Starts after the boot partition, end at the end of the first cylinder, sector 7688.
%assign PART1_LBA_START 8
%assign PART1_LBA_SIZE  7680

part1:
.attributes: db 0x00 ; Non-active
.CHS_start:  LBA_TO_CHS_BYTES PART1_LBA_START
.system_id:  db 0x00
.CHS_end:    LBA_TO_CHS_BYTES PART1_LBA_START + PART1_LBA_SIZE
.LBA_start:  dd PART1_LBA_START
.LBA_count:  dd PART1_LBA_SIZE

part2: TIMES 16 db 0x00
part3: TIMES 16 db 0x00

; Partition table ends just before boot signature
dw 0xAA55
