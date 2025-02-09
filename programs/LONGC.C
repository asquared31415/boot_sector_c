int _ax ;
int _p_i ;
int* _p ;
int* tmp ;

int gs ;
int* ptr ;
int ptr_val ;
void wide_ptr_read (){
  // reads a word at the specified wide pointer
  _ax = gs ;
  // mov gs, ax
  asm(" .byte 142 ; .byte 232 ; ");
  // xchg bx, ax
  // mov ax, word gs:[bx]
  // mov word [_ax], ax
  _ax = ptr ;
  asm(" .byte 147 ; .byte 101 ; .byte 139 ; .byte 7 ; .byte 163 ; .byte 4 ; .byte 16 ; ");
  ptr_val = _ax ;
}
void wide_ptr_write (){
  // write a word at the specified wide pointer
  _ax = gs ;
  // mov gs, ax
  asm(" .byte 142 ; .byte 232 ; ");
  _ax = ptr ;
  // push ax
  asm(" .byte 80 ; ");
  _ax = ptr_val ;
  // pop bx
  // mov word gs:[bx], ax
  asm(" .byte 91 ; .byte 101 ; .byte 137 ; .byte 7 ; ");
}
int ds ;
void get_ds (){
  // gets the value of the ds register, useful for wide pointers
  asm(" .byte 140 ; .byte 30 ; .byte 4 ; .byte 16 ; ");
  ds = _ax ;
}

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

int* buf ;
int c ;
void read_char (){
  // reads a character from the buffer specified in buf
  // and increments buf
  // 0x7000
  gs = 28672 ;
  ptr = buf ;
  wide_ptr_read ();
  c = ptr_val & 255 ;
  _p_i = buf ;
  buf = _p_i + 1 ;
}
void print_char (){
  arg_1 = 1 ;
  arg_2 = c ;
  do_int_41 ();
}

int errno ;

int* fname ;
void read_fname (){
  _p_i = 0 ;
  while( _p_i == 0 ){
    // we read 13 bytes here so that we can reliably detect
    // that the user typed exactly 11 bytes, since we expect
    // the 12th byte to be the null terminator
    // read_line(ds:6200, 13)
    arg_1 = 4 ;
    get_ds ();
    arg_2 = ds ;
    arg_3 = 25088 ;
    arg_4 = 13 ;
    do_int_41 ();
    if( int_ret != 12 ){
      // read the wrong number of bytes, need exactly 11+NULL
      c = 101 ;
      print_char ();
      c = 10 ;
      print_char ();
    }
    if( int_ret == 12 ){
      _p_i = 1 ;
    }
  }

  fname = 25088 ;
}


int* fs_meta ;
void init_source (){
  // open_file(fname)
  arg_1 = 0 ;
  get_ds ();
  arg_2 = ds ;
  arg_3 = fname ;
  do_int_40 ();

  if( int_ret != 0 ){
    errno = 1 ;
    while( 1 == 1 ){
    }
    return;
  }

  // file_info(fs_meta)
  // 0xFFF0
  fs_meta = 65520 ;
  arg_1 = 4 ;
  get_ds ();
  arg_2 = ds ;
  arg_3 = fs_meta ;
  do_int_40 ();
  if( int_ret != 0 ){
    errno = 1 ;
    while( 1 == 1 ){
    }
    return;
  }

  buf = 0 ;
  // read_file (start offset 0, file_len bytes, into 0x7000:0000)
  arg_1 = 2 ;
  arg_2 = 0 ;
  _p = fs_meta + 7 ;
  arg_3 = * _p ;
  arg_4 = 28672 ;
  arg_5 = buf ;
  do_int_40 ();

  if( int_ret != 0 ){
    errno = 1 ;
    while( 1 == 1 ){
    }
    return;
  }

  errno = 0 ;
}


void read_token (){
}

int count ;
int main (){
  read_fname ();
  init_source ();

  if( errno != 0 ){
    _ax = errno ;
    while( 1 == 1 ){
    }
  }

  _p = fs_meta + 7 ;
  count = * _p ;
  while( 0 < count ){
    read_char ();
    // print_char(c)
    arg_1 = 1 ;
    arg_2 = c ;
    do_int_41 ();
    count = count - 1 ;
  }

  while( 1 == 1 ){
  }
}
