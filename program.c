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

int buf ;
int read_byte_val ;
int* _read_byte_ptr ;
void read_byte_buf (){
    _read_byte_ptr = buf ;
    read_byte_val = * _read_byte_ptr ;
    read_byte_val = read_byte_val & 255 ;
    buf = buf + 1 ;
}


int* local_stack ;


int start_sector ;
int first_data_offset ;

int* dir_entry_pointer ;
void read_dir_entry (){

}

int root_dir_offset ;
int fat1_offset ;

int* _read_root_sector_buf ;
int _read_root_count ;
int _read_root_cluster ;
void read_root (){
    start_sector = 1 ;
    root_dir_offset = 3 ;
    fat1_offset = 1 ;
    first_data_offset = 11 ;
    _read_root_sector_buf = 28672 ;

    load_sector_lba = start_sector + root_dir_offset ;
    load_sector_buf = _read_root_sector_buf ;
    load_sector ();

    buf = _read_root_sector_buf ;
    _read_root_count = 0 ;
    while( _read_root_count < 11 ){
        read_byte_buf ();
        c = read_byte_val ;
        print_char ();
        _read_root_count = _read_root_count + 1 ;
    }

    _read_root_count = 0 ;
    while( _read_root_count < 15 ){
        read_byte_buf ();
        _read_root_count = _read_root_count + 1 ;
    }
    _read_root_sector_buf = buf ;

    _read_root_cluster = * _read_root_sector_buf ;

    load_sector_lba = _read_root_cluster - 2 ;
    load_sector_lba = load_sector_lba + start_sector ;
    load_sector_lba = load_sector_lba + first_data_offset ;
    _read_root_sector_buf = 28672 ;
    load_sector_buf = _read_root_sector_buf ;
    load_sector ();
}

int delay ;
int delay2 ;
int delay3 ;
int main (){
    delay = 0 ;
    while( delay < 65000 ){
        delay = delay + 1 ;
    }
    // this is a fun comment :3

    read_root ();

    asm(" .byte 235 ; .byte 254 ; ");
}
