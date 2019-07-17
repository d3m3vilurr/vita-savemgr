cmake_minimum_required(VERSION 2.8)

set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_C_COMPILER "arm-vita-eabi-gcc")

project(savemgr)
set(TITLE_ID "SAVEMGR00")
set(TITLE "Save Manager")
set(VERSION "2.0.0")

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

add_custom_target(vitashell_kernel_module
    COMMAND ${CMAKE_COMMAND} .
    COMMAND make
    COMMAND cp kernel.skprx ${CMAKE_SOURCE_DIR}/sce_sys/kernel.skprx
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/extern/vitashell/modules/kernel)

add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/extern/vitashell/modules/user/vitashell_user_stubs/libVitaShellUser_stub_weak.a
    DEPENDS vitashell_kernel_module
    COMMAND ${CMAKE_COMMAND} .
    COMMAND make
    COMMAND cp user.suprx ${CMAKE_SOURCE_DIR}/sce_sys/user.suprx
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/extern/vitashell/modules/user)

add_custom_target(vitashell_user_lib ALL DEPENDS ${CMAKE_SOURCE_DIR}/extern/vitashell/modules/user/vitashell_user_stubs/libVitaShellUser_stub_weak.a)
add_custom_target(vitashell_user_module ALL DEPENDS vitashell_user_lib)

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
    ${CMAKE_SOURCE_DIR}/extern/vitashell/modules/user/vitashell_user_stubs/libVitaShellUser_stub_weak.a

    -ldebugnet
    -lSceNetCtl_stub
    -lSceNet_stub
)

add_dependencies(${PROJECT_NAME}.elf vitashell_kernel_module vitashell_user_module)

add_custom_target(${PROJECT_NAME}.vpk ALL
    COMMAND vita-elf-create ${PROJECT_NAME}.elf ${PROJECT_NAME}.velf
    COMMAND vita-make-fself -c ${PROJECT_NAME}.velf eboot.bin
    COMMAND vita-mksfoex -s TITLE_ID=${TITLE_ID} -s APP_VER=${APP_VER} "${TITLE}" param.sfo
    COMMAND vita-pack-vpk -s param.sfo -b eboot.bin
            --add ../sce_sys/icon0.png=sce_sys/icon0.png
            --add ../sce_sys/kernel.skprx=sce_sys/kernel.skprx
            --add ../sce_sys/user.suprx=sce_sys/user.suprx
            --add ../sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png
            --add ../sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png
            --add ../sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml
            ${PROJECT_NAME}.vpk
)

add_dependencies(${PROJECT_NAME}.vpk ${PROJECT_NAME}.elf)
