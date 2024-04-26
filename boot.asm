BITS 16
org 0x7C00 ; BIOS drops us at this address

%include "consts.asm"

main:
    ; segments
    xor ax, ax
    mov ds, ax ; ds is used by lods/stos

    ; TODO: probably need to zero out the ident->memory map

    ; Initialize serial
    mov dx, COM1_PORT + 3
    push dx
    mov al, 0x80
    out dx, al       ; Enable DLAB

    mov dx, COM1_PORT
    mov al, 0x01 ; 115200 baud
    out dx, al

    ; 0x00 baud high byte divisor
    inc dx
    dec ax ; ax contains the 0x01 divisor, dec to 0x00
    out dx, al

    pop dx ; PORT + 3
    mov al, 0x03 ; 8 bits no parity 1 stop
    out dx, al

    dec dx ; PORT + 2
    mov al, 0b0000_0111 ; Enable FIFO and clear the FIFO buffers
    out dx, al

    mov si, PROGRAM_BUFFER ; point to start reading from in the compiler
    mov di, si

    ._load_program:
        mov dx, COM1_PORT + 5
        in al, dx
        and al, 0x01
        jz ._load_program
        mov dx, COM1_PORT
        in al, dx
        stosb
        or al, al
        jnz ._load_program

    ; di contains the current index to write to
    mov di, PROGRAM_MEM_START

    ; fallthrough, returns to end
compiler_entry:
    .loop:
        call next_token ; read the type
        or si, si       ; next_token sets si to 0x0000 if there's no more tokens
        jne .no_eof
        ; all well formed programs end with `}`, which will set gs to 0x1000
        mov ax, word gs:[0x07C8] ; gets main's address
        jmp ax

        .no_eof:
        ; get the name of the variable or function
        call next_token
        ; set up an entry for memory for this declaration
        mov word gs:[bx], di

        ; to distinguish a function and a variable declaration, look for a `()` following it.
        ; functions MUST have those characters while variables MUST have a `;`
        call next_token
        cmp ax, TokenKind.FN_ARGS
        jne .var

        call _block
        ; emit a ret
        mov al, 0xC3
        ; we use the stosw below, so one byte of random garbage gets written after each return

        .var:
        ; just need to create space in the program memory for this variable
        ; there's already an entry in the ident table, so just increment the next memory idx
        stosw ; you get garbage if you read uninit variables :)
        jmp .loop

_asm:
    ._loop:
        call next_token ; eat the .byte or find end
        cmp ax, TokenKind.ASM_END
        je mov_bx_action.end  ; ret
        call next_token ; get the value
        stosb ; write the byte
        call next_token ; eat the ;
        jmp ._loop

_block:
    .stmt_loop:
        call next_token
        cmp ax, TokenKind.CLOSE_BRACE
        jne .not_end
        ret
        .not_end:
        push .stmt_loop
        cmp ax, TokenKind.IF_START
        je _if_state ; returns to stmt_loop
        cmp ax, TokenKind.WHILE_START
        je _while ; returns to stmt_loop
        cmp ax, TokenKind.ASM_START
        je _asm ; returns to stmt_loop

        cmp ax, TokenKind.STAR ; everything that starts with a * is `*ptr = expr`
        jne ._next0

        call next_token ; get the ident to write to
        push cx
        call next_token ; eat the =
        pop cx
        push mov_bx_deref_action
        jmp assign_expr_common ; returns to stmt_loop

        ._next0:
        push cx
        ; if the statement is not an if, while, asm, or `*ptr = expr`, it's either an assignment or a function call
        ; this requires looking at the next token to see if it is `();` or `=`.
        call next_token
        pop cx
        push mov_bx_action
        cmp ax, TokenKind.FN_CALL
        je ._fn

        jmp assign_expr_common ; returns to stmt_loop

        ._fn:
        mov dx, 0xD3FF ; call bx (note: little endian)
        ret ; returns to mov_bx_action, which returns to top of loop

assign_expr_common:
    push cx
    call _expr
    pop cx

    mov dx, 0x0789 ; mov word [bx], ax

    ret ; returns to the correct deref or direct assignment

_while:
    push di ; store the current position to loop to
    call _if_state ; generate the condition and block
    ; bx holds the address of the `if`'s jump target, adjust that forward by 3, which is the size of the jmp
    add word [bx], 3
    pop dx
    sub dx, di
    sub dx, 3 ; size of the JMP rel16off
    mov al, 0xE9
    stosb
    jmp mov_bx_action.shared ; writes dx and returns

_if_state:
    call _expr ; parse an expr for the condition
    ; if the expr is 1, we want to run the following block
    mov ax, 0xC008 ; or al, al (little endian)
    stosw
    mov ax, 0x840F ; start of JE rel16off
    stosw
    push di ; save the location of the jump target to fixup later
    stosw ; skip 2 bytes (we will overwrite them later)

    ; codegen the block
    call _block ; this also eats the } at the end of the statement

    ; fixup jump location
    mov ax, di
    pop bx     ; start of jump target
    sub ax, bx ; jump target is relative to the end of the JE
    dec ax
    dec ax

    mov word [bx], ax
    ret

; emits a sequence
; mov bx, <CONST>
; <2 BYTES>
; cx must hold the constant to write to bx and dx must hold the next 2 bytes to write
; this is a common enough pattern that it's worth it
; clobbers ax
mov_bx_action:
    mov al, 0xBB ; 0xBB encodes `mov bx, ...`
    stosb
    xchg ax, cx ; constant to load into bx
    stosw
    .shared:
    xchg ax, dx ; action to happen after
    stosw
    .end:
    ret

; mov bx, <ADDR>
; mov bx, word [bx]
; <2 BYTES>
; cx must hold the address to load into bx, and dx must hold the final 2 bytes to write
mov_bx_deref_action:
    push dx
    mov dx, 0x1F8B ; mov bx, word [bx]
    call mov_bx_action
    pop dx
    jmp mov_bx_action.shared ; tail call

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

    cmp ax, TokenKind.SEMICOLON
    je mov_bx_action.end

    ; all non-semicolons are considered to be a condition
    ; conditions put a 1 in al if they are met, otherwise put a 0
    xchg bx, ax ; save ax in bx for this call (the call saves it)
    call ._binop_shared

    ; this codegen works by emitting `cmp ax, cx; SETcc al`
    mov ax, 0xC839 ; cmp ax, cx (note: little endian)
    stosw

    mov al, 0x0F ; first byte of SETcc
    stosb

    ; the setCC byte sequence is 0F XX C0, where the XX controls the condition

    ; it turns out that bits 1..=3 of the idents of the comparison operators are unique
    ; and easy to turn into an index
    sub bl, 2
    and bx, 0x0C
    shr bl, 2
    mov al, byte [bx + ._setcc_byte]

    mov ah, 0xC0 ; the last byte of the SETcc
    jmp .end_eat

    ; indexed by some bits in the last nibble of the ident
    ; yes this is cursed
    ._setcc_byte:
        db 0x9E ; setle
        db 0x95 ; setne
        db 0x9C ; setl
        db 0x94 ; sete


    ._binop_eq:
        call ._binop_shared
        mov ax, word [bx + 2]
        .end_eat:
        stosw
        jmp next_token ; eat the ; after a binop (tail call)

    ._arith_binop_codes:
        dw TokenKind.PLUS
            db 0x01, 0xC8 ; add ax, cx
        dw TokenKind.OR
            db 0x09, 0xC8 ; or  ax, cx
        dw TokenKind.AND
            db 0x21, 0xC8 ; and ax, cx
        dw TokenKind.MINUS
            db 0x29, 0xC8 ; sub ax, cx
        dw TokenKind.XOR
            db 0x31, 0xC8 ; xor ax, cx
        dw TokenKind.SHL
            db 0xD3, 0xE0 ; shl ax, cl
        dw TokenKind.SHR
            db 0xD3, 0xE8 ; shr ax, cl

    ; sets up for the binop by getting the LHS into ax and handling the RHS and putting it in cx
    ._binop_shared:
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
        ret

_unary:
    call next_token
    ; some things use this, save space
    mov dx, 0x078B ; mov ax, word [bx] (note: little endian)

    cmp ax, TokenKind.STAR
    je ._star
    cmp ax, TokenKind.AND
    je ._addr_of

    ; something is an ident if it has a non-zero entry in the ident map
    ; otherwise it's considered to be a number
    or cx, cx
    jnz ._ident

    ; mov ax, <CONST>
    ._num:
        xchg cx, ax
        mov dx, 0x9093
        ._ident:
        jmp mov_bx_action ; tail call

    ; mov ax, <ADDR>
    ._addr_of:
        ; get next ident and then use its addr
        call next_token
        xchg ax, cx
        jmp ._ident

    ; mov bx, <ADDR>
    ; mov bx, word [bx]
    ; mov ax, word [bx]
    ._star:
        push dx
        ; get next ident and then use its addr
        call next_token
        pop dx
        jmp mov_bx_deref_action ; tail call

; si must hold the address of the current position in the text
; returns the 16 bit token value in ax, returns the the address allocated for the token in bx, and sets gs appropriately for the access
; increments si to the start of the next token
; if a token was not found, sets si to 0x0000
; clobbers dx
next_token:
    ; al holds the current byte of the program, and ah must be 0 for the 16 bit addition
    xor ax, ax
    ; bx holds the accumulated 16 bit identifier
    xor bx, bx
    ._tokenizer_start:
    lodsb
    cmp al, 0x00 ; if a 0 byte is found at the start of a token, return EoF
    jne .no_end
    xor si, si
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

    mov ax, bx ; store for return

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
    mov cx, word gs:[bx]
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
