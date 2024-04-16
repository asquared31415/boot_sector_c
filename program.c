int x ;

int main ()
{
    while( x != 0 ){
        x = 20 ;
    }
    asm(" .byte 235 ; .byte 254 ; ");
}
