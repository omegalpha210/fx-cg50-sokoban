/* CG syscall dispatcher: r0=index, r4..r7=arguments. Tail calls preserve PR.
 * Original wrappers, numeric API references: docs/POWER.md. */
.text
.balign 4
.macro power_syscall name, index
.global \name
.type \name, @function
\name:
    mov.l 1f, r0
    mov.l 2f, r2
    jmp @r2
    nop
.balign 4
1:  .long \index
2:  .long 0x80020070
.size \name, .-\name
.endm
power_syscall _sok_os_auto_power_off, 0x1e91
power_syscall _sok_os_backlight_duration, 0x12d9
power_syscall _sok_os_light_level, 0x1e8f
power_syscall _sok_os_set_backlight, 0x0199
