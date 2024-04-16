int* x ;

int meow (){
    x = 49152 ;
}

int main (){
    meow ();
    * x = 42 ;
    asm(" .byte 235 ; .byte 254 ; ");
}
