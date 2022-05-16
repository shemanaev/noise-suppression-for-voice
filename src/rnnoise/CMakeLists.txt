cmake_minimum_required(VERSION 3.6)
project(RnNoise LANGUAGES C)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(RN_NOISE_SRC
        include/_kiss_fft_guts.h
        include/arch.h
        include/celt_lpc.h
        include/common.h
        include/kiss_fft.h
        include/opus_types.h
        include/pitch.h
        include/rnn.h
        include/rnn_data.h
        include/rnnoise.h
        include/tansig_table.h
        src/celt_lpc.c
        src/denoise.c
        src/kiss_fft.c
        src/pitch.c
        src/rnn.c
        src/rnn_data.c
        src/rnn_reader.c)

add_library(RnNoise STATIC ${RN_NOISE_SRC})

# Disable all warnings, since it's an external library.
target_compile_options(RnNoise PRIVATE
        $<$<OR:$<C_COMPILER_ID:Clang>,$<C_COMPILER_ID:AppleClang>,$<C_COMPILER_ID:GNU>>:
        -w>
        $<$<CXX_COMPILER_ID:MSVC>:
        /w>)

target_include_directories(RnNoise PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE src)

if(MSVC)
	target_compile_definitions(RnNoise PRIVATE "USE_MALLOC" "HAS_CPUID")
    target_compile_options(RnNoise PRIVATE /arch:AVX2)
endif()

