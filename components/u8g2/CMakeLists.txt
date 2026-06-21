set(U8G2_CLIB_SRCS
    "src/clib/u8g2_box.c"
    "src/clib/u8g2_buffer.c"
    "src/clib/u8g2_font.c"
    "src/clib/u8g2_fonts.c"
    "src/clib/u8g2_fonts_extra.c"
    "src/clib/u8g2_terminal_fonts.c"
    "src/clib/u8g2_hvline.c"
    "src/clib/u8g2_intersection.c"
    "src/clib/u8g2_kerning.c"
    "src/clib/u8g2_ll_hvline.c"
    "src/clib/u8g2_message.c"
    "src/clib/u8g2_setup.c"
    "src/clib/u8x8_display.c"
    "src/clib/u8x8_8x8.c"
    "src/clib/u8x8_message.c"
    "src/clib/u8x8_setup.c"
    "src/clib/u8x8_string.c"
)

idf_component_register(
    SRCS ${U8G2_CLIB_SRCS}
    INCLUDE_DIRS "src/clib"
)
