// INFINITE LOOP (EB FE - jmp $)
asm(" .byte 235 ; .byte 254 ; ");

int* memset_ptr ;
int memset_word ;
int memset_count ;
void memset (){
    while( memset_count > 0 ){
        * memset_ptr = memset_word ;
        memset_ptr = memset_ptr + 1 ;
    }
}

int* _init_fat_bpb_ptr ;
void init_fat_bpb (){
    _init_fat_bpb_ptr = 28672 ;
    * _init_fat_bpb_ptr = 65259 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 29584 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 25445 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 28532 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 25458 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 32 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 258 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 1 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 32770 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 32512 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 63488 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 1 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 62 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 124 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 1 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 0 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 0 ;
    _init_fat_bpb_ptr = _init_fat_bpb_ptr + 1 ;
    * _init_fat_bpb_ptr = 0 ;
}

// GET DRIVE OP STATUS
asm(" .byte 180 ; .byte 1 ; .byte 178 ; .byte 128 ; .byte 205 ; .byte 19 ; ");
