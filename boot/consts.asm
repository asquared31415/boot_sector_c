%ifndef CONSTS
    %define CONSTS

    COM1_PORT: equ 0x03F8 ; COM1 port

    PROGRAM_MEM_START: equ 0x1000

    ; TOKENS
    TokenKind:
        .COMMENT:     equ 0xFFF5 ; //

        .INT:         equ 0x18F4 ; int
        .INT_PTR:     equ 0xF982 ; int*
        .VOID:        equ 0x2C7A ; void

        .OPEN_PAREN:  equ 0xFFF8 ; (
        .CLOSE_PAREN: equ 0xFFF9 ; )
        .CLOSE_BRACE: equ 0x004D ; }
        .FN_ARGS:     equ 0xFCE5 ; (){
        .FN_CALL:     equ 0xFCA5 ; ();

        .SEMICOLON:   equ 0x000B ; ;

        .IF_START:    equ 0x1858 ; if(
        .WHILE_START: equ 0xDA02 ; while(

        .ASM_START:   equ 0x973E ; asm("
        .DOT_BYTE:    equ 0x9491 ; .byte
        .ASM_END:     equ 0xFA4D ; ");

        .MAIN_IDENT:  equ 0x03E4 ; main

        .STAR:        equ 0xFFFA ; *
        .AND:         equ 0xFFF6 ; &
        .RETURN:      equ 0x7DA7 ; return;

        .PLUS:        equ 0xFFFB ; +
        .MINUS:       equ 0xFFFD ; -
        .OR:          equ 0x004C ; |
        .XOR:         equ 0x002E ; ^
        .SHL:         equ 0x0084 ; <<
        .SHR:         equ 0x009A ; >>
        .EQUAL_EQUAL: equ 0x008F ; ==
        .NOT_EQUAL:   equ 0xFF77 ; !=
        .LESS:        equ 0x000C ; <
        .LESS_EQUAL:  equ 0x0085 ; <=

%endif
