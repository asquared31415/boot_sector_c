int _ax ;

int* local_stack ;
int local_idx ;
int local_val ;

void push_local (){
  // ARGUMENTS: <GLOBAL> local_val
  // RETURNS: NONE
  * local_stack = local_val ;
  local_stack = local_stack + 1 ;
}

void pop_local (){
  // ARGUMENTS: NONE
  // RETURNS: <GLOBAL> local_val: the value of the local at the top of the stack
  local_stack = local_stack + 65535 ;
  local_val = * local_stack ;
}

int* _local_ptr ;
void read_local (){
  // ARGUMENTS: <GLOBAL> local_idx
  // RETURNS: <GLOBAL> local_val: the value of the local
  // local_stack points to the NEXT pointer, so offset for that
  _local_ptr = ( local_stack - local_idx ) - 1 ;
  local_val = * _local_ptr ;
}

void write_local (){
  // ARGUMENTS:
  // - <GLOBAL> local_idx
  // - <GLOBAL> local_val
  // RETURNS: NONE
  // local_stack points to the NEXT pointer, so offset for that
  _local_ptr = ( local_stack - local_idx ) - 1 ;
  * _local_ptr = local_val ;
}

int _p_i ;
int* _p ;
int gs ;
int* ptr ;
int ptr_val ;
void wide_ptr_read (){
  // reads a word at the specified wide pointer
  // used to access outside the 16 bit pointer range
  // ARGUMENTS:
  // <GLOBAL> gs - the value to use for gs in the read
  // <GLOBAL> ptr - the pointer to use for the offset into the segment for the read
  // RETURNS: <GLOBAL> ptr_val - the read value
  _ax = gs ;
  // mov gs, ax
  asm(" .byte 142 ; .byte 232 ; ");
  // xchg bx, ax
  // mov ax, word gs:[bx]
  // mov word [0x1000], ax
  _ax = ptr ;
  asm(" .byte 147 ; .byte 101 ; .byte 139 ; .byte 7 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  ptr_val = _ax ;
}

void wide_ptr_write (){
  // write a word at the specified wide pointer
  // used to access outside the 16 bit pointer range
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

int* tmp ;

int* memset_ptr ;
int memset_val ;
int memset_count ;

int* _memset_ptr_end ;
void memset (){
  // sets memset_count *WORDS* of data beginning at memset_ptr to the *WORD* specified in memset_val
  _memset_ptr_end = memset_ptr + memset_count ;
  while( memset_ptr < _memset_ptr_end ){
    * memset_ptr = memset_val ;
    memset_ptr = memset_ptr + 1 ;
  }
}

int* memcpy_src ;
int* memcpy_dst ;
int memcpy_count ;
int* _memcpy_end ;
void memcpy (){
  // copies memcpy_count WORDS of data from memcpy_src to memcpy_dst
  // the regions of memory must not overlap
  _memcpy_end = memcpy_src + memcpy_count ;
  while( memcpy_src < _memcpy_end ){
    * memcpy_dst = * memcpy_src ;
    memcpy_src = memcpy_src + 1 ;
    memcpy_dst = memcpy_dst + 1 ;
  }
}

void memcpy_gs_dst (){
  // copies memcpy_count WORDS of data from memcpy_src to gs:memcpy_dst
  // the regions of memory must not overlap
  _memcpy_end = memcpy_src + memcpy_count ;
  while( memcpy_src < _memcpy_end ){
    ptr = memcpy_dst ;
    ptr_val = * memcpy_src ;
    wide_ptr_write ();
    memcpy_src = memcpy_src + 1 ;
    memcpy_dst = memcpy_dst + 1 ;
  }
}

void memcpy_gs_src (){
  // copies memcpy_count WORDS of data from gs:memcpy_src to memcpy_dst
  // the regions of memory must not overlap
  _memcpy_end = memcpy_src + memcpy_count ;
  while( memcpy_src < _memcpy_end ){
    ptr = memcpy_src ;
    wide_ptr_read ();
    * memcpy_dst = ptr_val ;
    memcpy_src = memcpy_src + 1 ;
    memcpy_dst = memcpy_dst + 1 ;
  }
}

int strcmp_lhs ;
int strcmp_rhs ;
int strcmp_diff ;

int* _strcmp_lhs_ptr ;
int* _strcmp_rhs_ptr ;
void strcmp (){
  // compares two null terminated strings lexicographically
  // ARGUMENTS:
  // <global> strcmp_lhs
  // <global> strcmp_rhs
  // RETURNS: <global> strcmp_diff - an integer which has a sign corresponding to the result of the comparison
  //           - negative if lhs is before rhs
  //           - zero if lhs and rhs are equal
  //           - positive if lhs is after rhs

  _strcmp_lhs_ptr = strcmp_lhs ;
  _strcmp_rhs_ptr = strcmp_rhs ;
  strcmp_diff = ( * _strcmp_lhs_ptr - * _strcmp_rhs_ptr ) & 255 ;
  // if the strings are different, this loop will exit
  //  - if the strings differ in a normal character, the difference will be non-zero, and the loop will exit
  //  - if one of the strings is a prefix of another ("abc", "abcd"), then the null terminator of one will be different from a character in another
  // if the strings are identical, we need to make sure that we exit when both bytes are null terminators, or else this would read out of bounds
  while( strcmp_diff == 0 ){
    // handling identical strings
    // reusing this variable as temp storage
    strcmp_diff = * _strcmp_lhs_ptr & 255 ;
    if( strcmp_diff == 0 ){
      strcmp_diff = * _strcmp_rhs_ptr & 255 ;
      if( strcmp_diff == 0 ){
        // both were null terminators, return (0 is already in the return global)
        return;
      }
    }

    // read the next byte
    strcmp_lhs = strcmp_lhs + 1 ;
    strcmp_rhs = strcmp_rhs + 1 ;
    _strcmp_lhs_ptr = strcmp_lhs ;
    _strcmp_rhs_ptr = strcmp_rhs ;
    strcmp_diff = ( * _strcmp_lhs_ptr - * _strcmp_rhs_ptr ) & 255 ;
  }
}

int port ;
int port_val ;
void inb (){
  _ax = port ;
  // xchg ax, dx; in al, dx; move byte [0x1000], al
  asm(" .byte 146 ; .byte 236 ; .byte 162 ; .byte 0 ; .byte 16 ; ");
  port_val = _ax & 255 ;
}
void outb (){
  _ax = port ;
  // xchg ax, dx
  asm(" .byte 146 ; ");
  _ax = port_val ;
  // out dx, al
  asm(" .byte 238 ; ");
}
int c ;
int state ;
void read_char (){
  state = 0 ;
  while( state == 0 ){
    // 0x3FD - status
    port = 1021 ;
    inb ();
    state = port_val & 1 ;
  }
  // 0x3FD - data
  port = 1016 ;
  inb ();
  c = port_val ;
}
void print_char (){
  state = 0 ;
  while( state == 0 ){
    // 0x3FD - status
    port = 1021 ;
    inb ();
    state = port_val & 32 ;
  }
  // 0x3FD - data
  port = 1016 ;
  port_val = c ;
  outb ();
}

int int_div_lhs ;
int int_div_rhs ;
int int_div_quot ;
int int_div_rem ;
void int_div (){
  int_div_quot = 0 ;
  while( int_div_rhs < ( int_div_lhs + 1 ) ){
    int_div_lhs = int_div_lhs - int_div_rhs ;
    int_div_quot = int_div_quot + 1 ;
  }
  int_div_rem = int_div_lhs ;
}

int print_hex_val ;
int _print_hex_nibble ;
int _print_hex_count ;
void print_hex (){
  // prints a value to the serial output, formatted as hex without a leading 0x
  // ARGUMENTS: <global> print_hex_val - the value to print
  // RETURNS: NONE
  _print_hex_count = 0 ;
  while( _print_hex_count < 4 ){
    // 0xF000
    _print_hex_nibble = ( print_hex_val & 61440 ) >> 12 ;
    // adjust letters forward into the letter region
    if( 9 < _print_hex_nibble ){
      _print_hex_nibble = _print_hex_nibble + 7 ;
    }
    // shift from value into digit range
    c = _print_hex_nibble + 48 ;
    print_char ();
    print_hex_val = print_hex_val << 4 ;
    _print_hex_count = _print_hex_count + 1 ;
  }
}

void println_hex (){
  print_hex ();
  c = 10 ;
  print_char ();
}

int print_dec_val ;
int _print_dec_c ;
int _seen_zero ;
void print_dec (){
  if( print_dec_val < 0 ){
    // -
    c = 45 ;
    print_char ();
    print_dec_val = 0 - print_dec_val ;
  }
  _print_dec_c = 0 ;
  while( print_dec_val != 0 ){
    int_div_lhs = print_dec_val ;
    int_div_rhs = 10 ;
    int_div ();
    local_val = int_div_rem ;
    push_local ();
    print_dec_val = int_div_quot ;
    _print_dec_c = _print_dec_c + 1 ;
  }

  while( 0 < _print_dec_c ){
    pop_local ();
    c = local_val + 48 ;
    print_char ();
    _print_dec_c = _print_dec_c - 1 ;
  }
}

void print_esc (){
  // ESC
  c = 27 ;
  print_char ();
  // [
  c = 91 ;
  print_char ();
}

int cursor_x ;
int cursor_y ;
void set_cursor_pos (){
  print_esc ();
  print_dec_val = cursor_y ;
  print_dec ();
  // ;
  c = 59 ;
  print_char ();
  print_dec_val = cursor_x ;
  print_dec ();
  // H
  c = 72 ;
  print_char ();
}

void clear (){
  print_esc ();
  // 2
  c = 50 ;
  print_char ();
  // J
  c = 74 ;
  print_char ();
  cursor_x = 1 ;
  cursor_y = 1 ;
  set_cursor_pos ();
}

int print_str_seg ;
int* print_str_ptr ;
int _print_str_b ;
void print_string (){
  // prints a NULL terminated string to the serial output
  // ARGUMENTS: <global> print_string_seg:print_str_ptr - pointer to the start of the string to print
  // RETURNS: NONE

  gs = print_str_seg ;
  ptr = print_str_ptr ;
  wide_ptr_read ();
  _print_str_b = ptr_val & 255 ;
  while( _print_str_b != 0 ){
    c = _print_str_b ;
    print_char ();
    _p_i = print_str_ptr ;
    print_str_ptr = _p_i + 1 ;
    ptr = print_str_ptr ;
    wide_ptr_read ();
    _print_str_b = ptr_val & 255 ;
  }
}

int print_str_len ;
void print_string_len (){
  gs = print_str_seg ;
  while( 0 < print_str_len ){
    ptr = print_str_ptr ;
    wide_ptr_read ();
    c = ptr_val & 255 ;
    print_char ();
    _p_i = print_str_ptr ;
    print_str_ptr = _p_i + 1 ;
    ptr = print_str_ptr ;
    wide_ptr_read ();
    _print_str_b = ptr_val & 255 ;
    print_str_len = print_str_len - 1 ;
  }
}

int read_line_seg ;
int* read_line_ptr ;
int read_line_max ;
int read_line_len ;
void read_stdin (){
  read_line_len = 0 ;
  gs = read_line_seg ;
  // save room for null terminator
  while( read_line_len < ( read_line_max - 1 ) ){
    read_char ();
    // exit on LF
    if( c == 10 ){
      c = 0 ;
    }
    ptr = read_line_ptr ;
    ptr_val = c ;
    // this always has room for the null terminator
    wide_ptr_write ();
    _p_i = read_line_ptr ;
    read_line_ptr = _p_i + 1 ;
    read_line_len = read_line_len + 1 ;
    // if we wrote a terminator, exit
    if( c == 0 ){
      return;
    }
  }
  // this is only reached if the line was too long
  // read + write back to only write one byte
  ptr = read_line_ptr ;
  wide_ptr_read ();
  // 0xFF00
  ptr_val = ptr_val & 65280 ;
  wide_ptr_write ();
  read_line_len = read_line_len + 1 ;
  // read until end of line if the line was too long
  while( c != 10 ){
    read_char ();
  }
}


int drive_idx ;
int num_heads ;
int sect_per_track ;
int num_cylinders ;
void get_drive_stats (){
  // mov dl, al
  // mov ah, 8
  // int 0x13
  // push cx
  // push dx
  _ax = drive_idx ;
  asm(" .byte 136 ; .byte 194 ; .byte 180 ; .byte 8 ; .byte 205 ; .byte 19 ; .byte 81 ; .byte 82 ; ");
  // pop ax; move word [0x1000], ax
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  num_heads = ( _ax >> 8 ) + 1 ;
  // pop ax; move word [0x1000], ax
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  num_cylinders = ( ( ( _ax & 192 ) << 8 ) | ( ( _ax & 65280 ) >> 8 ) ) + 1 ;
  sect_per_track = _ax & 63 ;
}

int mul_lhs ;
int mul_rhs ;
int mul_result ;
void int_mul (){
  // multiplies two 16 bit numbers, with wrapping
  mul_result = 0 ;
  while( 0 < mul_rhs ){
    mul_result = mul_result + mul_lhs ;
    mul_rhs = mul_rhs - 1 ;
  }
}

int lba_to_chs_lba ;
int lba_to_chs_c ;
int lba_to_chs_h ;
int lba_to_chs_s ;
int _lba_to_chs_tmp ;
void lba_to_chs (){
  get_drive_stats ();

  int_div_lhs = lba_to_chs_lba ;
  int_div_rhs = sect_per_track ;
  int_div ();
  lba_to_chs_s = int_div_rem + 1 ;
  int_div_lhs = int_div_quot ;
  int_div_rhs = num_heads ;
  int_div ();
  lba_to_chs_h = int_div_rem ;
  lba_to_chs_c = int_div_quot ;
}

int io_lba ;
int* io_buf ;
int _io_ax ;
int _io_cx ;
int _io_dx ;
void disk_io (){
  // reads or writes a sector from disk
  // ARGUMENTS:
  // <global> io_lba - the linear address of the sector to load
  // <global> _io_ax - either 513 to read or 769 to write
  // RETURNS: <global> io_buf - points to the loaded data
  lba_to_chs_lba = io_lba ;
  lba_to_chs ();
  // 192 - 0xC0
  _io_cx = ( ( lba_to_chs_c & 255 ) << 8 ) | ( lba_to_chs_s | ( ( lba_to_chs_c >> 2 ) & 192 ) ) ;
  // set dx to XXYY
  // where YY is the drive id from the bios
  _io_dx = ( lba_to_chs_h << 8 ) | drive_idx ;
  // 0x6000
  io_buf = 24576 ;
  _ax = io_buf ;
  asm(" .byte 80 ; ");
  _ax = _io_dx ;
  asm(" .byte 80 ; ");
  _ax = _io_cx ;
  asm(" .byte 80 ; ");
  _ax = _io_ax ;
  asm(" .byte 80 ; ");
  asm(" .byte 88 ; .byte 89 ; .byte 90 ; .byte 91 ; .byte 205 ; .byte 19 ; ");
  // asm(" .byte 134 ; .byte 224 ; .byte 152 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  // if( _ax != 0 ){
  //   asm(" .byte 235 ; .byte 254 ; .byte 144 ; .byte 144 ; ");
  // }
}

void read_sector (){
  _io_ax = 513 ;
  disk_io ();
}

void write_sector (){
  _io_ax = 769 ;
  disk_io ();
}

int* FAT ;
int* fat16_root_data ;
int start_sector ;
int first_data_offset ;
int root_dir_offset ;
int root_dir_entries ;
int fat1_offset ;
int fat2_offset ;
int* dirent_file_name ;

int* find_file_name ;
int* find_file_meta ;
int _find_file_idx ;
void find_file (){
  // ARGUMENTS: <global find_file_name> pointer to 12 bytes that contain the null terminated name of the file
  // RETURNS: <global find_file_meta> pointer to the start of the fs metadata block that matched, or 0
  find_file_meta = fat16_root_data ;
  _find_file_idx = 0 ;
  while( _find_file_idx < root_dir_entries ){
    // search for a fs entry with the same name
    strcmp_lhs = find_file_name ;
    strcmp_rhs = find_file_meta ;
    strcmp ();
    if( strcmp_diff == 0 ){
      return;
    }

    // increment to the next fs entry (16 bytes each)
    find_file_meta = find_file_meta + 8 ;
    _find_file_idx = _find_file_idx + 1 ;
  }

  // if we got here, no file found
  find_file_meta = 0 ;
}


int* open_file_metadata ;
int  open_file_sector ;
int  open_file_fat_cluster ;
int  open_file_length ;
void open_file (){
  // opens a a file and loads its first sector into memory
  // ARGUMENTS: <global> open_file_metadata - pointer to the in memory fs metadata describing the file to load
  // RETURNS: NONE

  // the file starts opened at the first logical sector of the file
  // this is the first cluster in the chain
  open_file_sector = 0 ;

  // read cluster number and load that first cluster into memory
  open_file_metadata = open_file_metadata + 6 ;
  open_file_fat_cluster = * open_file_metadata ;
  io_lba = ( first_data_offset + open_file_fat_cluster ) - 2 ;
  read_sector ();

  open_file_metadata = open_file_metadata + 1 ;
  open_file_length = * open_file_metadata ;

  // move back to start of metadata
  open_file_metadata = open_file_metadata - 7 ;
}

int next_fat_idx ;
int* next_fat_val ;
void next_fat_cluster (){
  // finds the next non-occupied cluster in the FAT
  // ARGUMENTS: IMPLICIT DISK STATE
  // RETURNS: <global> next_fat_idx - the index of the next free cluster, or -1 if it does not exist

  // entries 0 and 1 are reserved
  next_fat_idx = 2 ;
  // FIXME: this is wrong
  // we only have 256 clusters
  while( next_fat_idx < 256 ){
    next_fat_val = FAT + next_fat_idx ;
    next_fat_val = * next_fat_val ;
    if( next_fat_val == 0 ){
      return;
    }
    next_fat_idx = next_fat_idx + 1 ;
  }
  // no free cluster found
  next_fat_idx = 65535 ;
}

int cluster ;
int next_cluster ;
void next_cluster_get (){
  // gets the next cluster in a cluster chain
  // ARGUMENTS: IMPLICIT DISK STATE
  //            <global> cluster - the cluster to find the next cluster for
  // RETURNS: <global> next_cluster - the next cluster index
  next_fat_val = FAT + cluster ;
  next_cluster = * next_fat_val ;
}

void next_cluster_set (){
  // sets the next cluster in a cluster chain
  // ARGUMENTS: IMPLICIT DISK STATE
  //            <global> cluster - the cluster to set the next cluster for
  //            <global> next_cluster - the cluster to set
  // RETURNS: NONE
  next_fat_val = FAT + cluster ;
  * next_fat_val = next_cluster ;
}

int seek_sector ;
int _seek_s ;
int _seek_c ;
void seek_open_file (){
  // seeks to the absolute sector seek_sector in the open file
  // updates open_file_sector and open_file_fat_cluster

  _p = open_file_metadata + 6 ;
  _seek_c = * _p ;
  _seek_s = 0 ;
  while( _seek_s < seek_sector ){
    cluster = _seek_c ;
    next_cluster_get ();
    if( next_cluster == 65535 ){
      // TODO: return an error probably?
      return;
    }
    _seek_c = next_cluster ;
    _seek_s = _seek_s + 1 ;
  }

  open_file_sector = _seek_s ;
  open_file_fat_cluster = _seek_c ;

  io_lba = ( first_data_offset + _seek_c ) - 2 ;
  read_sector ();
}

void allocate_new_cluster (){
  // allocates a new cluster for a file
  // ARGUMENTS: IMPLICIT OPEN FILE STATE
  //            <global> cluster - the cluster to insert the new cluster after
  // RETURNS: <global> next_fat_idx - the newly allocated cluster
  //          <global> next_cluster - always 0xFFFF
  next_fat_cluster ();

  next_cluster = next_fat_idx ;
  next_cluster_set ();
  io_lba = ( first_data_offset + next_fat_idx ) - 2 ;
  // zero the newly allocated cluster
  // FIXME: THIS ISNT EVEN RUN???
  memset_ptr = io_buf ;
  memset_val = 0 ;
  memset_count = 256 ;
  // set the cluster after the next cluster to be EoF
  cluster = next_fat_idx ;
  // 0xFFFF
  next_cluster = 65535 ;
  next_cluster_set ();
}

int* _update_dir_ptr ;
int _update_dir_idx ;
void update_directory (){
  // updates the on-disk directory from the in-memory directory data

  // 0x6200
  tmp = 25088 ;
  memset_ptr = tmp ;
  // 0x800 words
  memset_count = 2048 ;
  memset_val = 0 ;
  memset ();

  _update_dir_ptr = fat16_root_data ;
  _update_dir_idx = 0 ;
  while( _update_dir_idx < root_dir_entries ){
    // copy filename
    memcpy_src = _update_dir_ptr ;
    memcpy_dst = tmp ;
    memcpy_count = 6 ;
    memcpy ();

    // start cluster at offset 12
    _update_dir_ptr = _update_dir_ptr + 6 ;
    // start cluster at offset 0x1A
    tmp = tmp + 13 ;
    * tmp = * _update_dir_ptr ;

    _update_dir_ptr = _update_dir_ptr + 1 ;
    tmp = tmp + 1 ;
    * tmp = * _update_dir_ptr ;

    _update_dir_idx = _update_dir_idx + 1 ;
    // already at offset 14 of 16
    _update_dir_ptr = _update_dir_ptr + 1 ;
    // already at offset 0x1C of 0x20
    tmp = tmp + 2 ;
  }

  // 0x6200
  tmp = 25088 ;
  // directory table is 0x1000 bytes - 8 sectors
  _update_dir_ptr = tmp + 2048 ;
  io_lba = root_dir_offset ;
  while( tmp < _update_dir_ptr ){
    memcpy_src = tmp ;
    memcpy_dst = io_buf ;
    memcpy_count = 256 ;
    memcpy ();
    write_sector ();
    tmp = tmp + 256 ;
    io_lba = io_lba +  1 ;
  }
}

void update_fat (){
  // write the updated FAT back out to disk
  memcpy_src = FAT ;
  memcpy_dst = io_buf ;
  memcpy_count = 256 ;
  memcpy ();
  io_lba = fat1_offset ;
  write_sector ();
  io_lba = fat2_offset ;
  write_sector ();
}

void free_fat_chain (){
  // frees all of the clusters in a FAT chain to the end of the file
  // ARGUMENTS: <global> cluster - the cluster to start freeing from
  // RETURNS: NONE
  local_val = cluster ;
  push_local ();
  next_cluster_get ();
  if( 1 < next_cluster ){
    // for valid next clusters, try to free them recursively
    cluster = next_cluster ;
    free_fat_chain ();
  }
  pop_local ();
  cluster = local_val ;
  next_cluster = 0 ;
  next_cluster_set ();
}

int file_length ;
int* _set_file_ptr ;
int _set_file_count ;
int _last_cluster ;
void file_length_set (){
  // sets the length of a file to a specified value and commits it to disk
  // allocates or frees FAT clusters as needed to set the size of the file
  // ARGUMENTS: IMPLICIT OPEN FILE STATE
  //   <global> file_length - the length to set
  // RETURNS: NONE

  // follow the FAT chain to set it up properly
  // all files have at least one cluster allocated
  _set_file_count = 512 ;
  _set_file_ptr = open_file_metadata + 6 ;
  cluster = * _set_file_ptr ;
  _last_cluster = cluster ;
  next_cluster_get ();
  // count allocated clusters
  while( ( _set_file_count < file_length ) & ( 1 < next_cluster ) ){
    _last_cluster = cluster ;
    cluster = next_cluster ;
    next_cluster_get ();
    _set_file_count = _set_file_count + 512 ;
  }
  // we now either have enough clusters or ran out


  while( _set_file_count < file_length ){
    allocate_new_cluster ();
    cluster = next_fat_idx ;
    _set_file_count = _set_file_count + 512 ;
  }

  // if there's still clusters remaining in the chain, free them
  // allocate_new_cluster sets next_cluster to FFFF, so this only runs if there was extra
  if( 1 < next_cluster ){
    cluster = next_cluster ;
    free_fat_chain ();
    // set the last used cluster to point to EoF
    cluster = _last_cluster ;
    next_cluster = 65535 ;
    next_cluster_set ();
  }

  // write to the in-memory fs structure
  open_file_metadata = open_file_metadata + 7 ;
  * open_file_metadata = file_length ;
  open_file_length = file_length ;
  open_file_metadata = open_file_metadata - 7 ;

  update_directory ();
  update_fat ();
}

int* create_file_name ;
int* _create_file_ptr ;
void create_file (){
  // creates a file with the specified name and zero length, and then opens it
  // ARGUMENTS: FS STATE
  //            <global> create_file_name - the 8.3 null terminated file name to create

  // find an open entry in the directory by looking for a file that has cluster 0
  _create_file_ptr = fat16_root_data + 6 ;
  while( * _create_file_ptr != 0 ){
    _create_file_ptr = _create_file_ptr + 8 ;
  }

  // copy the name
  memcpy_src = create_file_name ;
  memcpy_dst = _create_file_ptr - 6 ;
  memcpy_count = 6 ;
  memcpy ();

  next_fat_cluster ();
  * _create_file_ptr = next_fat_idx ;
  cluster = next_fat_idx ;
  next_cluster = 65535 ;
  next_cluster_set ();

  // zero length
  _create_file_ptr = _create_file_ptr + 1 ;
  * _create_file_ptr = 0 ;

  update_directory ();
  update_fat ();

  open_file_metadata = _create_file_ptr - 7 ;
  open_file ();
}

int write_file_seg ;
int* write_file_buf ;
int  write_file_count ;
int  write_file_offset ;
int _write_file_cluster ;
int _w_len ;
void write_file (){
  // writes the contents of a buffer into the open file at a specified offset in the file
  // expands the file if needed
  // ARGUMENTS:
  //   IMPLICIT OPEN FILE STATE
  //   <global> write_file_buf - the buffer to write into the file
  //   <global> write_file_count - the number of bytes to write to the file
  //   <global> write_file_offset - the offset into the file to begin writing at

  if( open_file_length < write_file_offset ){
    write_file_offset = open_file_length ;
  }

  // calculate the end size of the file after the write is done
  // reusing this variable
  _write_file_cluster = write_file_offset + write_file_count ;
  file_length = open_file_length ;
  if( file_length < _write_file_cluster ){
    file_length = _write_file_cluster ;
  }
  // allocates FAT clusters as needed
  file_length_set ();

  // bytes needed to align to a sector
  _w_len = 512 - ( write_file_offset & 511 ) ;

  if( write_file_count < _w_len ){
    _w_len = write_file_count ;
  }

  // get the correct cluster from the offset in the file
  _write_file_cluster = open_file_fat_cluster ;
  _p_i = write_file_offset ;
  while( 511 < _p_i ){
    cluster = _write_file_cluster ;
    next_cluster_get ();
    _write_file_cluster = next_cluster ;
    _p_i = _p_i - 512 ;
  }

  // don't do this alignment write if it's not needed
  if( _w_len != 512 ){
    io_lba = ( first_data_offset + _write_file_cluster ) - 2 ;
    read_sector ();
    gs = write_file_seg ;
    memcpy_src = write_file_buf ;
    _p_i = io_buf ;
    memcpy_dst = _p_i + ( write_file_offset & 511 ) ;
    // ( len + 2 ) & 0xFFFE : aligns up to 2
    memcpy_count = ( ( _w_len + 2 ) & 65534 ) >> 1 ;
    memcpy_gs_src ();
    _p_i = write_file_buf ;
    write_file_buf = _p_i + _w_len ;
    write_file_count = write_file_count - _w_len ;
    write_sector ();
    cluster = _write_file_cluster ;
    next_cluster_get ();
    _write_file_cluster = next_cluster ;
  }

  while( 0 < write_file_count ){
    io_lba = ( first_data_offset + _write_file_cluster ) - 2 ;
    read_sector ();
    gs = write_file_seg ;
    memcpy_src = write_file_buf ;
    memcpy_dst = io_buf ;
    // align up to 2
    memcpy_count = ( ( write_file_count + 2 ) & 65534 ) >> 1 ;
    // write at most one sector at a time
    if( 256 < memcpy_count ){
      memcpy_count = 256 ;
    }
    // FIXME: this is maybe right?
    write_file_buf = write_file_buf + memcpy_count ;
    write_file_count = write_file_count - ( memcpy_count << 1 ) ;
    memcpy_gs_src ();
    write_sector ();

    cluster = _write_file_cluster ;
    next_cluster_get ();
    _write_file_cluster = next_cluster ;
  }
}

int buf ;
int read_byte_val ;
int* _read_byte_ptr ;
void read_byte_buf (){
  _read_byte_ptr = buf ;
  read_byte_val = * _read_byte_ptr ;
  read_byte_val = read_byte_val & 255 ;
  buf = buf + 1 ;
}

int* _read_dir_entry_ptr ;
int _read_dir_entry_count ;
int _read_dir_start_sector ;
void read_dir_entry (){
  // reads a directory entry into the in-memory fs structure, assuming fat16_root_data points to an empty entry
  // also sets the contents of dirent_file_name to be the name of the file in 8.3 format
  // ARGUMENTS:
  // - pointer to the start of the directory entry
  // RETURNS:
  // - 0 if file exists, 1 if entry was vacant, and 2 if the entry is the end of the directory

  pop_local ();
  _read_dir_entry_ptr = local_val ;
  buf = _read_dir_entry_ptr ;

  // the first byte of the entry is the file name, but has some special values
  read_byte_buf ();

  if( read_byte_val == 0 ){
    // "end of directory" status
    local_val = 2 ;
    push_local ();
    return;
  }

  _read_dir_entry_count = 0 ;
  // copy the first 10 bytes of the filename to the name buffer
  while( _read_dir_entry_count < 5 ){
    * dirent_file_name = * _read_dir_entry_ptr ;
    dirent_file_name = dirent_file_name + 1 ;
    _read_dir_entry_ptr = _read_dir_entry_ptr + 1 ;
    _read_dir_entry_count = _read_dir_entry_count + 1 ;
  }
  // then add the last byte and its null terminator
  * dirent_file_name = * _read_dir_entry_ptr & 255 ;
  // move back to the start of the name
  dirent_file_name = dirent_file_name - 5 ;

  // copy the filename to the fat16 tables
  memcpy_src = dirent_file_name ;
  memcpy_dst = fat16_root_data ;
  memcpy_count = 6 ;
  memcpy ();
  fat16_root_data = fat16_root_data + 6 ;

  // pointer is at the last byte of the filename: offset 0x0A

  // get the start sector of the file (offset 0x1A)
  _read_dir_entry_ptr = _read_dir_entry_ptr + 8 ;
  * fat16_root_data = * _read_dir_entry_ptr ;
  fat16_root_data = fat16_root_data + 1 ;

  // file size
  _read_dir_entry_ptr = _read_dir_entry_ptr + 1 ;
  * fat16_root_data = * _read_dir_entry_ptr ;
  fat16_root_data = fat16_root_data + 1 ;

  // found file, return status 0
  local_val = 0 ;
  push_local ();
}

int _read_root_count ;
int _read_root_cluster ;
int _read_root_dirent_status ;
void init_fs (){
  // reads the FAT16 data on disk into an in-memory structure
  // this both loads the FAT itself and the contents of the root directory
  start_sector = 1 ;
  // these are all ABSOLUTE offsets from the start of the disk, not
  // relative to the start of the partition
  fat1_offset = 2 ;
  fat2_offset = 66 ;
  root_dir_offset = 130 ;
  root_dir_entries = 128 ;
  first_data_offset = 138 ;

  // get the drive number from the BIOS
  // 0x0600
  _p = 1536 ;
  drive_idx = * _p & 255 ;

  // TODO: somehow check that start_sector is a valid BPB for the partition
  // and if not, set it up, as well as two empty FATs

  // 0x8800
  FAT = 34816 ;
  io_lba = fat1_offset ;
  read_sector ();
  memcpy_src = io_buf ;
  memcpy_dst = FAT ;
  // 256 words = 1 sector
  memcpy_count = 256 ;
  memcpy ();

  io_lba = root_dir_offset ;
  read_sector ();

  // 0x8000
  fat16_root_data = 32768 ;
  memset_ptr = fat16_root_data ;
  memset_val = 0 ;
  // 0x400 words
  memset_count = 1024 ;
  memset ();

  _read_root_count = 0 ;
  while( _read_root_count < root_dir_entries ){
    local_val = io_buf ;
    push_local ();
    read_dir_entry ();
    pop_local ();

    _read_root_dirent_status = local_val ;
    // status 2 is end of directory
    if( _read_root_dirent_status == 2 ){
      // 0x6000
      io_buf = 24576 ;
      // 0x8000
      fat16_root_data = 32768 ;
      return;
    }
    // FIXME: implement status 1: vacant entry
    if( _read_root_dirent_status == 0 ){
      // each entry is 32 bytes
      io_buf = io_buf + 16 ;
      _read_root_count = _read_root_count + 1 ;
    }
  }

  // 0x6000
  io_buf = 24576 ;
  // 0x8000
  fat16_root_data = 32768 ;
}

void list_files (){
  _read_dir_entry_ptr = fat16_root_data ;
  _read_root_count = 0 ;
  while( _read_root_count < root_dir_entries ){
    _read_dir_entry_ptr = _read_dir_entry_ptr + 6 ;
    _p_i = * _read_dir_entry_ptr ;
    // skip files with an invalid start cluster (they don't exist)
    if( _p_i != 0 ){
      print_str_seg = 0 ;
      print_str_ptr = _read_dir_entry_ptr - 6 ;
      print_str_len = 12 ;
      print_string_len ();
      c = 32 ;
      print_char ();
      _read_dir_entry_ptr = _read_dir_entry_ptr + 1 ;
      print_hex_val = * _read_dir_entry_ptr ;
      println_hex ();
    }
    // go to next entry (already at offset 14)
    _read_dir_entry_ptr = _read_dir_entry_ptr + 1 ;
    _read_root_count = _read_root_count + 1 ;
  }
}

int read_file_offset ;
int read_file_count ;
int read_file_seg ;
int* read_file_ptr ;
void read_file_buf (){
    while( 0 < read_file_count ){
      // ptr / 512
      _p_i = read_file_offset ;
      seek_sector = _p_i >> 9 ;
      if( open_file_sector != seek_sector ){
        seek_open_file ();
      }
      // offset within the io buffer to read from (ptr % 512)
      // we have to do this dance to get the offsets to be bytes
      ptr_val = read_file_offset ;
      ptr_val = ptr_val & 511 ;
      _p_i = io_buf ;
      _p = _p_i + ptr_val ;
      gs = read_file_seg ;
      // need to read, mask, and write back to ensure
      // that garbage doesn't get put into the byte after the end of the buffer
      ptr = read_file_ptr ;
      wide_ptr_read ();
      // FF00 | 00FF
      ptr_val = ( ptr_val & 65280 ) | ( * _p & 255 ) ;
      wide_ptr_write ();
      _p_i = read_file_offset ;
      read_file_offset = _p_i + 1 ;
      _p_i = read_file_ptr ;
      read_file_ptr = _p_i + 1 ;
      read_file_count = read_file_count - 1 ;
    }

}

int* main_addr ;

int* _fname ;
int bin_lba ;
void backup_and_overwrite (){
  // saves the following data as the first three files on the root directory, in order:
  //  - the currently executing binary
  //  - the original boot sector that acts as a compiler
  // then writes a new boot sector to the drive

  // create the first file with name "PROGRAM .BIN\0"
  // 0x0F00
  _fname = 3840 ;
  * _fname = 21072 ;
  _fname = _fname + 1 ;
  * _fname = 18255 ;
  _fname = _fname + 1 ;
  * _fname = 16722 ;
  _fname = _fname + 1 ;
  * _fname = 8269 ;
  _fname = _fname + 1 ;
  * _fname = 18754 ;
  _fname = _fname + 1 ;
  * _fname = 78 ;

  create_file_name = _fname - 5 ;
  create_file ();

  // save the lba of the binary to place in the boot sector
  open_file_metadata = open_file_metadata + 6 ;
  bin_lba = ( first_data_offset + * open_file_metadata ) - 2 ;
  open_file_metadata = open_file_metadata - 6 ;

  // the program occupies the region 0x1000-0x5FFF (inclusive)
  write_file_seg = 0 ;
  write_file_buf = 4096 ;
  // 0x5000 bytes
  write_file_count = 20480 ;
  write_file_offset = 0 ;
  write_file ();

  // save the old boot sector in the second file on the fs
  // "BOOT    .BIN\0"
  // 0x0F00
  _fname = 3840 ;
  * _fname = 20290 ;
  _fname = _fname + 1 ;
  * _fname = 21583 ;
  _fname = _fname + 1 ;
  * _fname = 8224 ;
  _fname = _fname + 1 ;
  * _fname = 8224 ;
  _fname = _fname + 1 ;
  * _fname = 18754 ;
  _fname = _fname + 1 ;
  * _fname = 78 ;
  create_file_name = _fname - 5 ;
  create_file ();

  // the boot sector is at 0x7C00-0x7DFF (inclusive)
  write_file_seg = 0 ;
  write_file_buf = 31744 ;
  // 0x200 bytes
  write_file_count = 512 ;
  write_file_offset = 0 ;
  write_file ();

  // save a modified copy of the boot sector that is suitable for use by the fs driver program
  // "COMPILER.BIN\0"
  // 0x0F00
  _fname = 3840 ;
  * _fname = 20291 ;
  _fname = _fname + 1 ;
  * _fname = 20557 ;
  _fname = _fname + 1 ;
  * _fname = 19529 ;
  _fname = _fname + 1 ;
  * _fname = 21061 ;
  _fname = _fname + 1 ;
  * _fname = 18754 ;
  _fname = _fname + 1 ;
  * _fname = 78 ;
  create_file_name = _fname - 5 ;
  create_file ();

  // overwrite the code that normally jumps to main with
  // pop cx
  // far ret
  // 0x7D7A
  _p = 32122 ;
  * _p = 52057 ;

  // 0x7C00
  write_file_seg = 0 ;
  write_file_buf = 31744 ;
  // 0x200 bytes
  write_file_count = 512 ;
  write_file_offset = 0 ;
  write_file ();

  // 0x6200
  tmp = 25088 ;
  memset_ptr = tmp ;
  memset_count = 256 ;
  memset_val = 0 ;
  memset ();

  // BEGIN GENERATED CODE
  * tmp = 49201 ;
  tmp = tmp + 1 ;
  * tmp = 49294 ;
  tmp = tmp + 1 ;
  * tmp = 55438 ;
  tmp = tmp + 1 ;
  * tmp = 5768 ;
  tmp = tmp + 1 ;
  * tmp = 1536 ;
  tmp = tmp + 1 ;
  * tmp = 46162 ;
  tmp = tmp + 1 ;
  * tmp = 52488 ;
  tmp = tmp + 1 ;
  * tmp = 34835 ;
  tmp = tmp + 1 ;
  * tmp = 16950 ;
  tmp = tmp + 1 ;
  * tmp = 34940 ;
  tmp = tmp + 1 ;
  * tmp = 9416 ;
  tmp = tmp + 1 ;
  * tmp = 41535 ;
  tmp = tmp + 1 ;
  * tmp = 31811 ;
  tmp = tmp + 1 ;
  * tmp = 47777 ;
  tmp = tmp + 1 ;
  * tmp = 63101 ;
  tmp = tmp + 1 ;
  * tmp = 17206 ;
  tmp = tmp + 1 ;
  * tmp = 34940 ;
  tmp = tmp + 1 ;
  * tmp = 65249 ;
  tmp = tmp + 1 ;
  * tmp = 46273 ;
  tmp = tmp + 1 ;
  * tmp = 35328 ;
  tmp = tmp + 1 ;
  * tmp = 16918 ;
  tmp = tmp + 1 ;
  * tmp = 65148 ;
  tmp = tmp + 1 ;
  * tmp = 63170 ;
  tmp = tmp + 1 ;
  * tmp = 23282 ;
  tmp = tmp + 1 ;
  * tmp = 59016 ;
  tmp = tmp + 1 ;
  * tmp = 50568 ;
  tmp = tmp + 1 ;
  * tmp = 10424 ;
  tmp = tmp + 1 ;
  * tmp = 47874 ;
  tmp = tmp + 1 ;
  * tmp = 4096 ;
  tmp = tmp + 1 ;
  * tmp = 5069 ;
  tmp = tmp + 1 ;
  * tmp = 65138 ;
  tmp = tmp + 1 ;
  * tmp = 9983 ;
  tmp = tmp + 1 ;
  * tmp = 32188 ;
  tmp = tmp + 188 ;
  * tmp = 26265 ;
  tmp = tmp + 4 ;
  * tmp = 2 ;
  tmp = tmp + 1 ;
  * tmp = 4096 ;
  tmp = tmp + 1 ;
  * tmp = 529 ;
  tmp = tmp + 1 ;
  * tmp = 1 ;
  tmp = tmp + 2 ;
  * tmp = 16383 ;
  tmp = tmp + 26 ;
  * tmp = 43605 ;
  // END GENERATED CODE

  // 0x63BA - location of the LBA to fill in
  // 0x63BC - address of main to jump to
  tmp = 25530 ;
  * tmp = bin_lba ;
  tmp = tmp + 1 ;
  * tmp = main_addr ;

  // 0x6200
  memcpy_src = 25088 ;
  memcpy_dst = io_buf ;
  memcpy_count = 256 ;
  memcpy ();
  io_lba = 0 ;
  write_sector ();
}

int is ;
void is_num (){
  // returns whether a character is numeric
  is = 1 ;
  if( ( c < 48 ) | ( 57 < c ) ){
    is = 0 ;
  }
}

int* line ;
int line_len ;
void read_line (){
  // 0x8F00
  line = 36608 ;
  line_len = 0 ;
  // the first character was already read
  while( line_len < 128 ){
    read_char ();
    * line = c ;
    line_len = line_len + 1 ;
    _p_i = line ;
    line = _p_i + 1 ;
    // exit on newline
    if( c == 10 ){
      // 0x8F00
      line = 36608 ;
      return;
    }
  }
  // if the line count reaches 128 without a newline, truncate with one
  * line = 10 ;
  // 0x8F00
  line = 36608 ;
  // TODO: maybe eat until a newline is found?
}

int num ;
void atoi (){
  // reads a number from `line`
  num = 0 ;
  while( 0 < line_len ){
    c = * line & 255 ;
    is_num ();
    if( is != 1 ){
      return;
    }
    // num = 10 * num
    num = ( ( num << 2 ) + num ) << 1 ;
    num = num + ( c - 48 ) ;
    _p_i = line ;
    line = _p_i + 1 ;
    line_len = line_len - 1 ;
  }
}

int _next_line ;
int* active_line ;
int cursor_column ;

void advance_lines (){
  // advances num lines (signed), clamped to the start or end
  // num has unspecified value after this

  // positive advance
  while( 0 < num ){
    active_line = active_line + 1 ;
    _next_line = * active_line ;
    if( _next_line != 0 ){
      active_line = _next_line ;
    }
    // don't advance if at end of lines
    if( _next_line == 0 ){
      active_line = active_line - 1 ;
      return;
    }
    num = num - 1 ;
  }

  // negative advance
  while( num < 0 ){
    _next_line = * active_line ;
    if( _next_line != 0 ){
      active_line = _next_line ;
    }
    // don't advance if at start of lines
    if( _next_line == 0 ){
      return;
    }
    num = num + 1 ;
  }
}

void print_line (){
  line = line + 3 ;
  print_str_len = * line >> 8 ;
  print_str_seg = ( * line & 255 ) << 8 ;
  line = line - 1 ;
  print_str_ptr = * line ;
  line = line - 2 ;
  print_string_len ();
}


int _context_count ;
int _context_done ;
int _drawn_lines ;
void print_editor (){
  clear ();
  
  // prints the region of text around the active line
  _context_count = 0 ;
  _context_done = 0 ;
  line = active_line ;
  // print backwards context
  while( ( _context_count < 10 ) & ( _context_done != 1 ) ){
    if( * line == 0 ){
      _context_done = 1 ;
    }
    if( * line != 0 ){
      line = * line ;
      _context_count = _context_count + 1 ;
    }
  }
  _context_count = _context_count + 11 ;
  _drawn_lines = 0 ;
  while( _drawn_lines < _context_count ){
    if( line != active_line ){
      c = 32 ;
      print_char ();
      print_char ();
    }
    if( line == active_line ){
      // > 
      c = 62 ;
      print_char ();
      c = 32 ;
      print_char ();
    }
    print_line ();
    _drawn_lines = _drawn_lines + 1 ;
    line = line + 1 ;
    if( * line == 0 ){
      // break
      _context_count = 0 ;
    }
    if( * line != 0 ){
      line = * line ;
    }
  }

  // pad to the end of the screen
  _context_count = 21 - _drawn_lines ;
  while( 0 < _context_count ){
    c = 10 ;
    print_char ();
    _context_count = _context_count - 1 ;
  }
  // ~
  c = 126 ;
  print_char ();
}

int text_editor_state ;
int* text_buf ;
int* metadata ;
int* first_line_meta ;
int* text_buf_end ;
int* _meta ;
int _len ;
int* alloc_line_meta ;
int _line_c ;

void init_line (){
  // initializes a line by copying to it and setting up the linked list
  // metadata should point to the len of the line to init

  // set the length of the new line
  // and gs to 0x4000
  * metadata = ( line_len << 8 ) | 64 ;
  // set the line metadata to point to the newly allocated line
  metadata = metadata - 1 ;
  * metadata = text_buf_end ;

  // insert the line in the linked list
  // make this line's next line be the active line
  metadata = metadata - 1 ;
  * metadata = active_line ;
  // make the previous line for this line be the active line's previous line
  metadata = metadata - 1 ;
  * metadata = * active_line ;
  // make the next line for the active line's previous line be this line
  _meta = * metadata ;
  if( _meta != 0 ){
    _meta = _meta + 1 ;
    * _meta = metadata ;
  }
  // make the active line's previous line be this line
  * active_line = metadata ;
  alloc_line_meta = metadata ;

  // if inserting before the start, update the first line
  if( active_line == first_line_meta ){
    first_line_meta = metadata ;
  }

  // copy the line to the end of the text buffer
  // 0x4000
  gs = 16384 ;
  memcpy_src = line ;
  memcpy_dst = text_buf_end ;
  memcpy_count = line_len >> 1 ;
  _p_i = text_buf_end ;
  text_buf_end = _p_i + line_len ;
  // odd number of bytes to copy, handle the first one specially
  if( ( line_len & 1 ) == 1 ){
    ptr = memcpy_dst ;
    wide_ptr_read ();
    // replace low 8 bits with first byte of line
    ptr_val = ( ptr_val & 65280 ) | ( * line & 255 ) ;
    wide_ptr_write ();
    _p_i = memcpy_src ;
    memcpy_src = _p_i + 1 ;
    _p_i = memcpy_dst ;
    memcpy_dst = _p_i + 1 ;
  }
  // copy the remaining bytes
  memcpy_gs_dst ();

  // 0x9000
  metadata = 36864 ;
}

void insert_line (){
  // args: line, line_len
  // inserts before active_line

  // FIXME: line count
  _line_c = 0 ;
  while( _line_c < 2048 ){
     metadata = metadata + 3 ;
    _len = * metadata >> 8 ;
    // never allocated
    if( _len == 128 ){
      init_line ();
      return;
    }
    if( 128 < _len ){
      // previously allocated, check if suitable
      _len = _len & 127 ;
      if( _len == line_len ){
        init_line ();
        return;
      }
    }

    metadata = metadata + 1 ;
    _line_c = _line_c + 1 ;
  }

  // 0x9000
  metadata = 36864 ;
}

void create_or_open (){
  // creates or opens a file, intended for overwriting
  // args: find_file_name

  find_file ();
  if( find_file_meta != 0 ){
    open_file_metadata = find_file_meta ;
    open_file ();
  }
  if( find_file_meta == 0 ){
    create_file_name = find_file_name ;
    create_file ();
  }
}

void save_editor (){
  line_len = 13 ;
  while( line_len != 12 ){
    read_line ();
  }

  _p_i = line ;
  _p = _p_i + 11 ;
  * _p = 0 ;

  find_file_name = line ;
  create_or_open ();

  // loop starting from first_line_meta and write each line
  _line_c = 0 ;
  _meta = first_line_meta ;
  while( 1 == 1 ){
    _meta = _meta + 2 ;
    memcpy_src = * _meta ;
    _meta = _meta + 1 ;
    gs = ( * _meta & 255 ) << 8 ;
    write_file_count = * _meta >> 8 ;
    // 0x7E00
    memcpy_dst = 32256 ;
    // potentially writes one extra byte, but it won't be written to the file
    memcpy_count = ( write_file_count + 1 ) >> 1 ;
    memcpy_gs_src ();
    // TODO: this could be fixed to write directly from far memory
    write_file_seg = 0 ;
    // 0x7E00
    write_file_buf = 32256 ;
    write_file_offset = _line_c ;
    print_hex_val = write_file_offset ;
    print_hex ();
    print_hex_val = write_file_count ;
    println_hex ();
    write_file ();

    _line_c = _line_c + ( * _meta >> 8 ) ;
    _meta = _meta - 2 ;
    if( * _meta == 0 ){
      file_length = _line_c ;
      file_length_set ();
      return;
    }
    _meta = * _meta ;
  }
}

void init_editor (){
  // 0x9000
  metadata = 36864 ;
  // text at 0x1_0000
  text_buf = 0 ;
  text_buf_end = 0 ;
  _meta = 0 ;

  memset_ptr = metadata ;
  memset_count = 14336 ;
  memset_val = 0 ;
  memset ();

  // command state
  text_editor_state = 0 ;

  // set the length of every line to 0x80 - empty, never allocated
  _line_c = 0 ;
  metadata = metadata + 3 ;
  // TODO: line count
  while( _line_c < 2048 ){
    // 0x8040 - free, zero len, gs 0x4000
    * metadata = 32832 ;
    metadata = metadata + 4 ;
    _line_c = _line_c + 1 ;
  }
  // 0x9000
  metadata = 36864 ;
  active_line = metadata ;
  first_line_meta = metadata ;

  // at the end of the file place an empty line to insert before
  // 0x4000
  gs = 16384 ;
  ptr = text_buf_end ;
  ptr_val = 10 ;
  wide_ptr_write ();
  active_line = active_line + 1 ;
  * active_line = 0 ;
  active_line = active_line + 1 ;
  * active_line = text_buf_end ;
  active_line = active_line + 1 ;
  // 0x0140 - len 1, gs 0x4000
  * active_line = 320 ;

  text_buf_end = 1 ;
  active_line = metadata ;
}

void open_file_user (){
  // finds and opens a file specified by user input
  // repeats until a valid file is found
  while( 1 == 1 ){
    line_len = 13 ;
    while( line_len != 12 ){
      read_line ();
    }

    _p_i = line ;
    _p = _p_i + 11 ;
    * _p = 0 ;

    find_file_name = line ;
    find_file ();
    if( find_file_meta == 0 ){
      c = 63 ;
      print_char ();
      c = 10 ;
      print_char ();
    }
    if( find_file_meta != 0 ){
      open_file_metadata = find_file_meta ;
      open_file ();
      return;
    }
  }
}

int _read_c ;
int _open_c ;
int _open_end ;
int* _open_p ;
void open_text (){
  // list the files when opening too, for ease of use
  list_files ();

  // FIXME: this adds an extra line at the end of a file when opening it
  init_editor ();

  open_file_user ();

  // 0x6200
  line = 25088 ;
  line_len = 0 ;
  first_line_meta = 0 ;

  _open_c = 0 ;
  _read_c = 0 ;
  while( _read_c < open_file_length ){
    if( ( _read_c & 511 ) == 0 ){
      seek_sector = _open_c ;
      seek_open_file ();
      _open_c = _open_c + 1 ;
      _open_p = io_buf ;
    }

    c = * _open_p & 255 ;
    * line = c ;
    _p_i = line ;
    line = _p_i + 1 ;
    line_len = line_len + 1 ;
    if( c == 10 ){
      // reset to start of line and insert it
      line = 25088 ;
      insert_line ();
      if( first_line_meta == 0 ){
        first_line_meta = alloc_line_meta ;
      }
      line_len = 0 ;
    }

    _p_i = _open_p ;
    _open_p = _p_i + 1 ;
    _read_c = _read_c + 1 ;
  }

  // 0x9000
  metadata = 36864 ;
  if( first_line_meta != 0 ){
    active_line = first_line_meta ;
  }
}

void exec_compiler (){
  // loads the compiler into memory and executes it
  // assumes that source code is in place at 0x7_0000

  // third file entry is modified boot loader
  // FIXME: maybe find this by name or some other special way?
  open_file_metadata = fat16_root_data + 16 ;
  open_file ();
  memcpy_src = io_buf ;
  // 0x3_7C00
  gs = 12288 ;
  memcpy_dst = 31744 ;
  memcpy_count = 256 ;
  memcpy_gs_dst ();

  // clear ident->addr map
  // xor eax, eax
  // mov cx, 0x8000
  // mov edi, 0x1_0000
  // a32 rep stosd
  asm(" .byte 102 ; .byte 49 ; .byte 192 ; .byte 185 ; .byte 0 ; .byte 128 ; .byte 102 ; .byte 191 ; .byte 0 ; .byte 0 ; .byte 1 ; .byte 0 ; .byte 243 ; .byte 102 ; .byte 103 ; .byte 171 ; ");

  // xor si, si
  // mov ax, 0x3000
  // mov ds, ax
  // mov es, ax
  // mov ax, 0x7000
  // mov fs, ax
  // NOTE: saving room for a jump at the start of the file
  // mov di, 0x1003
  // call 3000:7C48
  // NOTE: must restore ds and es for the driver program
  // xor bx, bx
  // mov ds, bx
  // mov es, bx
  // NOTE: get ax from the compiler to save the address of main
  // mov word [0x1000], ax
  asm(" .byte 137 ; .byte 198 ; .byte 184 ; .byte 0 ; .byte 48 ; .byte 142 ; .byte 216 ; .byte 142 ; .byte 192 ; .byte 184 ; .byte 0 ; .byte 112 ; .byte 142 ; .byte 224 ; .byte 191 ; .byte 3 ; .byte 16 ; .byte 154  ; .byte 72 ; .byte 124 ; .byte 0 ; .byte 48 ; .byte 49 ; .byte 219 ; .byte 142 ; .byte 219 ; .byte 142 ; .byte 195 ; .byte 163 ; .byte 0 ; .byte 16 ; ");

  // offset of main from the end of the jump
  // 0x1003
  main_addr = _ax - 4099 ;
  // 0x1000
  ptr = 4096 ;
  // E9 00
  ptr_val = 233 ;
  wide_ptr_write ();
  _p_i = ptr ;
  ptr = _p_i + 1 ;
  ptr_val = main_addr ;
  wide_ptr_write ();

  // get di from compiler to fix up the final return to be a far return
  // generated code never uses di
  // mov word [0x1000], di
  asm(" .byte 137 ; .byte 62 ; .byte 0 ; .byte 16 ; ");

  // fixup final return
  // di is aligned after the final C3 00 is emitted
  // this means that the C3 byte is either 2 or 3 bytes back
  ptr = _ax - 2 ;
  wide_ptr_read ();
  if( ptr_val != 195 ){
    _p_i = ptr ;
    ptr = _p_i - 1 ;
  }
  // CB 00
  ptr_val = 203 ;
  wide_ptr_write ();

  // "A       .OUT\0"
  // 0x0F00
  _fname = 3840 ;
  * _fname = 8257 ;
  _fname = _fname + 1 ;
  * _fname = 8224 ;
  _fname = _fname + 1 ;
  * _fname = 8224 ;
  _fname = _fname + 1 ;
  * _fname = 8224 ;
  _fname = _fname + 1 ;
  * _fname = 21839 ;
  _fname = _fname + 1 ;
  * _fname = 84 ;
  find_file_name = _fname - 5 ;
  create_or_open ();

  // 0x31000-0x35FFF
  write_file_seg = 12288 ;
  write_file_buf = 4096 ;
  write_file_offset = 0 ;
  write_file_count = 20480 ;
  write_file ();
}

void compile_file (){
  list_files ();
  open_file_user ();

  // round up to nearest sector: (s + 0x1FF) & ~0x1FF
  // then get sector count
  _open_end = ( ( open_file_length + 511 ) & 65024 ) >> 9 ;
  _open_c = 0 ;
  _open_p = 0 ;
  while( _open_c < _open_end ){
    seek_sector = _open_c ;
    seek_open_file ();
    // 0x7000
    gs = 28672 ;
    memcpy_src = io_buf ;
    memcpy_dst = _open_p ;
    memcpy_count = 256 ;
    memcpy_gs_dst ();
    _open_c = _open_c + 1 ;
    _open_p = _open_p + 256 ;
  }

  // set a 0 byte at the end of the file
  ptr = open_file_length ;
  ptr_val = 0 ;
  wide_ptr_write ();

  exec_compiler ();
}

void exec_file (){
  list_files ();
  open_file_user ();

  // read file into 0x31000
  read_file_offset = 0 ;
  read_file_count = open_file_length ;
  read_file_seg = 12288 ;
  read_file_ptr = 4096 ;
  read_file_buf ();

  // mov ax, 0x3000
  // mov ds, ax
  // mov es, ax
  // call far 0x3_1000
  // NOTE: must restore ds - memory acesses use it, and es - bios interrupts use it
  // xor ax, ax
  // mov ds, ax
  // mov es, ax
  asm(" .byte 184 ; .byte 0 ; .byte 48 ; .byte 142 ; .byte 216 ; .byte 142 ; .byte 192 ; .byte 154 ; .byte 0 ; .byte 16 ; .byte 0 ; .byte 48 ; .byte 49 ; .byte 192 ; .byte 142 ; .byte 216 ; .byte 142 ; .byte 192 ; ");
}

int input_c ;
void handle_input (){
  read_line ();

  // insert mode
  if( text_editor_state == 1 ){
    // ESC to exit insert mode
    if( ( * line & 255 ) == 27 ){
      text_editor_state = 0 ;
      print_editor ();
      return;
    }

    insert_line ();
    if( first_line_meta == 0 ){
      first_line_meta = alloc_line_meta ;
    }
    return;
  }

  // command state
  if( text_editor_state == 0 ){
    input_c = * line & 255 ;

    // \n and \r at the start of a line should just be ignored in command mode
    if( ( input_c == 10 ) | ( input_c == 13 ) ){
      return;
    }
    // p
    if( input_c == 112 ){
      print_editor ();
      return;
    }
    // + or -
    if( ( input_c == 43 ) | ( input_c == 45 ) ){
      _p_i = line ;
      line = _p_i + 1 ;
      atoi ();
      if( num == 0 ){
        num = 1 ;
      }
      if( input_c == 45 ){
        num = 0 - num ;
      }
      advance_lines ();
      print_editor ();
      return;
    }
    // i - enters line insertion mode
    if( input_c == 105 ){
      text_editor_state = 1 ;
      return;
    }
    // d - delete line
    if( input_c == 100 ){
      // the last line can't actually be deleted
      active_line = active_line + 1 ;
      if( * active_line == 0 ){
        active_line = active_line - 1 ;
        print_editor ();
        return;
      }
      active_line = active_line + 2 ;
      // 0x8000 - set line free
      * active_line = * active_line | 32768 ;

      // hook up the previous line for the next line
      active_line = active_line - 2 ;
      _meta = * active_line ;
      active_line = active_line - 1 ;
      * _meta = * active_line ;
      // if the previous line exists, hook up its next line
      // if there's no previous line, update the first line pointer
      _meta = * active_line ;
      active_line = active_line + 1 ;
      if( _meta == 0 ){
        first_line_meta = * active_line ;
      }
      if( _meta != 0 ){
        _meta = _meta + 1 ;
        * _meta = * active_line ;
      }

      // set active line to the line after the deleted line
      active_line = * active_line ;
      print_editor ();
      return;
    }
    // w
    if( input_c == 119 ){
      save_editor ();
      print_editor ();
      return;
    }
    // o
    if( input_c == 111 ){
      open_text ();
      print_editor ();
      return;
    }
    // n
    if( input_c == 110 ){
      init_editor ();
      print_editor ();
      return;
    }
    // l
    if( input_c == 108 ){
      list_files ();
      // ~
      c = 126 ;
      print_char ();
      return;
    }
    // c
    if( input_c == 99 ){
      compile_file ();
      print_editor ();
      return;
    }
    // e
    if( input_c == 101 ){
      exec_file ();
      print_editor ();
      return;
    }

    // fall through to unknown input - ?
    c = 63 ;
    print_char ();
    c = 10 ;
    print_char ();
  }
}

void text_editor (){
  init_editor ();
  print_editor ();

  while( 1 == 1 ){
    handle_input ();
  }
}

int int_a ;
int int_b ;
int int_c ;
int int_d ;
int int_si ;
int int_di ;
int int_ret ;

void int_x40 (){
  asm(" .byte 30 ; .byte 6 ; .byte 87 ; .byte 86 ; .byte 82 ; .byte 81 ; .byte 83 ; .byte 80 ; ");

  // set ds and es to 0x0000 because the driver needs it
  asm(" .byte 49 ; .byte 255 ; .byte 142 ; .byte 223 ; .byte 142 ; .byte 199 ; ");

  // grab saved arguments
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_a = _ax ;
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_b = _ax ;
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_c = _ax ;
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_d = _ax ;
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_si = _ax ;
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_di = _ax ;

  if( int_a == 0 ){
    // open_file
    gs = int_b ;
    memcpy_src = int_c ;
    memcpy_dst = 25088 ;
    memcpy_gs_src ();
    find_file_name = 25088 ;
    find_file ();
    if( find_file_meta != 0 ){
      open_file_metadata = find_file_meta ;
      open_file ();
      int_ret = 0 ;
    }
    if( find_file_meta == 0 ){
      int_ret = 1 ;
    }
  }
  if( int_a == 1 ){
    // create_file
    gs = int_b ;
    memcpy_src = int_c ;
    memcpy_dst = 25088 ;
    memcpy_gs_src ();
    find_file_name = 25088 ;
    find_file ();
    // only create the file if it does not yet exist
    if( find_file_meta == 0 ){
      create_file_name = find_file_name ;
      create_file ();
    }
    if( find_file_meta != 0 ){
      int_ret = 1 ;
    }
  }
  if( int_a == 2 ){
    // read_file
    read_file_offset = int_b ;
    read_file_count = int_c ;
    read_file_seg = int_d ;
    read_file_ptr = int_si ;
    read_file_buf ();
    int_ret = 0 ;
  }
  if( int_a == 3 ){
    // write_file
    write_file_offset = int_b ;
    write_file_count = int_c ;
    write_file_seg = int_d ;
    write_file_buf = int_si ;
    write_file ();
    int_ret = 0 ;
  }
  if( int_a == 4 ){
    // file_info
    memcpy_src = open_file_metadata ;
    gs = int_b ;
    memcpy_dst = int_c ;
    memcpy_count = 8 ;
    memcpy_gs_dst ();
    int_ret = 0 ;
  }

  // restore ds and es for the caller
  // iret
  _ax = int_ret ;
  asm(" .byte 7 ; .byte 31 ; .byte 207 ; ");
}

void int_x41 (){
  // int 0x41 doesn't use si or di
  asm(" .byte 30 ; .byte 6 ; .byte 82 ; .byte 81 ; .byte 83 ; .byte 80 ; ");

  // set ds and es to 0x0000 because the driver needs it
  asm(" .byte 49 ; .byte 255 ; .byte 142 ; .byte 223 ; .byte 142 ; .byte 199 ; ");

  // grab saved arguments
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_a = _ax ;
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_b = _ax ;
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_c = _ax ;
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  int_d = _ax ;

  if( int_a == 0 ){
    read_char ();
    int_ret = c ;
  }
  if( int_a == 1 ){
    c = int_b ;
    print_char ();
    int_ret = 0 ;
  }
  if( int_a == 2 ){
    print_str_seg = int_b ;
    print_str_ptr = int_c ;
    print_string ();
    int_ret = 0 ;
  }
  if( int_a == 3 ){
    print_str_seg = int_b ;
    print_str_ptr = int_c ;
    print_str_len = int_d ;
    print_string_len ();
    int_ret = 0 ;
  }
  if( int_a == 4 ){
    read_line_seg = int_b ;
    read_line_ptr = int_c ;
    read_line_max = int_d ;
    read_stdin ();
    int_ret = read_line_len ;
  }

  // restore ds and es for the caller
  // iret
  _ax = int_ret ;
  asm(" .byte 7 ; .byte 31 ; .byte 207 ; ");
}

void int_setup (){
  // interrupt 0x40 - each entry is 4 bytes
  // entries are offset then segment
  _p = 256 ;
  // NONSTANDARD: implementing addr of in userspace
  // by using a function's name as a normal variable
  int_x40 = int_x40 ;
  asm(" .byte 137 ; .byte 30 ; .byte 0 ; .byte 16 ; ");
  * _p = _ax ;
  _p = _p + 1 ;
  * _p = 0 ;
  _p = _p + 1 ;
  int_x41 = int_x41 ;
  asm(" .byte 137 ; .byte 30 ; .byte 0 ; .byte 16 ; ");
  * _p = _ax ;
  _p = _p + 1 ;
  * _p = 0 ;
}

int delay ;
int* magic_num_ptr ;
int main (){
  // set up stack pointer to point somewhere nicer - mov sp, 0x0F00
  asm(" .byte 188 ; .byte 0 ; .byte 15 ; ");
  // NONSTANDARD: main's addr is in ax
  asm(" .byte 163 ; .byte 0 ; .byte 16 ; ");
  main_addr = _ax ;

  // press enter...
  c = 112 ;
  print_char ();
  c = 114 ;
  print_char ();
  c = 101 ;
  print_char ();
  c = 115 ;
  print_char ();
  print_char ();
  c = 32 ;
  print_char ();
  c = 101 ;
  print_char ();
  c = 110 ;
  print_char ();
  c = 116 ;
  print_char ();
  c = 101 ;
  print_char ();
  c = 114 ;
  print_char ();
  c = 46 ;
  print_char ();
  print_char ();
  print_char ();

  // wait for input
  read_char ();

  // 0x7200
  local_stack = 29184 ;
  // 0x0F00 - 16 bytes
  dirent_file_name = 3840 ;

  init_fs ();

  c = 77 ;
  print_char ();
  // check the boot sector for the magic number that indicates that we already set things up
  // 0x7DB8
  magic_num_ptr = 32184 ;
  print_hex_val = * magic_num_ptr ;
  println_hex ();

  // 0x6699
  if( * magic_num_ptr != 26265 ){
    backup_and_overwrite ();
  }

  int_setup ();

  text_editor ();

  // UNREACHABLE
  while( 1 == 1 ){
  }
}
