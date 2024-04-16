int x ;

int main ()
{
    if( x != 0 ){
        x = 20 ;
    }
    asm(" .byte 235 ; .byte 254 ; ");
}
