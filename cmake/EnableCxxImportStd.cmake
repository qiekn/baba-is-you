# https://github.com/BAN-43-32532/cmake-import-std/
# Usage: include(path/to/GetCxxImportStdGUID.cmake)
# Must be included BEFORE `project()`.

# CMake 4.0.2 + MSVC may fail from my testing. Consider using the latest version of CMake (>= 4.1)

# You can add this line in your CMakeLists.txt to suppress warning from CMake:
# set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "")

message(STATUS "Detected CMake version: ${CMAKE_VERSION}")

if (CMAKE_VERSION VERSION_LESS "3.30")
  message(FATAL_ERROR
    "CMAKE_EXPERIMENTAL_CXX_IMPORT_STD requires CMake version >= 3.30. "
    "The current version is ${CMAKE_VERSION}."
  )
endif ()

set(_url "https://raw.githubusercontent.com/Kitware/CMake/v${CMAKE_VERSION}/Source/cmExperimental.cxx")
set(_tmp "${CMAKE_BINARY_DIR}/cmExperimental.cxx")

if (EXISTS "${_tmp}")
  message(STATUS "Using cached cmExperimental.cxx: ${_tmp}. "
    "If you wish to refresh it (e.g. you are using a different CMake version), "
    "please delete this file manually."
  )
else ()
  message(STATUS "Downloading cmExperimental.cxx from: ${_url}")
  file(DOWNLOAD "${_url}" "${_tmp}" STATUS _dl_status SHOW_PROGRESS TIMEOUT 15)

  list(GET _dl_status 0 _dl_exit)
  if (NOT _dl_exit EQUAL 0)
    message(FATAL_ERROR
      "Failed to download cmExperimental.cxx. "
      "Download status: ${_dl_status}"
    )
  endif ()
endif ()

file(READ "${_tmp}" _file_contents)
string(REGEX MATCH
  "\\{[ \t\n\r]*\"CxxImportStd\"[ \t\n\r]*,[ \t\n\r]*\"([0-9a-fA-F-]+)\""
  _match "${_file_contents}"
)

if (NOT _match)
  message(FATAL_ERROR "Unable to extract the CxxImportStd GUID from cmExperimental.cxx")
endif ()

set(CXX_IMPORT_STD_GUID "${CMAKE_MATCH_1}")

set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "${CXX_IMPORT_STD_GUID}" CACHE STRING "" FORCE)
message(STATUS "CxxImportStd GUID set to: ${CMAKE_EXPERIMENTAL_CXX_IMPORT_STD}")

set(CMAKE_CXX_MODULE_STD ON CACHE BOOL "" FORCE)
message(STATUS "CMAKE_CXX_MODULE_STD enabled")
