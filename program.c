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
  // mov word [0x1000], ax
  _ax = ptr ;
  asm(" .byte 147 ; .byte 101 ; .byte 139 ; .byte 7 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
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

int* print_str_ptr ;
int _print_str_b ;
void print_string (){
  // prints a NULL terminated string to the serial output
  // ARGUMENTS: <global> print_string_src - pointer to the start of the string to print
  // RETURNS: NONE
  _print_str_b = * print_str_ptr & 255 ;
  while( _print_str_b != 0 ){
    c = _print_str_b ;
    print_char ();
    // hack to get a 1 byte offset
    _print_str_b = print_str_ptr ;
    print_str_ptr = _print_str_b + 1 ;
    _print_str_b = * print_str_ptr & 255 ;
  }
}

int print_str_len ;
void print_string_len (){
  while( 0 < print_str_len ){
    _print_str_b = * print_str_ptr & 255 ;
    c = _print_str_b ;
    print_char ();
    // hack to get a 1 byte offset
    _print_str_b = print_str_ptr ;
    print_str_ptr = _print_str_b + 1 ;
    print_str_len = print_str_len - 1 ;
  }
}


int max_head_num ;
int max_sector_num ;
int max_cylinder_num ;
void get_drive_stats (){
  asm(" .byte 180 ; .byte 8 ; .byte 178 ; .byte 128 ; .byte 205 ; .byte 19 ; .byte 81 ; .byte 82 ; ");
  // pop ax; move word [0x1000], ax
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  max_head_num = _ax >> 8 ;
  // pop ax; move word [0x1000], ax
  asm(" .byte 88 ; .byte 163 ; .byte 0 ; .byte 16 ; ");
  max_cylinder_num = ( ( _ax & 192 ) << 8 ) | ( ( _ax & 65280 ) >> 8 ) ;
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
  // set dx to XX80
  _io_dx = ( lba_to_chs_h << 8 ) | 128 ;
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

int* FAT ;
int* fat16_root_data ;
int start_sector ;
int first_data_offset ;
int root_dir_offset ;
int root_dir_entries ;
int fat1_offset ;
int fat2_offset ;
int* dirent_file_name ;

int* _find_file_name_ptr ;
int* _find_file_search_ptr ;
int find_file_idx ;
void find_file (){
  // ARGUMENTS: pointer to 12 bytes that contain the null terminated name of the file
  // RETURNS: pointer to the start of the fs metadata block that matched, or 0

  pop_local ();
  _find_file_name_ptr = local_val ;

  _find_file_search_ptr = fat16_root_data ;
  find_file_idx = 0 ;
  while( find_file_idx < root_dir_entries ){
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
    find_file_idx = find_file_idx + 1 ;
  }

  // if we got here, no file found
  local_val = 0 ;
  push_local ();
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
void seek_open_file (){
  // seeks to a specified logical sector in the currently open file
  // updates open_file_sector and open_file_fat_cluster
  // ARGUMENTS: <global> seek_sector - the absolute sector in the file to open
  // RETURNS: NONE
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
    c = 68 ;
    print_char ();
    print_hex_val = tmp ;
    print_hex ();
    print_hex_val = io_lba ;
    println_hex ();
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
  _set_file_count = 0 ;
  _set_file_ptr = open_file_metadata + 6 ;
  cluster = * _set_file_ptr ;
  while( _set_file_count < file_length ){
    _last_cluster = cluster ;
    next_cluster_get ();
    if( next_cluster < 0 ){
      // not enough clusters are allocated, allocate as many as needed
      while( _set_file_count < file_length ){
        allocate_new_cluster ();
        cluster = next_fat_idx ;
        _set_file_count = _set_file_count + 512 ;
      }
      // when this drops through, allocate_new_cluster has set next_cluster to 0xFFFF
    }
    if( 1 < next_cluster ){
      cluster = next_cluster ;
      _set_file_count = _set_file_count + 512 ;
    }
  }

  if( 1 < next_cluster ){
    // if there's still clusters remaining in the chain, free them
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
  print_hex_val = _create_file_ptr ;
  println_hex ();

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

int* write_file_buf ;
int  write_file_count ;
int  write_file_offset ;
int _write_file_start_sector ;
int _write_file_cluster ;
void write_file (){
  // writes the contents of a buffer into the open file at a specified offset in the file
  // expands the file if needed
  // ARGUMENTS:
  //   IMPLICIT OPEN FILE STATE
  //   <global> write_file_buf - the buffer to write into the file
  //   <global> write_file_count - the number of 16 bit words to write to the file
  //   <global> write_file_offset - the offset into the file to begin writing at. must be a multiple of 2, and gets clamped to the end of the file

  // save the logical sector in the file to restore after
  _write_file_start_sector = open_file_sector ;

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

  // byte offset to number of words needed to align to a sector
  // FIXME: why cant i shorten this with parens???
  memcpy_count = ( write_file_offset >> 1 ) & 255 ;
  memcpy_count = 256 - memcpy_count ;

  if( write_file_count < memcpy_count ){
    memcpy_count = write_file_count ;
  }

  _write_file_cluster = open_file_fat_cluster ;
  // don't do this alignment write if it's not needed
  if( memcpy_count != 0 ){
    memcpy_src = write_file_buf ;
    memcpy_dst = io_buf ;
    // memcpy_count gets destroyed by memcpy, so do this first
    write_file_buf = write_file_buf + memcpy_count ;
    write_file_count = write_file_count - memcpy_count ;
    memcpy ();
    io_lba = ( first_data_offset + _write_file_cluster ) - 2 ;
    write_sector ();
    cluster = _write_file_cluster ;
    next_cluster_get ();
    _write_file_cluster = next_cluster ;
  }

  while( 0 < write_file_count ){
    io_lba = ( first_data_offset + _write_file_cluster ) - 2 ;
    print_hex_val = io_lba ;
    println_hex ();
    read_sector ();
    memcpy_src = write_file_buf ;
    memcpy_dst = io_buf ;
    memcpy_count = write_file_count ;
    // write at most one sector at a time
    if( 256 < memcpy_count ){
      memcpy_count = 256 ;
    }
    write_file_buf = write_file_buf + memcpy_count ;
    write_file_count = write_file_count - ( memcpy_count << 1 ) ;
    memcpy ();
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

  // 0xD000
  FAT = 53248 ;
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
  // 0x2000 * 2 bytes = 0x4000 bytes
  memset_count = 8192 ;
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
    // FIXME: implement status 1: vacant entry by not printing
    if( _read_root_dirent_status == 0 ){
      print_str_ptr = fat16_root_data - 8 ;
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

  // 0x6000
  io_buf = 24576 ;
  // 0x8000
  fat16_root_data = 32768 ;
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
  * tmp = 2228 ;
  tmp = tmp + 1 ;
  * tmp = 5069 ;
  tmp = tmp + 1 ;
  * tmp = 13960 ;
  tmp = tmp + 1 ;
  * tmp = 31804 ;
  tmp = tmp + 1 ;
  * tmp = 51336 ;
  tmp = tmp + 1 ;
  * tmp = 16164 ;
  tmp = tmp + 1 ;
  * tmp = 15778 ;
  tmp = tmp + 1 ;
  * tmp = 41340 ;
  tmp = tmp + 1 ;
  * tmp = 32186 ;
  tmp = tmp + 1 ;
  * tmp = 14070 ;
  tmp = tmp + 1 ;
  * tmp = 31805 ;
  tmp = tmp + 1 ;
  * tmp = 57736 ;
  tmp = tmp + 1 ;
  * tmp = 49662 ;
  tmp = tmp + 1 ;
  * tmp = 180 ;
  tmp = tmp + 1 ;
  * tmp = 5770 ;
  tmp = tmp + 1 ;
  * tmp = 31804 ;
  tmp = tmp + 1 ;
  * tmp = 49918 ;
  tmp = tmp + 1 ;
  * tmp = 62198 ;
  tmp = tmp + 1 ;
  * tmp = 59016 ;
  tmp = tmp + 1 ;
  * tmp = 32946 ;
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
  tmp = tmp + 191 ;
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
  

int num ;
void read_num (){
  num = 0 ;
  while( 1 == 1 ){
    read_char ();
    is_num ();
    if( is != 1 ){
      return;
    }
    // num = 10 * num
    num = ( ( num << 2 ) + num ) << 1 ;
    num = num + ( c - 48 ) ;
  }
}

int _next_line ;
int* active_line ;

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


int text_editor_state ;
int* text_buf ;
int* metadata ;
int* _prev_metadata ;

int input_c ;
int _handled ;
void handle_input (){
  read_char ();
  // need this to not change during prints
  input_c = c ;
  _handled = 0 ;

  // command state
  if( text_editor_state == 0 ){
    // \n and \r at the start of a line should just be ignored in command mode
    if( ( input_c == 10 ) | ( input_c == 13 ) ){
      _handled = 1 ;
    }
    // p
    if( input_c == 112 ){
      // TODO: read length from the active line (it will always be in use)
      // and print it
      active_line = active_line + 3 ;
      print_str_len = * active_line >> 8 ;
      active_line = active_line - 1 ;
      print_str_ptr = * active_line ;
      print_string_len ();
      active_line = active_line - 2 ;
      _handled = 1 ;
    }
    // + or -
    if( ( input_c == 43 ) | ( input_c == 45 ) ){
      read_num ();
      if( num == 0 ){
        num = 1 ;
      }
      if( input_c == 45 ){
        num = 0 - num ;
      }
      advance_lines ();
      print_hex_val = active_line ;
      println_hex ();
      _handled = 1 ;
    }
    // i - enters line insertion mode
    if( input_c == 105 ){
      text_editor_state = 1 ;
      _handled = 1 ;
    }

    if( _handled == 0 ){
      // unknown input
      // ?
      c = 63 ;
      print_char ();
    }
  }
  // insert mode
  if( ( _handled == 0 ) & ( text_editor_state == 1 ) ){
    c = 73 ;
    print_char ();
  }
}


int* _line_start ;
int* _p ;
int _p_i ;
int _len ;

void text_editor (){
  // 0x9000
  metadata = 36864 ;
  // 0xB000
  text_buf = 45056 ;
  _prev_metadata = 0 ;

  // clear both metadata and text
  memset_ptr = metadata ;
  memset_count = 14336 ;
  memset_val = 0 ;
  memset ();

  // command state
  text_editor_state = 0 ;

  open_file_metadata = fat16_root_data ;
  open_file ();

  // 0x6000
  memcpy_src = io_buf ;
  memcpy_dst = text_buf ;
  memcpy_count = 256 ;
  memcpy ();

  _len = 0 ;
  _p = text_buf ;
  _line_start = _p ;
  while( _p < ( text_buf + 256 ) ){
    _len = _len + 1 ;
    _p_i = * _p & 255 ;
    if( _p_i == 10 ){
      c = 78 ;
      print_char ();
      print_hex_val = _p ;
      print_hex ();
      print_hex_val = _len ;
      println_hex ();

      * metadata = _prev_metadata ;
      // update next pointer of previous line
      if( _prev_metadata != 0 ){
        _prev_metadata = _prev_metadata + 1 ;
        * _prev_metadata = metadata ;
      }
      metadata = metadata + 1 ;
      * metadata = 0 ;
      metadata = metadata + 1 ;
      * metadata = _line_start ;
      metadata = metadata + 1 ;
      * metadata = _len << 8 ;
      metadata = metadata + 1 ;

      _prev_metadata = metadata - 4 ;
      _len = 0 ;
      _p_i = _p ;
      _line_start = _p_i + 1 ;
    }
    _p_i = _p ;
    _p = _p_i + 1 ;
  }

  // 0x9000
  metadata = 36864 ;
  active_line = metadata ;
  while( metadata != 0 ){
    print_hex_val = metadata ;
    print_hex ();
    metadata = metadata + 2 ;
    print_str_ptr = * metadata ;
    metadata = metadata + 1 ;
    print_str_len = * metadata >> 8 ;
    print_string_len ();
    metadata = metadata - 2 ;
    metadata = * metadata ;
  }

  while( 1 == 1 ){
    handle_input ();
  }
}

int delay ;
int* magic_num_ptr ;
int main (){
  // set up stack pointer to point somewhere nicer - mov sp, 0x0F00
  asm(" .byte 188 ; .byte 0 ; .byte 15 ; ");
  // NONSTANDARD: extract the address of main from ax
  // mov word [0x1000], ax
  asm(" .byte 163 ; .byte 0 ; .byte 16 ; ");
  main_addr = _ax ;

  delay = 65535 ;
  while( delay < 32767 ){
    delay = delay + 1 ;
  }

  // 0x7200
  local_stack = 29184 ;
  // 0x0F00 - 16 bytes
  dirent_file_name = 3840 ;

  init_fs ();

  // check the boot sector for the magic number that indicates that we already set things up
  // 0x7DB8
  magic_num_ptr = 32184 ;
  print_hex_val = * magic_num_ptr ;
  println_hex ();

  // 0x6699
  if( * magic_num_ptr != 26265 ){
    backup_and_overwrite ();
  }

  text_editor ();

  while( 1 == 1 ){
  }
}
