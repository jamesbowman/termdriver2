cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(td2)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

include_directories(../common .)

# rest of your project
add_executable(td2
    usb_descriptors.c
    main.c
    panel.c
    ../common/drawscreen.c
    ../common/render_font_4x6.c
    ../common/render_font_6x9_sub.c
    ../common/text.c
)
pico_set_linker_script(td2 ${CMAKE_CURRENT_LIST_DIR}/memmap_td2.ld)

set(ENC_KEYHEX "d55f8f")

set_source_files_properties(${LOGO_OBJ}
  PROPERTIES GENERATED TRUE EXTERNAL_OBJECT TRUE)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(
  td2
  pico_unique_id
  pico_stdlib
  pico_multicore
  hardware_spi
  hardware_dma
  tinyusb_device)

pico_add_extra_outputs(td2)

pico_enable_stdio_usb(td2 0)
pico_enable_stdio_uart(td2 1)
