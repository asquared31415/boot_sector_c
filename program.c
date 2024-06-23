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
    _local_ptr = local_stack - local_idx ;
    // local_stack points to the NEXT pointer, so offset for that
    _local_ptr = _local_ptr + 65535 ;
    local_val = * _local_ptr ;
}

void write_local (){
    // ARGUMENTS:
    // - <GLOBAL> local_idx
    // - <GLOBAL> local_val
    // RETURNS: NONE
    _local_ptr = local_stack - local_idx ;
    // local_stack points to the NEXT pointer, so offset for that
    _local_ptr = _local_ptr - 1 ;
    * _local_ptr = local_val ;
}

int gs ;
int* ptr ;
int ptr_val ;
void wide_pointer_read (){
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
    // mov word [0x8000], ax
    _ax = ptr ;
    asm(" .byte 147 ; .byte 101 ; .byte 139 ; .byte 7 ; .byte 163 ; .byte 0 ; .byte 128 ; ");
}


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
  strcmp_diff = * _strcmp_lhs_ptr - * _strcmp_rhs_ptr ;
  strcmp_diff = strcmp_diff & 255 ;
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
      strcmp_diff = * _strcmp_lhs_ptr - * _strcmp_rhs_ptr ;
      strcmp_diff = strcmp_diff & 255 ;
  }
}

int port ;
int port_val ;
void inb (){
    _ax = port ;
    asm(" .byte 146 ; .byte 236 ; .byte 162 ; .byte 0 ; .byte 128 ; ");
    port_val = _ax ;
    port_val = port_val & 255 ;
}
void outb (){
    _ax = port ;
    asm(" .byte 146 ; ");
    _ax = port_val ;
    asm(" .byte 238 ; .byte 162 ; .byte 0 ; .byte 128 ; ");
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
    return;
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
    _print_hex_nibble = print_hex_val & 61440 ;
    _print_hex_nibble = _print_hex_nibble >> 12 ;
    // adjust letters forward into the letter region
    if( 10 <= _print_hex_nibble ){
      _print_hex_nibble = _print_hex_nibble + 7 ;
    }
    // shift from value into digit range
    c = _print_hex_nibble + 48 ;
    print_char ();
    print_hex_val = print_hex_val << 4 ;
    _print_hex_count = _print_hex_count + 1 ;
  }
}

int print_string_src ;
int _print_string_byte ;
void print_string (){
  // prints a NULL terminated string to the serial output
  // ARGUMENTS: <global> print_string_src - pointer to the start of the string to print
  // RETURNS: NONE
  _print_string_byte = * print_string_src & 255 ;
  while( _print_string_byte != 0 ){
    c = _print_string_byte ;
    print_char ();
    print_string_src = print_string_src + 1 ;
    _print_string_byte = * print_string_src & 255 ;
  }
}


int max_head_num ;
int max_sector_num ;
int max_cylinder_num ;
int _get_drive_stats_tmp ;
void get_drive_stats (){
  asm(" .byte 180 ; .byte 8 ; .byte 178 ; .byte 128 ; .byte 205 ; .byte 19 ; .byte 81 ; .byte 82 ; ");
  asm(" .byte 88 ; .byte 162 ; .byte 0 ; .byte 128 ; ");
  max_head_num = _ax & 255 ;
  asm(" .byte 88 ; .byte 162 ; .byte 0 ; .byte 128 ; ");
  max_cylinder_num = _ax & 192 ;
  max_cylinder_num = max_cylinder_num << 8 ;
  _get_drive_stats_tmp = _ax & 65280 ;
  _get_drive_stats_tmp = _get_drive_stats_tmp >> 8 ;
  max_cylinder_num = max_cylinder_num | _get_drive_stats_tmp ;
  max_sector_num = _ax & 63 ;
}

int int_div_lhs ;
int int_div_rhs ;
int int_div_quot ;
int int_div_rem ;
void int_div (){
    int_div_quot = 0 ;
    while( int_div_rhs < int_div_lhs ){
        int_div_lhs = int_div_lhs - int_div_rhs ;
        int_div_quot = int_div_quot + 1 ;
    }
    int_div_rem = int_div_lhs ;
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
    int_div_rhs = max_sector_num ;
    int_div ();
    _lba_to_chs_tmp = int_div_quot ;
    lba_to_chs_s = int_div_rem + 1 ;
    int_div_lhs = _lba_to_chs_tmp ;
    int_div_rhs = max_head_num + 1 ;
    int_div ();
    lba_to_chs_h = int_div_rem ;
    lba_to_chs_c = int_div_quot ;
}

int io_lba ;
int* io_buf ;
int _io_ax ;
int _io_cx ;
int _io_dx ;
int _io_tmp ;
void disk_io (){
  // reads or writes a sector from disk
  // ARGUMENTS:
  // <global> io_lba - the linear address of the sector to load
  // <global> _io_ax - either 513 to read or 769 to write
  // RETURNS: <global> io_buf - points to the loaded data
  lba_to_chs_lba = io_lba ;
  lba_to_chs ();
  _io_cx = lba_to_chs_c & 255 ;
  _io_cx = _io_cx << 8 ;
  _io_tmp = lba_to_chs_c >> 2 ;
  _io_tmp = _io_tmp & 192 ;
  _io_tmp = lba_to_chs_s | _io_tmp ;
  _io_cx = _io_cx | _io_tmp ;
  _io_dx = lba_to_chs_h << 8 ;
  _io_dx = _io_dx | 128 ;
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
}

void read_sector (){
  _io_ax = 513 ;
  disk_io ();
}

void write_sector (){
  _io_ax = 769 ;
  disk_io ();
}

int* fat16_root_data ;
int start_sector ;
int first_data_offset ;

int* dirent_file_name ;

int* _find_file_name_ptr ;
int* _find_file_search_ptr ;
void find_file (){
  // ARGUMENTS: pointer to 12 bytes that contain the null terminated name of the file
  // RETURNS: pointer to the start of the fs metadata block that matched, or 0

  pop_local ();
  _find_file_name_ptr = local_val ;

  _find_file_search_ptr = fat16_root_data ;
  // 0x80 entries times 0x10 bytes per entry means the end of the fs data is at 0x5FFF
  while( _find_file_search_ptr < 24575 ){
    // search for a fs entry with the same name
    strcmp_lhs = _find_file_name_ptr ;
    strcmp_rhs = _find_file_search_ptr ;
    strcmp ();
    if( strcmp_diff == 0 ){
      local_val = _find_file_search_ptr ;
      push_local ();
      return;
    }

    // increment to the next fs entry (16 bytes each)
    _find_file_search_ptr = _find_file_search_ptr + 8 ;
  }

  // if we got here, no file found
  local_val = 0 ;
  push_local ();
}


int* open_file_metadata ;
int  open_file_sector ;
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
  io_lba = * open_file_metadata - 2 ;
  io_lba = io_lba + first_data_offset ;
  read_sector ();

  open_file_metadata = open_file_metadata + 1 ;
  open_file_length = * open_file_metadata ;

  // move back to start of metadata
  open_file_metadata = open_file_metadata - 7 ;
}

int seek_sector ;
void seek_open_file (){
  // seeks to a specified sector in the currently open file
  // ARGUMENTS: <global> seek_sector - the absolute sector in the file to open
  // RETURNS: NONE
}

int* write_file_buf ;
int  write_file_count ;
int  write_file_offset ;
void write_file (){
  // writes the contents of a buffer into the open file at a specified offset in the file
  // expands the file if needed
  // ARGUMENTS:
  //   IMPLICIT OPEN FILE STATE
  //   <global> write_file_buf - the buffer to write into the file
  //   <global> write_file_count - the number of 16 bit words to write to the file
  //   <global> write_file_offset - the offset into the file to begin writing at. must be a multiple of 2, and gets clamped to the end of the file

  // save the currently open sector to restore it later
  seek_sector = open_file_sector ;

  if( open_file_length < write_file_offset ){
    write_file_offset = open_file_length ;
  }

  // maximum amount needed to align to a sector
  memcpy_count = open_file_length & 255 ;
  memcpy_count = 512 - memcpy_count ;
  // bytes -> words
  memcpy_count = memcpy_count >> 1 ;

  if( write_file_count < memcpy_count ){
    memcpy_count = write_file_count ;
  }

  memcpy_src = write_file_buf ;
  memcpy_dst = io_buf ;
  // memcpy_count gets destroyed by memcpy, so do this first
  write_file_buf = write_file_buf + memcpy_count ;
  write_file_count = write_file_count - memcpy_count ;
  io_buf = io_buf + memcpy_count ;
  memcpy ();
  // TODO: SAVE THINGS

  // now save sectors at a time
  int_div_lhs = write_file_count ;
  int_div_rhs = 256 ;
  int_div ();
  while( 0 < int_div_quot ){
    memcpy_src = write_file_buf ;
    memcpy_dst = io_buf ;
    memcpy_count = 256 ;
    memcpy ();
    // TODO: SAVE THINGS
    write_file_buf = write_file_buf + 256 ;
    int_div_quot = int_div_quot - 1 ;
  }

  if( 0 < int_div_rem ){
    memcpy_src = write_file_buf ;
    memcpy_dst = io_buf ;
    memcpy_count = int_div_rem ;
    memcpy ();
    // SAVE IT HERE
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

int root_dir_offset ;
int fat1_offset ;

int _read_root_count ;
int _read_root_cluster ;
int _read_root_dirent_status ;
void read_root (){
  start_sector = 1 ;
  root_dir_offset = 3 ;
  fat1_offset = 1 ;
  first_data_offset = 11 ;

  c = 48 ;
  print_char ();
  io_lba = start_sector + root_dir_offset ;
  read_sector ();

  // 0x1000
  fat16_root_data = 4096 ;
  memset_ptr = fat16_root_data ;
  memset_val = 0 ;
  // 0x2000 * 2 bytes = 0x4000 bytes
  memset_count = 8192 ;
  memset ();

  _read_root_count = 0 ;
  while( _read_root_count < 128 ){
    local_val = io_buf ;
    push_local ();
    read_dir_entry ();
    pop_local ();

    _read_root_dirent_status = local_val ;
    // status 2 is end of directory
    if( _read_root_dirent_status == 2 ){
      // 0x1000
      fat16_root_data = 4096 ;
      return;
    }
    // FIXME: implement status 1: vacant entry by not printing
    if( _read_root_dirent_status == 0 ){
      print_string_src = fat16_root_data - 8 ;
      print_string ();
      c = 32 ;
      print_char ();

      // print the start cluster
      print_hex_val = fat16_root_data - 2 ;
      print_hex_val = * print_hex_val ;
      print_hex ();
      c = 32 ;
      print_char ();

      // print the file size
      print_hex_val = fat16_root_data - 1 ;
      print_hex_val = * print_hex_val ; 
      print_hex ();

      c = 10 ;
      print_char ();

      // each entry is 32 bytes
      io_buf = io_buf + 16 ;
      _read_root_count = _read_root_count + 1 ;
    }
  }

  // 0x1000
  fat16_root_data = 4096 ;
}

int delay ;
int main (){
  // 0x7C00 - used to be boot sector, but control has been transferred permanently
  local_stack = 31744 ;

  // 0x0F00 - 16 bytes
  dirent_file_name = 3840 ;

  delay = 0 ;
  while( delay < 65000 ){
    delay = delay + 1 ;
  }
  // read the root directory of the FAT16 tables and create the in-memory representation
  read_root ();
  local_val = fat16_root_data + 8 ;
  push_local ();
  find_file ();
  pop_local ();

  open_file_metadata = local_val ;
  open_file ();

  print_hex_val = open_file_metadata - 6 ;
  print_hex ();

  asm(" .byte 235 ; .byte 254 ; ");
}
