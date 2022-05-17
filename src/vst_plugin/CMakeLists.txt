cmake_minimum_required(VERSION 3.15)
project(rnnoise_vst2_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(VST2_INTERFACE_SRC
        pluginterfaces/vst2.x/aeffect.h
        pluginterfaces/vst2.x/aeffectx.h
        pluginterfaces/vst2.x/vstfxstore.h
        vst2.x/aeffeditor.h
        vst2.x/audioeffect.h
        vst2.x/audioeffectx.h
        vst2.x/vstplugmain.cpp
        vst2.x/audioeffect.cpp
        vst2.x/audioeffectx.cpp)

foreach (file ${VST2_INTERFACE_SRC})
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
        message(WARNING "${file} does not exist.\n Please see vst2.x/FILES.txt and pluginterfaces/vst2.x/FILES.txt")
        return()
    endif ()
endforeach ()

set(IMGUI_SRC
        imgui/imgui.h
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        imgui/misc/cpp/imgui_stdlib.cpp
        imgui/backends/imgui_impl_win32.h
        imgui/backends/imgui_impl_win32.cpp
        imgui/backends/imgui_impl_dx11.h
        imgui/backends/imgui_impl_dx11.cpp
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/imgui ${CMAKE_CURRENT_SOURCE_DIR}/imgui/backends)

set(IMGUI_LINK_LIBRARIES
        d3d11.lib
)

set(VST2_IMPL_SRC
        RnNoiseVstPlugin.h
        RnNoiseVstPlugin.cpp
        Editor.h
        Editor.cpp
)

set(VST2_PLUGIN_SRC
        ${VST2_INTERFACE_SRC}
        ${VST2_IMPL_SRC}
        ${IMGUI_SRC}
)

set(VST2_TARGET rnnoise_vst)

add_library(${VST2_TARGET} SHARED ${VST2_PLUGIN_SRC})

if (MINGW)
    target_link_libraries(${VST2_TARGET} ${MINGW_ADDITIONAL_LINKING_FLAGS})
    set(COMPILE_OPTIONS "$<$<CONFIG:RELEASE>:-O3;>")
endif()

target_link_libraries(${VST2_TARGET} RnNoisePluginCommon ${IMGUI_LINK_LIBRARIES})

target_compile_options(${VST2_TARGET} PRIVATE ${COMPILE_OPTIONS})

target_compile_definitions(${VST2_TARGET} PRIVATE
        "$<$<CXX_COMPILER_ID:GNU>:__cdecl=;>" # Workaround for vst headers
        )

set_target_properties(${VST2_TARGET} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vst"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/vst")
