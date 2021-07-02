a20_check:

; *************************************************
;     Checks if the A20 address line is enabled
; *************************************************
; OUT:
; Carry if disabled, cleared if enabled

push ax										; Save registers
push bx
push es
push fs

xor ax, ax									; Set ES segment to zero
mov es, ax
not ax										; Set FS segment to 0xFFFF
mov fs, ax

mov ax, word [es:0x7DFE]					; Check using boot signature
cmp word [fs:0x7E0E], ax					; If A20 is disabled, this should be the
											; same address as the boot signature
je .change_values							; If they are equal, check again with another value

.enabled:

clc											; A20 is enabled, clear carry flag
jmp .done

.change_values:

mov word [es:0x7DFE], 0x1234				; Change the value of 0000:7DFE to 0x1234
cmp word [fs:0x7E0E], 0x1234				; Is FFFF:7E0E changed as well?
jne .enabled								; If it is, A20 is enabled

stc											; Otherwise set carry

.done:

mov word [es:0x7DFE], ax					; Restore boot signature
pop fs										; Restore registers
pop es
pop bx
pop ax
ret											; Exit routine



enable_a20_debug:
  mov si, .a20status
  mov ah, 0x0F
  xor bl, bl
  call sprint
  call a20_check
  jc .isdisabled

.isenabled:
  mov si, .a20enabled ; print message
  mov ah, 0x02 ; formatting for sprint 0x02
  xor bl, bl
  call sprint
  jmp .done

.isdisabled:
  mov si, .a20disabled ; print message
  mov ah, 0x04 ; formatting for sprint
  xor bl, bl
  call sprint

; try using BIOS
  call a20_print_attempt
  mov si, .a20BIOS ; print message
  mov ah, 0x0F ; formatting for sprint
  xor bl, bl
  call sprint

  call a20_bios
  call a20_print_status
  call a20_check
  jnc .done

; try using keyboard
  call a20_print_attempt
  mov si, .a20KB ; print message
  mov ah, 0x0F ; formatting for sprint
  xor bl, bl
  call sprint

  call a20_keyboard
  call a20_print_status

.done:
  ret

.a20status db 'A20 status: ', 0x00
.a20enabled db 'enabled', 0x0A, 0x00
.a20disabled db 'disabled', 0x0A, 0x00
.a20BIOS db 'BIOS: ', 0x00
.a20KB db 'keyboard controller: ', 0x00
a20attempt db 'Attempting to activate using ', 0x00
pushc db 0
popc db 0

a20_print_attempt:
  mov si, a20attempt ; print message
  mov ah, 0x0F ; formatting for sprint
  xor bl, bl
  call sprint
  ret


a20_print_status:
  jc .fail
.success:
  mov si, .successmsg
  mov ah, 0x02 ; formatting for sprint
  jmp .print

.fail:
  mov si, .failmsg
  mov ah, 0x04 ; formatting for sprint

.print:
  xor bl, bl
  call sprint
  ret

.successmsg db 'success' ,0x0A, 0x00
.failmsg db 'fail' ,0x0A, 0x00


a20_bios:
  push eax								; Save registers
  mov ax, 0x2401							; Use BIOS to try to enable a20
  int 0x15
  call a20_check
  pop eax
  ret

a20_keyboard:


cli										; Disable interrupts

call .a20wait							; Use the keyboard controller to try and
mov al, 0xAD							; open the A20 gate
out 0x64, al

call .a20wait
mov al, 0xD0
out 0x64, al

call .a20wait2
in al, 0x60
push eax

call .a20wait
mov al, 0xD1
out 0x64, al

call .a20wait
pop eax
or al, 2
out 0x60, al

call .a20wait
mov al, 0xAE
out 0x64, al

call .a20wait
sti										; Enable interrupts back

jmp .keyboard_done

.a20wait:

in al, 0x64
test al, 2
jnz .a20wait
ret

.a20wait2:

in al, 0x64
test al, 1
jz .a20wait2
ret

.keyboard_done:

call a20_check							; Check for success

; Now just quit the routine, forwarding the carry flag to the caller

.done:
ret
