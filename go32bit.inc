;
; Less go to 32bit mode
;
less_go_32:
  ; start by enabling A20 line
  call enable_a20_debug

  cli
  lgdt [GDT]
  mov eax, cr0
  or al, 1
  mov cr0, eax

  jmp 0x08:.pmode

  .pmode:					; Now in protected mode

  mov ax, 0x10
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax

  mov eax, cr0			; Exit protected mode
  and eax, 11111110b
  mov cr0, eax

  jmp 0x0000:.unreal_mode

  .unreal_mode:			; Now in Unreal Mode

  xor ax, ax
  mov ds, ax
  mov es, ax
  mov fs, ax
  mov gs, ax
  mov ss, ax
  mov sp, 0x7BEE

  mov ax, 0xb800
  mov es, ax
  mov si, umodemsg
  mov ah, 0x0F
  mov bl, 0
  call sprint 
  
  sti						; Enable interrupts

  ret

; includes needed for enabling unreal mode
%include 'a20.inc'
%include 'gdt.inc'


a20msg db 'A20 active', 0x0A, 0x00
pmodemsg db 'We are now protected', 0x0A, 0x00
umodemsg db "This mode isn't real", 0x0A, 0x00