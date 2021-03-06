# Copyright (c) 2015-2016, Intel Corporation
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors
# may be used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# This is the main cmake file of the DebugAgent project


# cmake configuration file of the "cAVS" component

# option to use Alsa-lib based Control device
SET_PROPERTY(CACHE MIXER_CONTROL_LIB_TO_USE PROPERTY STRINGS Tinyalsa Alsa-lib STRING
    "library selected to set/get control mixer on soundcard")

# Common source files
set(LIB_SRCS
    src/ModuleHandler.cpp
    src/LogStreamer.cpp
    src/System.cpp
    src/Topology.cpp
    src/PerfService.cpp
    src/ProbeService.cpp)

# Common include files
set(LIB_INCS
    include/cAVS/LogBlock.hpp
    include/cAVS/Logger.hpp
    include/cAVS/LogStreamer.hpp
    include/cAVS/Driver.hpp
    include/cAVS/DriverFactory.hpp
    include/cAVS/SystemDriverFactory.hpp
    include/cAVS/System.hpp
    include/cAVS/ModuleHandler.hpp
    include/cAVS/ModuleHandlerImpl.hpp
    include/cAVS/Topology.hpp
    include/cAVS/ProbeService.hpp
    include/cAVS/Prober.hpp
    include/cAVS/Perf.hpp
    include/cAVS/ProbeExtractor.hpp
    include/cAVS/ProbeInjector.hpp)

# firwmare include files
set(FW_INCS
    include/cAVS/DspFw/AudioFormat.hpp
    include/cAVS/DspFw/ConfigTypes.hpp
    include/cAVS/DspFw/FwConfig.hpp
    include/cAVS/DspFw/HwConfig.hpp
    include/cAVS/DspFw/ModuleInstance.hpp
    include/cAVS/DspFw/Pipeline.hpp
    include/cAVS/DspFw/Common.hpp
    include/cAVS/DspFw/Gateway.hpp
    include/cAVS/DspFw/Infrastructure.hpp
    include/cAVS/DspFw/ModuleType.hpp
    include/cAVS/DspFw/Scheduler.hpp
    include/cAVS/DspFw/ExternalFirmwareHeaders.hpp
    include/cAVS/DspFw/Probe.hpp
    include/cAVS/DspFw/GlobalPerfData.hpp
    include/cAVS/DspFw/GlobalMemoryState.hpp)

source_group("Header Files\\DspFw" FILES ${FW_INCS})
set(LIB_INCS ${LIB_INCS} ${FW_INCS})

set(LIBRAIRIES)

# Target specific source and header files
if (WIN32)
    # Windows IOCTL SST Driver
    include("CMakeListsWindows.txt")

    # including also Linux sources, but excluding them from build
    include("CMakeListsLinux.txt")
    set_source_files_properties(${LINUX_LIB_SRCS} PROPERTIES HEADER_FILE_ONLY true)

    # adding also os-specific cmake files to the visual studio project
    set(LIB_SRCS ${LIB_SRCS} CMakeListsWindows.txt CMakeListsLinux.txt)
elseif (UNIX)
    # Linux Debug FS SST Driver
    include("CMakeListsLinux.txt")
else ()
    error("No cAVS implementation for this platform")
endif (WIN32)

add_library(cAVS STATIC ${LIB_SRCS} ${LIB_INCS})
set_common_settings(cAVS)

target_link_libraries(cAVS ${LIBRAIRIES})

# exporting "include" directory
target_include_directories(cAVS PUBLIC "include")

find_path(CAVS_FW_HEADERS_INCLUDE_DIRECTORIES FW/src/common/include/stream_type.h)

# including and exporting cAVS fw headers
target_include_directories(cAVS SYSTEM PUBLIC
    "${CAVS_FW_HEADERS_INCLUDE_DIRECTORIES}/FW/src/common/include"
    "${CAVS_FW_HEADERS_INCLUDE_DIRECTORIES}/FW/src/intel_adsp/include"
)

if (WIN32)
    # including and exporting OED driver headers
    target_include_directories(cAVS SYSTEM PUBLIC
        "${PROJECT_SOURCE_DIR}/external/cAVS/cavs_sw/SW/OEPrivIface")
endif()

link_poco(cAVS)

#Adding test framework library, currently windows only
add_subdirectory("TestFramework")

# Adding test
add_subdirectory("test")

# Needed components
target_link_libraries(cAVS System Util Tlv)

if (CMAKE_SYSTEM_NAME MATCHES Linux)
    find_path(TINYCOMPRESS_INCLUDE_DIR NAMES tinycompress/tinycompress.h REQUIRED)
    target_include_directories(cAVS SYSTEM PUBLIC "${TINYCOMPRESS_INCLUDE_DIR}")

    find_library(TINYCOMPRESS_LIBRARY NAMES libtinycompress.so REQUIRED)
    target_link_libraries(cAVS ${TINYCOMPRESS_LIBRARY})

    if (MIXER_CONTROL_LIB_TO_USE MATCHES "Tinyalsa")
        find_path(TINYALSA_INCLUDE_DIR NAMES tinyalsa/asoundlib.h REQUIRED)
        # Why do I need to filter the tinyalsa include dir?
        # Tinyalsa is exposing 2 include directories:
        #   -"sound" directory with a asound.h file
        #   -"tinyalsa" directory with asoundlib.h file.
        # asound.h file is conflicting with libasound asound.h file.
        # As we do not need to include this file, we limit the inclusion to tinyalsa subfolder to
        # avoid conflict.
        target_include_directories(cAVS PUBLIC "${TINYALSA_INCLUDE_DIR}/tinyalsa")

        find_library(TINYALSA_LIBRARY NAMES tinyalsa REQUIRED)
        target_link_libraries(cAVS ${TINYALSA_LIBRARY})
    elseif (MIXER_CONTROL_LIB_TO_USE MATCHES "Alsa-lib")
        find_path(ALSA_LIB_INCLUDE_DIR NAMES alsa/asoundlib.h REQUIRED)
        target_include_directories(cAVS PUBLIC "${ALSA_LIB_INCLUDE_DIR}")

        find_library(LIBASOUND_LIBRARY NAMES asound REQUIRED)
        target_link_libraries(cAVS ${LIBASOUND_LIBRARY})
    endif ()
endif ()
