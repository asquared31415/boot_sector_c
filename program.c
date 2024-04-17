int ax ;

int port ;
int port_val ;
int inb (){
    ax = port ;
    asm(" .byte 146 ; .byte 236 ; .byte 162 ; .byte 0 ; .byte 128 ; ");
    port_val = ax ;
}
int outb (){
    ax = port ;
    asm(" .byte 146 ; ");
    ax = port_val ;
    asm(" .byte 238 ; .byte 162 ; .byte 0 ; .byte 128 ; ");
    port_val = ax ;
}
int c ;
int state ;
int read_char (){
    state = 0 ;
    while( state == 0 ){
        port = 1021 ;
        inb ();
        state = port_val & 1 ; 
    }
    port = 1016 ;
    inb ();
    c = ax ;
}
int print_char (){
    state = 0 ;
    while( state == 0 ){
        port = 1021 ;
        inb ();
        state = port_val & 32 ;
    }
    port = 1016 ;
    port_val = c ;
    outb ();
}

int main (){
    while( 1 == 1 ){
        read_char ();
        print_char ();
    }
    asm(" .byte 235 ; .byte 254 ; ");
}
