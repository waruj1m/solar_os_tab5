<!-- cce-block-version: 3 -->
## Context Engine (CCE)

This project uses Code Context Engine for intelligent code retrieval and
cross-session memory.

### Searching the codebase

**You MUST use `context_search` instead of reading files directly** when
exploring the codebase, answering questions about code, or understanding how
things work. This is a hard requirement, not a suggestion. `context_search`
returns the most relevant code chunks with confidence scores instead of whole
files, and tracks token savings automatically.

When to use `context_search`:
- Answering questions about the codebase ("how does X work?", "where is Y?")
- Exploring structure or architecture
- Finding related code, functions, or patterns
- Any time you would otherwise read a file just to understand it

When to use `Read` instead:
- You need to edit a specific file (read before editing)
- You need the exact, complete content of a known file path

Other search tools:
- `expand_chunk` — get full source for a compressed result
- `related_context` — find what calls/imports a function

### Cross-session memory — use it actively

This project has persistent memory across Claude Code sessions. **You must
use it both ways: recall before answering, record after deciding.** Memory
that is not recorded is lost; memory that is not recalled does nothing.

**Before answering a non-trivial question, call `session_recall`.**
Especially when:
- The question touches architecture, design, or naming choices
- The user asks "what / why / how did we ..."
- You are about to recommend an approach the team may have already chosen
  or already rejected

Pass a topic phrase, not a single word — e.g. `session_recall("auth flow")`,
not `session_recall("auth")`. Recall is vector-similarity-based, so paraphrases
match. If recall returns relevant entries, lead with them ("Per a prior
decision: ...") instead of re-deriving the answer.

**After making a non-obvious decision, call `record_decision`.** Especially:
- Choosing one library / pattern / approach over another
- Resolving an ambiguity in the spec or requirements
- Establishing a convention the project should follow going forward
- Anything you would not want to re-litigate next session

Format: `record_decision(decision="...", reason="...")`. Keep both fields
short and specific — they are surfaced verbatim at the start of future
sessions.

**After meaningful work in a file, call `record_code_area`.** Especially when:
- You added or substantially modified a function/class
- You traced through a non-obvious flow and want future-you to find it fast

Format: `record_code_area(file_path="...", description="...")`.

Skip recording for trivial reads, formatting changes, or one-off lookups —
the goal is durable signal, not an event log.

### Drilling deeper from a recall hit

`session_recall` results are tagged with the source session id, e.g.
`[turn sid:abc123|n:5]`. To drill in:

- `session_timeline(session_id="abc123")` — walk the per-turn summaries of
  that session in order. Use this when the user asks "what was the
  reasoning?" or "how did we get there?".
- `session_event(event_id=N)` — fetch a specific tool event's raw input
  and output (capped at 4 KB at read time). Use this when a turn summary
  references a tool result you actually need to inspect.

Both are read-only and cheap. Prefer them over re-running tool calls or
asking the user to re-paste context.

## Output Style

Be concise. Lead with the answer or action, not reasoning. Skip filler words,
preamble, and phrases like "I'll help you with that" or "Certainly!". Prefer
fragments over full sentences in explanations. No trailing summaries of what
you just did. One sentence if it fits.

Code blocks, file paths, commands, and error messages are always written in full.
<!-- /cce-block -->

## M5Stack Tab5 Board Port

Branch `board/m5stack-tab5` (based on `board/m5stack-cardputer`, which is
itself unmerged to `main`). Full port plan and phase history in
`/Users/james/.claude/plans/i-want-you-to-nested-ritchie.md`.

Status as of the last hardware bring-up session (verified on real
hardware, not just build-tested):

**Working**: boot, PSRAM, I2C bus, both PI4IOE5V6408 IO expanders, SDMMC
SD card, USB HID keyboard host (compiles and initializes; not yet tested
against a physical USB keyboard), BMI270 IMU driver, RX8130 RTC, INA226
battery monitor, ES8388/ES7210 audio (build-verified only).

**Display**: multi-panel support in `src/drivers/lcd_ili9881c_dsi.c`.
Tab5 units in the wild ship one of three panel/touch pairings; detected
at runtime the same way M5Stack's own firmware does it (probe GT911 at
its I2C address; else probe touch address 0x55 and read its firmware
version register). The physical unit tested this session is **ST7123**
(fw_version 3), confirmed via touch chip response even though the panel's
own ID registers read back zero for all three variants (not a reliable
signal). Panel needs mirroring on both axes
(`esp_lcd_panel_mirror(panel, true, true)`) to correct a 180-degree
mounting offset -- applied unconditionally in `lcd_panel_setup()` for all
three panel types; re-check if a differently-mounted unit shows up.

**WiFi (broken, needs separate hardware to fix)**: the on-board ESP32-C6
ships M5Stack's own `ESP32C6-WiFi-SDIO-Interface` firmware
(`m5stack/M5Tab5-UserDemo/platforms/tab5/wifi_c6_fw/`), not the
`espressif/esp-hosted-mcu` slave firmware `solar_os_wifi.c` expects via
`esp_wifi_remote`/`esp_hosted`. Host-side SDIO init is confirmed correct
(right pins, right reset polarity, `esp_hosted_init()` called explicitly
-- see decisions below) but the `sdmmc_init_ocr` handshake with the C6
fails every time. Fixing this means reflashing the C6 with matching
slave firmware via its own separate physical download header (see
`docs.m5stack.com/en/guide/restore_factory/m5tab5_c6_wifi`) using
M5Stack's "ESP32 Downloader" accessory or a USB-TTL adapter -- not
reachable through the main USB-C port, and not something this session
could do without that accessory.

**Two toolchain bugs found and fixed, load-bearing for any board using
`esp_hosted` under pioarduino, not just this one:**
1. `CONFIG_ESP_HOSTED_SDIO_RESET_ACTIVE_LOW` is required; the Kconfig
   default (active-high) leaves the C6 permanently held in reset.
2. `esp_hosted_init()` must be called explicitly (now done at the top of
   `solar_os_wifi_init()` in `src/services/solar_os_wifi.c`, gated on
   `SOLAR_OS_BOARD_M5STACK_TAB5`). The managed component normally
   self-initializes via a GCC `__attribute__((constructor))` in
   `port_esp_hosted_host_init.c`, but pioarduino's SCons-based link does
   not honor that component's `WHOLE_ARCHIVE` CMake property, so the
   constructor-only translation unit is silently dropped (confirmed via
   the link map: every other file in `libespressif__esp_hosted.a` was
   pulled in by a real symbol reference; this one never was).

**Also found**: ESP32-P4 has no Bluetooth radio, so `service_ble` is
excluded via a flavor-level package override (`flavors/tab5.toml`) rather
than compiled in and left non-functional; `generate_flavor_config.py` now
lets explicit package overrides beat core-group membership to support
this, and `solar_os_ble_keyboard.h` has inline not-supported stubs so
every caller compiles unchanged.

**Not attempted**: camera capture (SC2356 via `esp_video`/`esp_cam_sensor`/
ISP) -- deliberately skipped. Unlike everything else in this port, that
stack can't be verified correct from source alone (ISP tuning, JPEG
hardware pipeline, MIPI-CSI bring-up are all version-sensitive and
opaque without the device in hand), and a wrong config risks a hang with
no way to debug it remotely.

**Hardware bring-up notes for next session**: this board's USB-Serial-JTAG
reset behaves oddly -- a manual RTS/DTR toggle (rather than `esptool`'s
own reset sequence) can leave it stuck in "waiting for download" mode
that persists across further software-only resets; the fix each time was
a full physical unplug/replug power cycle. Prefer `pio run -t upload`
(which drives `esptool`'s own correct reset sequence) over any manual
serial line manipulation.
