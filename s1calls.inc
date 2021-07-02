;========================================
; Calls
;========================================

clear_screen_l: ; set screen to 80x50 chars
  mov ax, 0x0000 ; set 40x25 chars
  int 0x10
  mov ah, 1   ; hide cursor
	mov cx, 0x2607
	int 0x10
  mov byte [xpos], 0  ;reset cursor
  mov byte [ypos], 0
  ret

clear_screen_s:
  mov ax, 0x0003 ;set 80x25 chars
  int 0x10
  mov ah, 1   ; hide cursor
  mov cx, 0x2607
	int 0x10
  mov byte [xpos], 0 ;reset cursor
  mov byte [ypos], 0
  ret

; bl=0 default, bl=1 fast, bl=2 keepBG
; AH if for formatting BG, FG
; 0 = black, 1 = blue, 2 = green, 3 = cyan, 4 = red, 5 - Magenta, 6 - Brown, 7 - Light Grey, 8 - Dark Grey, 9 - Light Blue, 10/a - Light Green, 11/b - Light Cyan, 12/c - Light Red, 13/d - Light Magenta, 14/e - Light Brown, 15/f – White.
dochar:
  call cprint         ; print one character
sprint:
  or bl, bl ;check if bl=0
  jnz .nosleep
  sleep 0x0, 20000 ; sleep
.nosleep:
  lodsb      ; string char to AL
  cmp al, 0  ; is it zero?
  je .done   ; if yes, we're done
  cmp al, 0x0A ; check if newline
  jne dochar
  add byte [ypos], 1   ;down one row
  mov byte [xpos], 0   ;back to left
  jmp sprint
.done:
  ret

; print char
cprint:   
  mov cx, ax    ; save char/attribute
  movzx ax, byte [ypos]
  mov dx, 160   ; 2 bytes (char/attrib)
  mul dx      ; for 80 columns
  movzx dx, byte [xpos]
  shl dx, 1    ; times 2 to skip attrib

  mov di, 0        ; start of video memory
  add di, ax      ; add y offset
  add di, dx      ; add x offset
  cmp bl, 2 ;check if bl=2
  jne .nokeep
  mov byte ah, [es:di+1] ;load original formatting
  and ah, 0xF0 ; remove old FG
  and ch, 0x0F ; remove prevoius bg (if any)
  or ah, ch ; add new FG
  mov al, cl
  jmp .write

.nokeep:
  mov ax, cx        ; restore char/attribute
.write:
  stosw              ; write char/attribute
  add byte [xpos], 1  ; advance to right

  ret

gchar:
  mov ah, 1
  mov al, 0
  int 0x16   ; scan for keypress
  jz .done  ; ret if none
.clearbuf:
  ; push ax
  mov bx, [gs:001Ah] ; get head of keyboard buffer
  push word [gs:bx]
  mov word [gs:bx], 0x0000 ; zero out key
  xor ah, ah ; remove key from buffer
  int 0x16
  pop ax

.done:
  ret ; char in al


read_sectors:

; EAX = LBA sector to load
; DL = Drive number
; ES = Buffer segment
; BX = Buffer offset
; CX = Sector count

; set CF on error

push si
push es
pop word [.target_segment]
mov word [.target_offset], bx
mov dword [.lba_address_low], eax
mov word [.count], cx

xor esi, esi
mov si, .da_struct
mov ah, 0x42

clc										; Clear carry for int 0x13 because some BIOSes may not clear it on success

int 0x13								; Call int 0x13
.done:
pop si
ret

align 4
%assign dastructaddr 0x7c00 + $-$$
%warning da at dastructaddr
.da_struct:
    .packet_size        db  16
    .unused             db  0
    .count              dw  1
    .target_offset      dw  0
    .target_segment     dw  0
    .lba_address_low    dd  0
    .lba_address_high   dd  0

;------------------------------------
; DEBUG registers
printreg16:
  push ax
  mov di, outstr16 ; buffer for hex string
  mov ax, [reg16] ; content to write as hex
  mov si, hexstr ; hex chars
  mov cx, 4   ;four places
.hexloop:
  rol ax, 4   ;leftmost will
  mov bx, ax   ; become
  and bx, 0x0f   ; rightmost
  mov bl, [si + bx] ;index into hexstr
  mov [di], bl
  inc di
  dec cx
  jnz .hexloop

  mov si, outstr16
  ;mov ah, 0x70 ; grey bg
  pop ax
  mov bl, 1
  call sprint ; print it

  ret


hexstr   db '0123456789ABCDEF' ; hex chars
outstr16   db '0000', 0  ;register value string
reg16   dw    0  ; pass values to printreg16
;------------------------------------