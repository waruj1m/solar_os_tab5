MicroPython Embed Component
===========================

This component vendors the MicroPython `ports/embed` package as a self-contained
ESP-IDF component for SolarOS.  It is generated from the upstream MicroPython
repository using `components/micropython_embed/mpconfigport.h` as the port
configuration.

The generated package lives in `micropython_embed/`.  If it is regenerated,
reapply the SolarOS shims in:

- `micropython_embed/port/mphalport.c`
  Routes MicroPython stdout through `solar_os_micropython_stdout()`.

- `micropython_embed/port/mphalport.h`
  Provides the no-op `mp_hal_set_interrupt_char()` hook used by
  `micropython.kbd_intr()`.

- `micropython_embed/port/embed_util.c`
  Marks `__assert_func()` weak so ESP-IDF newlib can provide the real symbol.
