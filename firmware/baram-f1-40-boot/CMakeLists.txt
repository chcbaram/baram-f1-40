cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tools")
include(arm-none-eabi-gcc)


# 프로젝트 이름은 최종 출력 파일 이름으로 사용된다.
#
set(PRJ_NAME baram-f1-40-boot)


project(${PRJ_NAME}
  LANGUAGES ASM C CXX
)


set(EXECUTABLE ${PRJ_NAME}.elf)

include(src/hw/driver/usb/CMakeLists.txt)

# 지정한 폴더에 있는 파일만 포함한다.
#
file(GLOB SRC_FILES CONFIGURE_DEPENDS
  src/*.cpp
  src/*.c
  src/ap/*.cpp
  src/ap/*.c
  src/ap/modules/uf2/*.c
  src/ap/modules/boot/*.c
  src/bsp/*.c
  src/bsp/device/*.c

  src/hw/*.c
  src/hw/driver/*.c
  src/hw/driver/eeprom/*.c
  src/hw/driver/usb/*.c

  src/lib/ST/STM32_USB_Device_Library/Core/Src/*.c
)

# 하위폴더에 있는 파일까지 포함한다.
#
file(GLOB_RECURSE SRC_FILES_RECURSE CONFIGURE_DEPENDS
  src/bsp/*.s
  src/common/*.c

  src/lib/ST/STM32F4xx_HAL_Driver/Src/*.c
)

# 특정 폴더를 빌드에서 제외한다.
#
set(EXCLUDE_PATHS 
)

foreach(TMP_PATH ${SRC_FILES_RECURSE}) 
  foreach(EXCLUDE_PATH ${EXCLUDE_PATHS}) 
    string(FIND ${TMP_PATH} ${EXCLUDE_PATH} RESULT) 
    if(NOT ${RESULT} EQUAL -1) 
      list(REMOVE_ITEM SRC_FILES_RECURSE ${TMP_PATH}) 
    endif() 
  endforeach(EXCLUDE_PATH) 
endforeach(TMP_PATH)


add_executable(${EXECUTABLE} 
  ${SRC_FILES}
  ${SRC_FILES_RECURSE}
  ${QMK_SRC_FILES}
  ${USB_SRC_FILES}  
)



target_include_directories(${EXECUTABLE} PRIVATE 
  src  
  src/ap
  src/ap/modules
  src/bsp
  src/bsp/device
  src/common 
  src/common/core
  src/common/hw/include 
  src/hw
  src/hw/driver
  src/lib

  src/hw/driver/usb
  src/hw/driver/usb/usb_cdc
  src/hw/driver/usb/usb_hid
  src/hw/driver/usb/usb_cmp
  src/hw/driver/eeprom

  src/lib/ST/CMSIS/Include
  src/lib/ST/CMSIS/Device/ST/STM32F4xx/Include
  src/lib/ST/STM32F4xx_HAL_Driver/Inc

  ${USB_INC_DIR}  
)

target_compile_definitions(${EXECUTABLE} PRIVATE
  -DSTM32F411xE
  -DUSE_HAL_DRIVER  
  )

target_compile_options(${EXECUTABLE} PRIVATE
  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=hard

  -fdata-sections
  -ffunction-sections
  
  -fstack-usage 
  -Wmissing-field-initializers
  -Wuninitialized 
  # -Wenum-conversion

  -Wall
  -g3
  -Og
  )

target_link_options(${EXECUTABLE} PRIVATE
  -T../src/bsp/ldscript/STM32F411CEUx_BOOT.ld

  -mcpu=cortex-m4
  -mthumb
  -mfpu=fpv4-sp-d16
  -mfloat-abi=hard
  -specs=nano.specs
  -lc
  -lm
  -lstdc++
  -lsupc++
  -fno-rtti
  # -lnosys
  -Wl,-Map=${PRJ_NAME}.map,--cref
  -Wl,--gc-sections
  -Xlinker -print-memory-usage -Xlinker
  )

set (CMAKE_C_FLAGS "-Wenum-conversion")
set (CMAKE_CXX_FLAGS "-fno-rtti")

message(${CMAKE_CXX_COMPILER_VERSION})
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 11)
  string(APPEND CMAKE_EXE_LINKER_FLAGS "-Wl,--no-warn-rwx-segments")
endif()

add_custom_command(TARGET ${EXECUTABLE} 
  POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} ARGS -O binary ${EXECUTABLE} ${PROJECT_NAME}.bin
  COMMENT "Invoking: Make Bin -> ${KEYBOARD_PATH}"
  )  
  

