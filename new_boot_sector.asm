BITS 16
org 0x7C00

main:
    xor ax, ax
    mov es, ax

    ;; get drive parameters to get the CHS
    mov ah, 0x08
    int 0x13                    ; dl is still set to 0x80
    mov byte [max_head_num], dh
    mov al, cl
    and al, 0b0011_1111
    mov byte [max_sector_num], al

    ;; LBA to CHS
    mov ax, word [program_lba]
    div byte [max_sector_num]
    mov cl, ah
    inc cl                      ; sector
    mov ah, 0x00                ; use quotient of division in a div again
    mov dl, byte [max_head_num]
    inc dl
    div dl
    mov dh, ah                  ; head
    mov dl, 0x80                ; disk 0x80
    mov ch, al                  ; cylinder

    mov ax, 0x0228              ; 0x02 = read, 0x28 sectors = 0x5000 bytes
    mov bx, 0x1000              ; program expects to be positioned at 0x1000
    int 0x13
    jc $                        ; infinite loop on error to avoid corruption
    jmp word [main_addr]

max_head_num: db 0x00
max_sector_num: db 0x00

TIMES 0x1B8-($-$$) db 0x00
MAGIC_NUM:  dw 0x6699

TIMES 0x1BA-($-$$) db 0x00

;; TO BE FILLED
program_lba:    dw 0x0000
main_addr:      dw 0x0000

TIMES 0x1BE-($-$$) db 0x00

; ====================================================================
; PARTITION TABLE ABSTRACTIONS (see next section for actual tables)
; ====================================================================

; Geometry of the target disk.
; The USB that is in use has 1015 cylinders, 124 heads, and 62 sectors and contains 7810176 sectors total.
%define NUM_CYLINDERS 1015
%define NUM_HEADS      124
%define NUM_SECTORS     62

; Converts a LBA passed as an integer to a sequence of 3 CHS bytes in BIOS format.
%macro LBA_TO_CHS_BYTES 1
    ; Head number.
    db (%1 / NUM_SECTORS) % NUM_HEADS
    ; Cylinder is split across the next two bytes.
    %assign %%cylinder_val %1 / (NUM_HEADS * NUM_SECTORS)

    ; High 2 bits of cylinder and all 6 bits of sector.
    db (%%cylinder_val >> 2) | ((%1 % NUM_SECTORS) + 1)

    ; Truncate to get low 8 bits of cylinder.
    db %%cylinder_val
%endmacro

%macro PART 3
    %%.attributes: db %1
    %%.CHS_start: LBA_TO_CHS_BYTES %2
    %%.system_id: db 0x00
    %%.CHS_end: LBA_TO_CHS_BYTES %eval(%2 + %3)
    %%.LBA_START: dd %2
    %%.LBA_COUNT: dd %3
%endmacro

; ====================================================================
; PARTITION TABLES
; ====================================================================
; NOTE: the first sector of the device does not need to be within a partition

; FAT16 partition
part0: PART 0x00,0x01,0x3FFF

part1: TIMES 16 db 0x00
part2: TIMES 16 db 0x00
part3: TIMES 16 db 0x00

; Partition table ends just before boot signature
dw 0xAA55
