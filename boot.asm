BITS 16
org 0x7C00 ; BIOS drops us at this address

%include "consts.asm"

main:
    ; clear ident->addr map
    xor ax, ax
    mov ds, ax ; ds is used by lods
    mov es, ax ; es used by stos
    mov cx, 0x8000 ; write 0x8000 dwords = 0x2_0000 bytes
    push cx
    mov di, 0xFFFF ; |
    inc edi        ; | start pointer is 0x1_0000
    a32 rep stosd
    pop di         ; program buffer for compiler
    mov si, di

    ; Initialize serial
    mov dx, COM1_PORT + 3
    push dx
    mov al, 0x80
    out dx, al       ; Enable DLAB

    mov dl, COM1_PORT & 0xFF
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

    ._load_program:
        mov dl, (COM1_PORT + 5) & 0xFF ; status port
        in al, dx
        and al, 0x01 ; bit 0 is set if there is a character ready
        jz ._load_program
        mov dl, (COM1_PORT & 0xFF) ; data port
        in al, dx
        stosb
        or al, al ; exit when a 0x00 byte is read
        jnz ._load_program

    ; di contains the current index to write to
    mov di, PROGRAM_MEM_START

    ; fallthrough, returns to end
compiler_entry:
    .loop:
        stosb          ; | align to 2 bytes
        and di, 0xFFFE ; | (note: this is 3 bytes, assembler shortens it)
        call next_token
        cmp al, (TokenKind.INT_PTR & 0xFF) ; token can only be int, int*, or void here
        jne ._no_ptr
        stosb ; increment by 1 to mark as int ptr
        ._no_ptr:
        ; get the name of the variable or function
        call next_token
        ; set up an entry for memory for this declaration
        mov word gs:[bx], di

        ; to distinguish a function and a variable declaration, look for a `(){` following it.
        ; functions MUST have those characters while variables MUST have a `;`
        call next_token
        cmp al, (TokenKind.FN_ARGS & 0xFF) ; token can only be a (){ or ;, which have different low bytes
        jne ._var

        call _block
        ; emit a ret
        mov al, 0xC3
        ; we use the stosw below, so one byte of random garbage gets written after each return

        ._var:
        ; just need to create space in the program memory for this variable
        ; there's already an entry in the ident table, so just increment the next memory idx
        stosw ; you get garbage if you read uninit variables :)
        jmp .loop

_asm:
    ._loop:
        call next_token ; eat the .byte or find end
        cmp al, (TokenKind.ASM_END & 0xFF) ; since the token must be .byte or ");, we can only compare one byte
        je _block.stmt_loop
        call next_token ; get the value
        stosb ; write the byte
        call next_token ; eat the ;
        jmp ._loop

_block:
    .stmt_loop:
        call next_token
        cmp ax, TokenKind.CLOSE_BRACE
        je ._ret
        cmp ax, TokenKind.ASM_START
        je _asm ; JUMPS (not return) to stmt_loop
        push .stmt_loop
        cmp ax, TokenKind.COMMENT
        jne ._no_comment
        ._inc:
            lodsb
            cmp al, 0x0A
            jne ._inc
            ._ret:
            ret
        ._no_comment:
        cmp ax, TokenKind.IF_START
        je _if_state ; returns to stmt_loop
        cmp ax, TokenKind.WHILE_START
        je _while ; returns to stmt_loop
        cmp ax, TokenKind.RETURN
        mov dl, 0xC3
        je mov_bx_action.shared ; write C3 XX, returns to stmt_loop

        cmp ax, TokenKind.STAR ; everything that starts with a * is `*ptr = expr`
        jne ._next1

        call next_token ; get the ident to write to
        push cx
        call next_token ; eat the =
        pop cx
        push mov_bx_deref_action
        jmp assign_expr_common ; returns to stmt_loop

        ._next1:
        push mov_bx_action
        push cx ; save the used ident/function
        ; if the statement is not an if, while, asm, return, or `*ptr = expr`, it's either an assignment or a function call
        ; this requires looking at the next token to see if it is `();` or `=`.
        call next_token
        pop cx
        cmp ax, TokenKind.FN_CALL
        jne assign_expr_common ; returns to stmt_loop

        mov dx, 0xD3FF ; call bx (note: little endian)
        ret ; returns to mov_bx_action, which returns to top of loop

; cx contains the addr to write to on call
assign_expr_common:
    push cx
    call _expr
    pop cx
    mov dx, 0x0789 ; mov word [bx], ax
    ret ; returns to the correct deref or direct assignment

_while:
    push di ; store the current position to loop to
    call _if_state ; generate the condition and block
    ; bx-2 holds the address of the `if`'s jump target, adjust that forward by 3, which is the size of the loop jmp
    add word [bx - 2], 3
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
    stosw ; skip 2 bytes (we will overwrite them later)
    push di ; save the location of the jump target +2 to fixup later

    ; codegen the block
    call _block ; this also eats the } at the end of the statement

    ; fixup jump location
    mov ax, di
    pop bx     ; end of JE addr
    sub ax, bx ; jump target is relative to the end of the JE
    mov word [bx - 2], ax ; target is 2 bytes back
    ret

; emits a sequence
; mov bx, <CONST>
; <2 BYTES>
; cx must hold the constant to write to bx and dx must hold the next 2 bytes to write
; this is a common enough pattern that it's worth it
; clobbers ax, exchanges cx and dx
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

_unary:
    call next_token
    mov bp, cx               ; store the ident value in bp (zero for integers)
    ; some things use this, save space
    mov dx, 0x078B ; mov ax, word [bx] (note: little endian)

    ;; parens are a unary expression and parse a sub-expression
    cmp ax, TokenKind.OPEN_PAREN
    jne ._no_paren
    jmp _expr                  ; eats the close paren (tail call)

    ._no_paren:
    cmp ax, TokenKind.STAR
    je ._star

    ; something is an ident if it has a non-zero entry in the ident map
    ; otherwise it's considered to be a number
    jcxz ._num
    ._ident: jmp mov_bx_action ; tail call

    ; mov bx, <CONST>
    ; xchg ax, bx
    ; nop
    ._num:
        xchg cx, ax ; move the constant value into cx to load into program bx
        mov dx, 0x9093 ; xchg ax, bx; nop
        jmp mov_bx_action ; tail call

    ; mov bx, <ADDR>
    ; mov bx, word [bx]
    ; mov ax, word [bx]
    ._star:
        ; get next ident and then use its addr
        call next_token
        jmp mov_bx_deref_action ; tail call

; emits an expression
; expressions return their value in ax
; binary expressions use ax for the LHS and cx for the RHS
_expr:
    call _unary
    call next_token ; get the binop if it exists
    cmp ax, TokenKind.SEMICOLON
    je mov_bx_action.end

    ; codegen the RHS unary and get the LHS in ax and RHS in cx
    push bp ; the addr of the ident in the program memory
    push ax ; store the binop ident
    ; the previous unary codegen put something in ax already, so save it, emit another unary and then restore it
    mov al, 0x50 ; push ax
    stosb
    call _unary  ; now the first value is on the stack, and the second is in ax
    mov ax, 0x9159 ; pop ax; xchg ax, cx
    stosw

    pop ax
    pop dx ; move addr to dx

    ; NOTE: at this point, if it's not a semicolon, the token must be a binop token
    ; the binop tokens all have unique low bytes, so only that is compared
    mov bx, ._arith_binop_codes - 3
    ._binop_loop:
        add bl, 3
        cmp al, byte [bx]
        jne ._binop_loop

    ._binop_eq:
        cmp bl, (._binop_cmp_start - $$) & 0xFF ; & is not allowed on non-scalars, relative to 0x7C00 is the same though
        pushf ; we need this condition later too
        jb ._no_cond
        mov ax, 0xC839 ; cmp ax, cx (note: little endian)
        stosw
        mov al, 0x0F ; first byte of SETcc
        stosb
        ._no_cond:
        mov ax, word [bx + 1]
        stosw
        popf         ; restore flags from comparison against binop status
        jnb ._no_ptr ; no need to adjust if the binop is a condition
        rcr dl, 1 ; special handling for lhs pointers
        jnc ._no_ptr

        ; for ptr + int, emit another add ax, cx
        ; NOTE: ptr + <non int> does not exist, case ignored
        ; for ptr - int, emit another sub ax, cx
        ; for ptr - ptr, emit a shr ax, 1 on the final result
        ; for other ops, no adjustment is needed

        rcr bp, 1 ; if the RHS is also a pointer, it's ptr - ptr
        jnc ._emit_end ; ptr <op> int, emit the operation twice
        mov ax, 0xE8D1 ; for ptr - ptr, emit a shr ax, 1 instead
        ._emit_end:
        stosw
        ._no_ptr:
        jmp next_token ; eat the ; after a binop (tail call)

    ; NOTE: truncation to low byte is intentional, see note above
    ._arith_binop_codes:
        db TokenKind.PLUS & 0xFF
            db 0x01, 0xC8 ; add ax, cx
        db TokenKind.MINUS & 0xFF
            db 0x29, 0xC8 ; sub ax, cx
        db TokenKind.OR & 0xFF
            db 0x09, 0xC8 ; or  ax, cx
        db TokenKind.AND & 0xFF
            db 0x21, 0xC8 ; and ax, cx
        db TokenKind.SHL & 0xFF
            db 0xD3, 0xE0 ; shl ax, cl
        db TokenKind.SHR & 0xFF
            db 0xD3, 0xE8 ; shr ax, cl
        ._binop_cmp_start:
        db TokenKind.EQUAL_EQUAL & 0xFF
            db 0x94, 0xC0 ; sete last two bytes
        db TokenKind.NOT_EQUAL & 0xFF
            db 0x95, 0xC0 ; setne last two bytes
        db TokenKind.LESS & 0xFF
            db 0x9C, 0xC0 ; setl last two bytes


; si must hold the address of the current position in the text
; returns the 16 bit token value in ax and the entry in the ident map in cx
; increments si to the start of the next token
; if a token was not found, jumps to main
; clobbers bx
next_token:
    ; al holds the current byte of the program, and ah must be 0 for the 16 bit addition
    xor ax, ax
    ; bx holds the accumulated 16 bit identifier
    xor bx, bx
    ._tokenizer_start:
    lodsb
    cmp al, 0x00 ; if a 0 byte is found at the start of a token, return EoF
    jne .no_end
    ; all well formed programs end with `}`, which will set gs to 0x1000
    mov ax, word gs:[0x07C8] ; gets main's address
    jmp ax

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
    xor cx, cx   ; clears the carry to ensure a 0 gets rotated in and makes sure
                 ; that cx is 0 if the high bit of bx is 0
    rcl bx, 1    ; rotate the high bit of bx into the carry register
    rcr ch, 4    ; cx holds 0x0000 or 0x1000
    add ch, 0x10 ; 0x1000 or 0x2000
    mov gs, cx
    ; we don't restore bx here, because we need to multiply by 2 anyway, the rcl did that
    ; really this whole thing just forms a 17 bit address with a constant offset
    ; of 0x1_0000, if you think about it
MAGIC_SIGNATURE:
    mov cx, word gs:[bx]
    ret

TIMES 0x1BE-($-$$) db 0x00

; ====================================================================
; PARTITION TABLE ABSTRACTIONS (see next section for actual tables)
; ====================================================================

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

%macro PART 3
    %%.attributes: db %1
    %%.CHS_start: LBA_TO_CHS_BYTES %2
    %%.system_id: db 0x00
    %%.CHS_end: LBA_TO_CHS_BYTES %eval(%2 + %3)
    %%.LBA_START: dd %2
    %%.LBA_COUNT: dd %3
%endmacro

; ====================================================================
; PARTITION TABLES
; ====================================================================
; NOTE: the first sector of the device does not need to be within a partition

; FAT16 partition
part0: PART 0x00,0x01,0x3FFF

part1: TIMES 16 db 0x00
part2: TIMES 16 db 0x00
part3: TIMES 16 db 0x00

; Partition table ends just before boot signature
dw 0xAA55
