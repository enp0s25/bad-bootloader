
%macro sleep 2
	; Use BIOS interrupt to sleep
	push dx
  push cx
  push ax
	mov ah, 86h
	mov cx, %1
	mov dx, %2
	int 15h
  pop ax
  pop cx
	pop dx
%endmacro

org 0x7C00
bits 16


start:
  xor ax, ax   ; make it zero
  mov ds, ax   ; DS=0
  mov ss, ax   ; stack starts at 0
  mov sp, 0x7BF0   ; stack address
  mov ax, 0xb800  ; video memory
  mov es, ax  ; put videomem to es
  mov byte [disknum], dl ; save number of drive where we were booted from

  call clear_screen_s ; clear screen and set it to 80x25

; [Welcome message] ==================================
  mov si, welcome ; print welcome message
  mov ah, 0x0F ; formatting for sprint
  xor bl, bl
  call sprint

; ; load other sectors from disk
; CHS_LOAD:
;   xor ax, ax
;   mov es, ax                      ; set to 0
; 	mov ah, 0x02                    ; load second stage to memory
; 	mov al, 12                      ; numbers of sectors to read into memory
;   mov byte dl, [disknum]          ; sector read from fixed/usb disk
; 	mov ch, 0                       ; cylinder number
; 	mov dh, 0                       ; head number
; 	mov cl, 2                       ; sector number
; 	mov bx, stage2                  ; load into es:bx segment :offset of buffer
; 	int 0x13                        ; disk I/O interrupt

;   mov ax, 0xb800
;   mov es, ax

;   jmp stage2 ; goto kernel

; use LBA to load from disk
LBA_LOAD:
  mov ax, 0x7e0
  mov es, ax ; data will be loaded to memory after 1st sector
  xor bx, bx
  mov cx, 7 ; load 7 sectors (8kb)
  mov ax, 1 ; start at sector 1
  mov dl, [disknum]
  call read_sectors

  mov ax, 0xb800 ; set es back to videomem
  mov es, ax

  jmp stage2 ; goto stage 2

; things that can print colored message and stop CPU
ice_red:
  mov ah, 0x04
  jmp freeze

ice_lblue:
  mov ah, 0x09
  jmp freeze

ice_green:
  mov ah, 0x02
  jmp freeze

freeze:
  ; print msg
  mov dx, 0xb800
  mov es, dx
  mov si, friizu
  xor bl, bl
  call sprint
  ; actual freeze
  cli
  hlt



disknum db 0 ; drive ID
welcome db 'boot is good', 0x0A, 0x00
xpos db 0
ypos db 0
friizu db 'freeze', 0x0A, 0x00

;calls used in 1st stage + others that fit into 1st sector are here
%include 's1calls.inc'

; print amount of free bytes in boot sector when compiling
%assign codefree 440-($-$$)
%warning codefree bytes free in boot sector
times codefree db 0

; MBR:
; this should be reserved, and ORed with the actual MBR of install drive
MBR.UID dd 0
MBR.reserved dw 0
MBR.entry1:
  .flags db 0x80
  .CHS_start db 4,1,4
  .type db 0xB
  .CHS_end db 0xFE, 0xC2, 0xFF
  .LBA_start dd 2048
  .num_sectors dd 0x1f5800


times 510-($-$$) db 0 ; 1st sector padding
dw 0xAA55 ; some BIOSes require this signature

; more includes
%include 'go32bit.inc'
%include 'disk.inc'

; start of 2nd stage
stage2:
  mov si, s2succcess ; print stage 2 welcome message
  mov ah, 0x0F ; formatting for sprint
  xor bl, bl
  call sprint

  ; enable unreal mode, load kernel
  call less_go_32
  call kernel_load

  mov ax, 0xb800
  mov es, ax
  mov si, jumping ; print message
  mov ah, 0x0F ; formatting for sprint
  xor bl, bl
  call sprint

; set up protected mode right before jump
enter_pmode:

  cli ; Disable interrupts

  mov eax, cr0 ; enter pmode
  or eax, 0x00000001
  mov cr0, eax

  jmp 0x18:.pmode

  bits 32
  .pmode:	; Now in protected mode
  mov ax, 0x20
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  mov esp, 0xEFFFF0

  xor eax, eax
  xor ebx, ebx
  xor ecx, ecx
  xor edx, edx
  xor esi, esi
  xor edi, edi
  xor ebp, ebp

  jmp 0x100000 ; goto bootfile entry

s2succcess db 'but stage 2 is better', 0x0A, 0x00
getmemerr db "can't get mem", 0x0A, 0x00
jumping db 'jumping...', 0x0a, 0

%assign sector1 4096-($-$$)
%warning sector1 bytes free in 1st sector

times sector1 db 0