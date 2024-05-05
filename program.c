int _ax ;
int cond ;
int is_error ;

int* program_addr ;

int port ;
int port_val ;
int inb (){
    _ax = port ;
    asm(" .byte 146 ; .byte 236 ; .byte 162 ; .byte 0 ; .byte 128 ; ");
    port_val = _ax ;
    port_val = port_val & 255 ;
}
int outb (){
    _ax = port ;
    asm(" .byte 146 ; ");
    _ax = port_val ;
    asm(" .byte 238 ; .byte 162 ; .byte 0 ; .byte 128 ; ");
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
    c = port_val ;
    return;
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

int _main_eof ;
int _main_count ;
int main (){
    program_addr = 49152 ;
    _main_eof = 0 ;
    while( _main_eof == 0 ){
        read_char ();
        if( c == 0 ){
            _main_eof = 1 ;
        }
        * program_addr = c ;
        program_addr = program_addr + 1 ;
    }
    program_addr = 49152 ;
    _main_count = 0 ;
    while( _main_count < 100 ){
        c = * program_addr ;
        print_char ();
        program_addr = program_addr + 1 ;
        _main_count = _main_count + 1 ;
    }
    asm(" .byte 235 ; .byte 254 ; ");
}
