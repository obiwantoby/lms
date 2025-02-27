cmake_minimum_required(VERSION 3.12)

project(lms)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake/modules/)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
if (UNIX)
	set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
endif ()

include(CTest)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(Filesystem REQUIRED)
find_package(GTest REQUIRED)
find_package(Boost REQUIRED COMPONENTS system program_options)
find_package(Wt REQUIRED COMPONENTS Wt Dbo DboSqlite3 HTTP)
pkg_check_modules(Taglib REQUIRED IMPORTED_TARGET taglib)
pkg_check_modules(Config++ REQUIRED IMPORTED_TARGET libconfig++)
pkg_check_modules(GraphicsMagick++ IMPORTED_TARGET GraphicsMagick++)
pkg_check_modules(LIBAV IMPORTED_TARGET libavcodec libavutil libavformat)
find_package(PAM)
find_package(STB)

# WT
if (NOT Wt_FOUND)
	message(FATAL_ERROR "Wt package not found!")
endif ()
if (NOT TARGET Wt::Wt)
	message(FATAL_ERROR "Cannot find Wt::Wt!")
endif ()
if (NOT TARGET Wt::Dbo)
	message(FATAL_ERROR "Cannot find Wt::Dbo!")
endif ()
if (NOT TARGET Wt::DboSqlite3)
	message(FATAL_ERROR "Cannot find Wt::DboSqlite3!")
endif ()
if (NOT TARGET Wt::HTTP)
	message(FATAL_ERROR "Cannot find Wt::HTTP!")
endif ()

# PAM
option(USE_PAM "Use the PAM backend authentication API" ON)
if (USE_PAM AND NOT PAM_FOUND)
	message(WARNING "PAM library not found: disabling")
	set(USE_PAM OFF)
endif ()
if (USE_PAM)
	message(STATUS "Using PAM authentication backend")
else ()
	message(STATUS "NOT using PAM authentication backend")
endif ()

# IMAGE
if (STB_FOUND)
	set(IMAGE_LIBRARY STB CACHE STRING "STB library")
elseif (GraphicsMagick++_FOUND)
	set(IMAGE_LIBRARY GraphicsMagick++ CACHE STRING "GraphicsMagick++")
else ()
	message(FATAL_ERROR "Cannot find suitable image library")
endif()
set_property(CACHE IMAGE_LIBRARY PROPERTY STRINGS GraphicsMagick++ STB)
if (IMAGE_LIBRARY STREQUAL GraphicsMagick++ AND NOT GraphicsMagick++_FOUND)
	message(FATAL_ERROR "GraphicsMagick++ not found")
elseif (IMAGE_LIBRARY STREQUAL STB AND NOT STB_FOUND)
	message(FATAL_ERROR "STB not found")
endif ()
message(STATUS "IMAGE_LIBRARY set to ${IMAGE_LIBRARY}")

add_subdirectory(src)

install(DIRECTORY approot DESTINATION share/lms)
install(DIRECTORY docroot DESTINATION share/lms)
install(FILES conf/systemd/default.service DESTINATION share/lms)
install(FILES conf/pam/lms DESTINATION share/lms)
install(FILES conf/lms.conf DESTINATION share/lms)

