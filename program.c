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
int print_char (){
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

int load_sector_lba ;
int* load_sector_buf ;
int _load_sector_ax ;
int _load_sector_cx ;
int _load_sector_dx ;
int _load_sector_tmp ;
void load_sector (){
    // loads a sector from disk
    // ARGUMENTS: <GLOBAL> load_sector_lba - the linear address of the sector to load
    // RETURNS: <GLOBAL> load_sector_buf - points to the loaded data
    _load_sector_ax = 513 ;
    lba_to_chs_lba = load_sector_lba ;
    lba_to_chs ();
    _load_sector_cx = lba_to_chs_c & 255 ;
    _load_sector_cx = _load_sector_cx << 8 ;
    _load_sector_tmp = lba_to_chs_c >> 2 ;
    _load_sector_tmp = _load_sector_tmp & 192 ;
    _load_sector_tmp = lba_to_chs_s | _load_sector_tmp ;
    _load_sector_cx = _load_sector_cx | _load_sector_tmp ;
    _load_sector_dx = lba_to_chs_h << 8 ;
    _load_sector_dx = _load_sector_dx | 128 ;
    // 0x6000
    load_sector_buf = 24576 ;
    _ax = load_sector_buf ;
    asm(" .byte 80 ; ");
    _ax = _load_sector_dx ;
    asm(" .byte 80 ; ");
    _ax = _load_sector_cx ;
    asm(" .byte 80 ; ");
    _ax = _load_sector_ax ;
    asm(" .byte 80 ; ");
    asm(" .byte 88 ; .byte 89 ; .byte 90 ; .byte 91 ; .byte 205 ; .byte 19 ; ");
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


int* load_file_metadata ;
void load_file (){
  // loads a file from disk into memory at load_sector_buf (note: cannot set this address)
  // ARGUMENTS: <global> load_file_metadata - pointer to the in memory fs metadata describing the file to load
  // RETURNS: NONE

  // read cluster number
  load_file_metadata = load_file_metadata + 6 ;
  load_sector_lba = * load_file_metadata - 2 ;
  load_sector_lba = load_sector_lba + first_data_offset ;
  load_sector ();
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

  load_sector_lba = start_sector + root_dir_offset ;
  load_sector ();

  // 0x1000
  fat16_root_data = 4096 ;
  memset_ptr = fat16_root_data ;
  memset_val = 0 ;
  // 0x2000 * 2 bytes = 0x4000 bytes
  memset_count = 8192 ;
  memset ();

  _read_root_count = 0 ;
  while( _read_root_count < 128 ){
    local_val = load_sector_buf ;
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
      load_sector_buf = load_sector_buf + 16 ;
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

  print_hex_val = 999 ;
  print_hex ();

  load_file_metadata = local_val ;
  load_file ();
  asm(" .byte 235 ; .byte 254 ; ");

  print_hex_val = local_val ;
  print_hex ();

  asm(" .byte 235 ; .byte 254 ; ");
}
