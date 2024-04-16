%ifndef CONSTS
    %define CONSTS

    ; SP starts here, and grows downwards towards 0x0000
    STACK_TOP: equ 0x7C00

    COM1_PORT: equ 0x03F8 ; COM1 port

    VAR_SIZE: equ 2

    ; the location where the boot sector loads the program into
    PROGRAM_BUFFER: equ 0x6000

    PROGRAM_MEM_START: equ 0x8000

    ; TOKENS
    TokenKind:
        .INT:         equ 0x18F4 ; int
        .INT_PTR:     equ 0xF982 ; int*

        .OPEN_PAREN:  equ 0xFFF8 ; (
        .CLOSE_PAREN: equ 0xFFF9 ; )
        .CLOSE_BRACE: equ 0x004D ; }
        .FN_ARGS:     equ 0xFFA9 ; ()
        .FN_CALL:     equ 0xFCA5 ; ();

        .SEMICOLON:   equ 0x000B ; ;

        .IF_START:    equ 0x1858 ; if(
        .WHILE_START: equ 0xDA02 ; while(

        .ASM_START:   equ 0x973E ; asm("
        .DOT_BYTE:    equ 0x9491 ; .byte
        .ASM_END:     equ 0xFA4D ; ");

        .STAR:        equ 0xFFFA ; *
        .AND:         equ 0xFFF6 ; &

        .PLUS:        equ 0xFFFB
        .MINUS:       equ 0xFFFD
        .OR:          equ 0x004C
        .XOR:         equ 0x002E
        .SHL:         equ 0x0084
        .SHR:         equ 0x009A
        .EQUAL_EQUAL: equ 0x008F
        .NOT_EQUAL:   equ 0xFF77
        .LESS:        equ 0x000C
        .LESS_EQUAL:  equ 0x0085

%endif