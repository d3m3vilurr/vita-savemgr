cmake_minimum_required(VERSION 2.8)

# VitaSDK defines
if( NOT DEFINED CMAKE_TOOLCHAIN_FILE )
  if( DEFINED ENV{VITASDK} )
  set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
  message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

include(${CMAKE_TOOLCHAIN_FILE})
include("${VITASDK}/share/vita.cmake" REQUIRED)

project(savemgr)
set(TITLE_ID "SAVEMGR00")
set(TITLE "Save Manager")
set(VERSION "2.0.0")

add_subdirectory(extern/vitashell/modules/kernel)
add_subdirectory(extern/vitashell/modules/user)

execute_process(COMMAND
    python -c "print ('%02d.%02d' % tuple(map(lambda x: int(x), '${VERSION}'.split('.')[:2])))"
    OUTPUT_VARIABLE APP_VER
)
string(REPLACE "\n" "" APP_VER "${APP_VER}")

if (NOT EXISTS "$ENV{RELEASE}")
    execute_process(COMMAND
        git describe --abbrev=7 --dirty --always
        OUTPUT_VARIABLE GIT_HASH
    )
    string(REPLACE "\n" "" GIT_HASH "${GIT_HASH}")

    set(VERSION "${VERSION}-${GIT_HASH}")
endif(NOT EXISTS "$ENV{RELEASE}")

add_definitions(-DSQLITE_OS_OTHER=1)
add_definitions(-DSQLITE_TEMP_STORE=3)
add_definitions(-DSQLITE_THREADSAFE=0)
add_definitions(-DVERSION=\"${VERSION}\")
#add_definitions(-DUSE_DEBUG=1)
#add_definitions(-DDEBUG_IP=\"127.0.0.1\")

set(CMAKE_C_FLAGS "-Wl,-q -Wall -O3 -std=c99")

include_directories(
    extern/sqlite3
    extern/inih
    extern/vitashell/modules/kernel
    extern/vitashell/modules/user
)

add_executable(${PROJECT_NAME}.elf
    src/appdb.c
    src/file.c
    src/font.c
    src/display.c
    src/input.c
    src/config.c
    src/util.c
    src/main.c
    src/system.c
    src/vita_sqlite.c

    extern/sqlite3/sqlite3.c
    extern/inih/ini.c
)

target_link_libraries(${PROJECT_NAME}.elf
    -lvita2d
    #-lfreetype
    -lpng
    -lz
    -lm
    -lSceIofilemgr_stub
    -lSceKernelThreadMgr_stub
    -lSceCommonDialog_stub
    -lSceDisplay_stub
    -lSceGxm_stub
    -lSceAppMgr_stub
    -lSceAppUtil_stub
    -lSceSysmodule_stub
    -lSceCtrl_stub
    -lSceTouch_stub
    -lScePgf_stub
    -lSceShellSvc_stub
    -lSceRegistryMgr_stub
    -ltaihen_stub
    ${CMAKE_CURRENT_BINARY_DIR}/extern/vitashell/modules/user/libVitaShellUser_stub_weak.a

    -ldebugnet
    -lSceNetCtl_stub
    -lSceNet_stub
)

add_dependencies(${PROJECT_NAME}.elf libVitaShellUser_stub_weak.a)
add_dependencies(${PROJECT_NAME}.elf kernel.skprx)
add_dependencies(${PROJECT_NAME}.elf user.suprx)

vita_create_self(eboot.bin ${PROJECT_NAME}.elf UNSAFE)
vita_create_vpk(${PROJECT_NAME}.vpk ${TITLE_ID} eboot.bin
  VERSION ${VERSION}
  NAME ${TITLE}
  FILE ${CMAKE_SOURCE_DIR}/sce_sys/icon0.png sce_sys/icon0.png
       ${CMAKE_CURRENT_BINARY_DIR}/extern/vitashell/modules/kernel/kernel.skprx sce_sys/kernel.skprx
       ${CMAKE_CURRENT_BINARY_DIR}/extern/vitashell/modules/user/user.suprx sce_sys/user.suprx
       ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/bg.png sce_sys/livearea/contents/bg.png
       ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/startup.png sce_sys/livearea/contents/startup.png
       ${CMAKE_SOURCE_DIR}/sce_sys/livearea/contents/template.xml sce_sys/livearea/contents/template.xml
)
