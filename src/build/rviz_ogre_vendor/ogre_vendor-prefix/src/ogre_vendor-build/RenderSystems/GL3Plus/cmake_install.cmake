# Install script for directory: /home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus

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
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL3Plus.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL3Plus.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL3Plus.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL3Plus.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL3Plus.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL3Plus.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL3Plus.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL3Plus.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL3Plus.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OGRE/RenderSystems/GL3Plus" TYPE FILE FILES
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusDepthBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusFBOMultiRenderTarget.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusFBORenderTexture.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusFrameBufferObject.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusHardwareBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusHardwareBufferManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusHardwareIndexBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusHardwareOcclusionQuery.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusHardwarePixelBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusHardwareUniformBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusHardwareVertexBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusPixelFormat.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusPlugin.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusPrerequisites.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusRenderSystem.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusRenderToVertexBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusStateCacheManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusTexture.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusTextureBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreGL3PlusTextureManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/OgreSPIRVShaderFactory.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLContext.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLDepthBufferCommon.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLHardwarePixelBufferCommon.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLNativeSupport.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLPBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLRenderSystemCommon.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLRenderTarget.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLRenderTexture.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLStateCacheManagerCommon.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLTextureCommon.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLUniformCache.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLUtil.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLVertexArrayObject.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GLSupport/include/OgreGLWindow.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/include/OgreGLSupportPrerequisites.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OGRE/RenderSystems/GL3Plus" TYPE DIRECTORY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL3Plus/include/GL")
endif()

