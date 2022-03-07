cmake_minimum_required (VERSION 3.18)
set(CMAKE_CXX_STANDARD 20)
set(PROJECT_NAME shooter)
project(${PROJECT_NAME})
find_package(OpenGL REQUIRED)
include_directories( 
          ${OPENGL_INCLUDE_DIRS} 
          src/game/
          src/platform/windows/
          src/platform/linux/
          src/shared/
          lib/
          )

file(GLOB SOURCE_FILES 
          "src/shared/*.cpp"
          "src/game/*.cpp"
          "src/platform/windows/*.cpp"
)
if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.12.25835)
    set(CMAKE_CXX20_STANDARD_COMPILE_OPTION "-std:c++latest")
    set(CMAKE_CXX20_EXTENSION_COMPILE_OPTION "-std:c++latest")
  endif()
add_executable(${PROJECT_NAME} WIN32 ${SOURCE_FILES})
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME}) 
set(CMAKE_GENERATOR_PLATFORM x64)
target_link_libraries(${PROJECT_NAME} kernel32 user32 gdi32 opengl32 xinput ole32 winmm)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_C_FLAGS "/D SOGL_MAJOR_VERSION=3 /D SOGL_MINOR_VERSION=3 /W4 /WX /wd5105 /wd4201 /wd4100")
  set(CMAKE_CXX_FLAGS "/D SOGL_MAJOR_VERSION=3 /D SOGL_MINOR_VERSION=3 /W4 /WX /wd5105 /wd4201 /wd4100")
else()
  set(CMAKE_C_FLAGS "-DSOGL_MAJOR_VERSION=3 -DSOGL_MINOR_VERSION=3 -Wno-address-of-temporary")
  set(CMAKE_CXX_FLAGS "-DSOGL_MAJOR_VERSION=3 -DSOGL_MINOR_VERSION=3 -Wno-address-of-temporary")
endif()
