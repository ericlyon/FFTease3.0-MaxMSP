include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)

set(LIBFFTEASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../libfftease")

include_directories(
	"${MAX_SDK_INCLUDES}"
	"${MAX_SDK_MSP_INCLUDES}"
	"${MAX_SDK_JIT_INCLUDES}"
    "${LIBFFTEASE_DIR}"
)

add_library(
	"${PROJECT_NAME}"
	MODULE
	multyq~.c
	"${LIBFFTEASE_DIR}/bloscbank.c"
    "${LIBFFTEASE_DIR}/convert.c"
    "${LIBFFTEASE_DIR}/fft.c"
    "${LIBFFTEASE_DIR}/fft4.c"
    "${LIBFFTEASE_DIR}/fftease_setup.c"
    "${LIBFFTEASE_DIR}/fftease_utilities.c"
    "${LIBFFTEASE_DIR}/fold.c"
    "${LIBFFTEASE_DIR}/leanconvert.c"
    "${LIBFFTEASE_DIR}/leanunconvert.c"
    "${LIBFFTEASE_DIR}/legacy.c"
    "${LIBFFTEASE_DIR}/limited_oscbank.c"
    "${LIBFFTEASE_DIR}/makewindows.c"
    "${LIBFFTEASE_DIR}/oscbank.c"
    "${LIBFFTEASE_DIR}/overlapadd.c"
    "${LIBFFTEASE_DIR}/PenroseOscil.c"
    "${LIBFFTEASE_DIR}/PenroseRand.c"
    "${LIBFFTEASE_DIR}/power_of_two.c"
    "${LIBFFTEASE_DIR}/qsortE.c"
    "${LIBFFTEASE_DIR}/unconvert.c"
)

string(PREPEND "${PROJECT_NAME}_EXTERN_OUTPUT_NAME" "fftz.")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)