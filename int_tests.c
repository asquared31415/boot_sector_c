int _ax ;

int arg_1 ;
int arg_2 ;
int arg_3 ;
int arg_4 ;
int arg_5 ;
int arg_6 ;
int int_ret ;
void do_int_40 (){
  // push ax
  _ax = arg_1 ;
  asm(" .byte 80 ; ");
  _ax = arg_2 ;
  asm(" .byte 80 ; ");
  _ax = arg_3 ;
  asm(" .byte 80 ; ");
  _ax = arg_4 ;
  asm(" .byte 80 ; ");
  _ax = arg_5 ;
  asm(" .byte 80 ; ");
  _ax = arg_6 ;
  asm(" .byte 80 ; ");
  // pop args
  // int 0x40
  // mov word [_ax], ax
  asm(" .byte 95 ; .byte 94 ; .byte 90 ; .byte 89 ; .byte 91 ; .byte 88 ; .byte 205 ; .byte 64 ; ");
  asm(" .byte 163 ; .byte 4 ; .byte 16 ; ");
  int_ret = _ax ;
}
void do_int_41 (){
  // push ax
  _ax = arg_1 ;
  asm(" .byte 80 ; ");
  _ax = arg_2 ;
  asm(" .byte 80 ; ");
  _ax = arg_3 ;
  asm(" .byte 80 ; ");
  _ax = arg_4 ;
  asm(" .byte 80 ; ");
  _ax = arg_5 ;
  asm(" .byte 80 ; ");
  _ax = arg_6 ;
  asm(" .byte 80 ; ");
  // pop args
  // int 0x41
  // mov word [_ax], ax
  asm(" .byte 95 ; .byte 94 ; .byte 90 ; .byte 89 ; .byte 91 ; .byte 88 ; .byte 205 ; .byte 65 ; ");
  asm(" .byte 163 ; .byte 4 ; .byte 16 ; ");
  int_ret = _ax ;
}
int* tmp ;
int main (){
  // open_file (0x3000:0x6200)
  // PROG    C  \0
  tmp = 25088 ;
  * tmp = 21072 ;
  tmp = tmp + 1 ;
  * tmp = 18255 ;
  tmp = tmp + 1 ;
  * tmp = 8224 ;
  tmp = tmp + 1 ;
  * tmp = 8224 ;
  tmp = tmp + 1 ;
  * tmp = 8259 ;
  tmp = tmp + 1 ;
  * tmp = 32 ;
  arg_1 = 0 ;
  arg_2 = 12288 ;
  arg_3 = 25088 ;
  do_int_40 ();

  // read_file (start offset 64, 89 bytes, into 0x3000:8000)
  arg_1 = 2 ;
  arg_2 = 64 ;
  arg_3 = 89 ;
  arg_4 = 12288 ;
  arg_5 = 32768 ;
  do_int_40 ();

  // print what we just read
  arg_1 = 3 ;
  arg_2 = 12288 ;
  arg_3 = 32768 ;
  arg_4 = 89 ;
  do_int_41 ();

  while( 1 == 1 ){
  }
}