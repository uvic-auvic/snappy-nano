# Install script for directory: /home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "RelWithDebInfo")
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
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreOverlay.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreOverlay.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreOverlay.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreOverlay.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreOverlay.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreOverlay.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreOverlay.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreOverlay.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreOverlay.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OGRE/Overlay" TYPE FILE FILES
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreBorderPanelOverlayElement.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreFont.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreFontManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreImGuiOverlay.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreOverlay.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreOverlayContainer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreOverlayElement.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreOverlayElementFactory.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreOverlayManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreOverlayProfileSessionListener.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreOverlaySystem.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgrePanelOverlayElement.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreTextAreaOverlayElement.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreUTFString.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/include/OgreOverlayPrerequisites.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/Overlay/include/OgreOverlay.i"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/imgui-1.79/imgui.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/imgui-1.79/imconfig.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/OGRE/Media/packs" TYPE FILE FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Media/packs/profiler.zip")
endif()

