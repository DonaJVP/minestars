# Install script for directory: /home/user/dev/minestars/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "0")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/home/user/dev/minestars/src/threading/cmake_install.cmake")
  include("/home/user/dev/minestars/src/content/cmake_install.cmake")
  include("/home/user/dev/minestars/src/database/cmake_install.cmake")
  include("/home/user/dev/minestars/src/gui/cmake_install.cmake")
  include("/home/user/dev/minestars/src/mapgen/cmake_install.cmake")
  include("/home/user/dev/minestars/src/network/cmake_install.cmake")
  include("/home/user/dev/minestars/src/unittest/cmake_install.cmake")
  include("/home/user/dev/minestars/src/core/cmake_install.cmake")
  include("/home/user/dev/minestars/src/addons/cmake_install.cmake")
  include("/home/user/dev/minestars/src/util/cmake_install.cmake")
  include("/home/user/dev/minestars/src/irrlicht_changes/cmake_install.cmake")
  include("/home/user/dev/minestars/src/server/cmake_install.cmake")
  include("/home/user/dev/minestars/src/slave_proxy_net/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "/home/user/dev/minestars/src/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
