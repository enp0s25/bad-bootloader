; ==========================================================
; =- Stuff needed to load kernel.bin from FAT32 partition -=
; ==========================================================

; buffer for data that need to be parsed
%assign aaaa $-$$ + 0x7c00
%warning buffer starts at aaaa
BUFF times 512 db 0

; START structs
; struct of FAT32 header
f32head:
times 11 db 0
.bytes_per_sector dw 0
.sectors_per_cluster db 0
.reserved_sectors dw 0
.num_tables db 0
times 19 db 0
.sectors_per_fat32 dd 0
times 4 db 0
.root_dir_cluster dd 0

; struct of kernel.bin file entry
file_entry:
.filename times 8 db 0
.extension db 0,0,0
times 9 db 0
.first_cluster_msb dw 0
dd 0
.first_cluster_lsb dw 0
.file_size dd 0
;.actual_first_cluster dd 0
; END structs

; some info for loading kernel file
current_kernel_mem dw 0x0000
gottaloadcluster dd 0

; load kernel
kernel_load:
  xor ax, ax
  push es
  mov es, ax

  ; load FAT 32 head
  mov eax, dword [MBR.entry1.LBA_start]
  mov dl, byte [disknum]
  mov bx, BUFF
  mov cx, 1
  call read_sectors

  ; copy F32head from buffer to struct
  mov si, BUFF
  mov di, f32head
  mov ecx, 48
  call memcpy

  ;load Root dir
  xor ax, ax
  mov es, ax
  ; loop through root directory
  .root_dir_loop:
    inc byte [.rootdir_loops_count]
    mov al, [f32head.sectors_per_cluster]
    inc al
    cmp byte [.rootdir_loops_count], al
    je .file_locate_error ; file not found
    mov eax, dword [f32head.sectors_per_fat32]
    xor dx, dx
    mov dl, byte [f32head.num_tables]
    mul dx
    add eax, dword [MBR.entry1.LBA_start]
    xor edx, edx
    mov dx, word [f32head.reserved_sectors]
    add eax, edx
    mov bx, BUFF
    mov cx, 1
    mov dl, byte [disknum]
    call read_sectors ; load part of root dir into buffer

    ;search for kernel file name in buffer
    mov si, BUFF
    mov di, kernel_name
    call searchstr
    cmp ax, -1
    je .root_dir_loop ; jump to loop if not found

  ; if found copy it to file entry struct
  mov si, ax
  mov di, file_entry
  mov cx, 32
  call memcpy

  ; xor eax, eax
  ; mov ax, word [f32head.reserved_sectors]
  ; add eax, dword [MBR.entry1.LBA_start]
  ; mov bx, BUFF
  ; mov cx, 1
  ; mov dl, byte [disknum]
  ; call read_sectors

  call kernelfile_load 
  jmp .done 

.file_locate_error:
  mov ax, 0xb800
  mov es, ax
  mov si, error_nofile
  mov ah, 0x02
  xor bl, bl
  call sprint

.done:
  pop es
  ret

.rootdir_loops_count db 0


memcpy:
  lodsb ; load data from ds:si to al, inc si
  stosb ; save data from al to es:di, inc di
  dec ecx
  jnz memcpy
  ret

searchstr:
  mov word [.var_si], si
  mov word [.var_di], di

.loop1:
  cmp word [.var_i], 512
  je .ripdone
  inc word [.var_i]
  inc si
  mov bx, si
  .loop2:
    cmp word [.var_j], 12
    je .reset2
    inc word [.var_j]
    mov al, [si]
    ; cmp al, 0
    ; je .zero
    ; inc word [reg16]
    ; .zero:
    cmp al, [di]
    jne .reset2
    inc si
    inc di
    cmp word [.var_j], 11
    je .done
    
    jmp .loop2

  .reset2:
    mov word [.var_j], 0
    mov di, word [.var_di]
    mov si, word [.var_si]
    add si, word [.var_i]  

  jmp .loop1

.ripdone:
  mov bx, -1
.done:
  mov ax, bx
  ret

.var_i dw 0
.var_j db 0
.var_si dw 0
.var_di dw 0



kernelfile_load:
  xor eax, eax
  mov ax, word [file_entry.first_cluster_msb]
  shl eax, 16
  mov ax, word [file_entry.first_cluster_lsb]
  

  follow_cluster_loop:

    xor edx, edx

    ;div edx
    mov dword [gottaloadcluster], eax
    mov dl, al
    and dl, 01111111b
    shr eax, 7
    push dx

    xor ebx, ebx
    mov bx, word [f32head.reserved_sectors]
    add eax, ebx
    add eax, dword [MBR.entry1.LBA_start]

    ; eax has FAT sector with file first entry
    mov bx, BUFF
    mov cx, 1
    mov dl, byte [disknum]
    call read_sectors

    pop ax
    shl ax, 2
    add bx, ax
    mov edx, dword [ebx]
    cmp edx, 0xfffffff
    ; cmp edx, 0xFFFFFF0F
    je loop_end_found
    ; %assign break $-$$ + 0x7c00
    ; %warning bp at break
    push edx
    mov [gottaloadcluster], eax

    add dword [current_kernel_mem], 4096
    pop eax

    jmp follow_cluster_loop


loop_end_found:
  call load_stpid_cluster
  jc ice_red

  ret

load_stpid_cluster:
  push es
  xor eax, eax
  mov ebx, eax
  mov edx, eax

  mov eax, [f32head.sectors_per_fat32]
  mov dl, [f32head.num_tables]
  mul edx
  mov ebx, eax

  mov eax, dword [gottaloadcluster]
  sub eax, 2
  mov dl, byte [f32head.sectors_per_cluster]
  mul edx
  add eax, ebx

  xor ebx, ebx
  mov bx, word [f32head.reserved_sectors]
  add eax, ebx
  add eax, dword [MBR.entry1.LBA_start]

  mov bx, 0
  mov es, bx
  ; mov bx, word [current_kernel_mem]
  mov bx, 0x500
  mov dl, byte [disknum]
  xor cx, cx
  mov cl, byte [f32head.sectors_per_cluster]
  call read_sectors
  jc ice_red

  mov esi, 0x500
  mov edi, 0x100000
  add di, [current_kernel_mem]
  mov ax, word [f32head.bytes_per_sector]
  xor dx, dx
  mov dl, byte [f32head.sectors_per_cluster]
  mul dx
  xor ecx, ecx
  mov cx, ax
  a32 o32 rep movsb
.done:
  pop es
  ret


error_nofile db "Can't locate file!", 0x0A, 0
kernel_name db 'KERNEL  BIN', 0x00