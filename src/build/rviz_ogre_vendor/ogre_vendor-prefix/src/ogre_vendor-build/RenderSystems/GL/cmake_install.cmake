# Install script for directory: /home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL

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
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee]|[Nn][Oo][Nn][Ee]|)$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL.so.1.12.10")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so.1.12.10")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  if("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      file(RPATH_CHECK
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so"
           RPATH "")
    endif()
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/OGRE" TYPE SHARED_LIBRARY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib/RenderSystem_GL.so")
    if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so" AND
       NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      file(RPATH_CHANGE
           FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so"
           OLD_RPATH "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor-build/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/OGRE/RenderSystem_GL.so")
      endif()
    endif()
  endif("${CMAKE_INSTALL_CONFIG_NAME}" MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OGRE/RenderSystems/GL" TYPE FILE FILES
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLCopyingRenderTexture.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLDepthBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLFBOMultiRenderTarget.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLFBORenderTexture.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLFrameBufferObject.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLGpuNvparseProgram.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLGpuProgram.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLGpuProgramManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLHardwareBufferManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLHardwareIndexBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLHardwareOcclusionQuery.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLHardwarePixelBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLHardwareVertexBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLPBRenderTexture.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLPixelFormat.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLPlugin.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLPrerequisites.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLRenderSystem.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLRenderToVertexBuffer.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLStateCacheManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLTexture.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/OgreGLTextureManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/GLSL/include/OgreGLSLExtSupport.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/GLSL/include/OgreGLSLLinkProgram.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/GLSL/include/OgreGLSLLinkProgramManager.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/GLSL/include/OgreGLSLProgram.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/GLSL/include/OgreGLSLProgramFactory.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/_ps1.0_parser.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/_rc1.0_parser.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/_ts1.0_parser.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/_vs1.0_parser.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/macro.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/nvparse.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/nvparse_errors.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/nvparse_externs.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/ps1.0_program.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/rc1.0_combiners.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/rc1.0_final.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/rc1.0_general.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/rc1.0_register.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/ts1.0_inst.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/ts1.0_inst_list.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/vs1.0_inst.h"
    "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/nvparse/vs1.0_inst_list.h"
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OGRE/RenderSystems/GL" TYPE DIRECTORY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/include/GL")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/OGRE/RenderSystems/GL" TYPE DIRECTORY FILES "/home/kraken/snappyNano/src/build/rviz_ogre_vendor/ogre_vendor-prefix/src/ogre_vendor/RenderSystems/GL/src/GLSL/include/")
endif()

