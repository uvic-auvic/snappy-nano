# Install script for directory: /home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator

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
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreMeshLodGenerator.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreMeshLodGenerator.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreMeshLodGenerator.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreMeshLodGenerator.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreMeshLodGenerator.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreMeshLodGenerator.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreMeshLodGenerator.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/libOgreMeshLodGenerator.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libOgreMeshLodGenerator.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OGRE/MeshLodGenerator" TYPE FILE FILES
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLod0Stripifier.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodCollapseCost.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodCollapseCostCurvature.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodCollapseCostOutside.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodCollapseCostProfiler.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodCollapseCostQuadric.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodCollapser.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodConfig.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodConfigSerializer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodData.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodInputProvider.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodInputProviderBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodInputProviderMesh.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodOutputProvider.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodOutputProviderBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodOutputProviderCompressedBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodOutputProviderCompressedMesh.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodOutputProviderMesh.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodOutsideMarker.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodPrerequisites.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodWorkQueueInjector.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodWorkQueueInjectorListener.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodWorkQueueRequest.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreLodWorkQueueWorker.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreMeshLodGenerator.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreSmallVector.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreVectorSet.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/Components/MeshLodGenerator/include/OgreVectorSetImpl.h"
    )
endif()

