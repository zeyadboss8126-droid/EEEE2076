cmake_minimum_required(VERSION 3.19)
project(Worksheet6 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ────────────────────────────────────────────────────────────────
# Platform-specific paths and components
# ────────────────────────────────────────────────────────────────
if(APPLE)
  # macOS: Homebrew paths, no OpenVR
  set(Qt6_DIR  "/opt/homebrew/lib/cmake/Qt6")
  set(VTK_DIR  "/opt/homebrew/lib/cmake/vtk-9.5")
  set(VTK_VR_COMPONENT "")          # OpenVR not available on Mac
else()
  # Windows — all paths can be overridden with -D flags (e.g. from build_installer.bat)

  # VTK: default to standard binary distribution location
  if(NOT DEFINED VTK_DIR OR VTK_DIR STREQUAL "")
    set(VTK_DIR "C:/Program Files (x86)/VTK/lib/cmake/vtk-9.6" CACHE PATH "VTK cmake dir")
  endif()

  # OpenVR: accept a single OpenVR_ROOT, or individual paths.
  # Auto-detect C:/OpenVR then D:/OpenVR if nothing is provided.
  if(NOT DEFINED OpenVR_ROOT OR OpenVR_ROOT STREQUAL "")
    if(EXISTS "C:/OpenVR/headers/openvr.h")
      set(OpenVR_ROOT "C:/OpenVR" CACHE PATH "OpenVR SDK root")
    elseif(EXISTS "D:/OpenVR/headers/openvr.h")
      set(OpenVR_ROOT "D:/OpenVR" CACHE PATH "OpenVR SDK root")
    else()
      set(OpenVR_ROOT "C:/OpenVR" CACHE PATH "OpenVR SDK root")
    endif()
  endif()

  # Derive sub-paths from OpenVR_ROOT only when not already set
  if(NOT DEFINED OpenVR_INCLUDE_DIR OR OpenVR_INCLUDE_DIR STREQUAL "")
    set(OpenVR_INCLUDE_DIR "${OpenVR_ROOT}/headers" CACHE PATH "OpenVR include directory")
  endif()
  if(NOT DEFINED OpenVR_LIBRARY OR OpenVR_LIBRARY STREQUAL "")
    set(OpenVR_LIBRARY "${OpenVR_ROOT}/lib/win64/openvr_api.lib" CACHE FILEPATH "OpenVR library")
  endif()
  if(NOT DEFINED OpenVR_BIN_DIR OR OpenVR_BIN_DIR STREQUAL "")
    set(OpenVR_BIN_DIR "${OpenVR_ROOT}/bin/win64" CACHE PATH "OpenVR bin directory")
  endif()

  set(VTK_VR_COMPONENT "RenderingOpenVR")
endif()

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets OpenGLWidgets)
qt_standard_project_setup()

find_package(VTK REQUIRED COMPONENTS
  CommonCore
  CommonDataModel
  FiltersSources
  FiltersGeneral
  InteractionStyle
  RenderingCore
  RenderingOpenGL2
  RenderingQt
  GUISupportQt
  IOGeometry
  IOLegacy
  ${VTK_VR_COMPONENT}
)

qt_add_executable(Worksheet6
  WIN32
  main.cpp
  mainwindow.cpp
  mainwindow.h
  mainwindow.ui
  ModelPart.cpp
  ModelPart.h
  ModelPartList.cpp
  ModelPartList.h
  optiondialog.cpp
  optiondialog.h
  optiondialog.ui
  VRRenderThread.cpp
  VRRenderThread.h
  resources.qrc
)

target_link_libraries(Worksheet6 PRIVATE
  Qt6::Core
  Qt6::Widgets
  Qt6::OpenGLWidgets
  ${VTK_LIBRARIES}
)

# OpenVR linking only on Windows
if(NOT APPLE)
  target_include_directories(Worksheet6 PRIVATE ${OpenVR_INCLUDE_DIR})
  target_link_libraries(Worksheet6 PRIVATE ${OpenVR_LIBRARY})
endif()

vtk_module_autoinit(
  TARGETS Worksheet6
  MODULES ${VTK_LIBRARIES}
)

# VR bindings copy and DLL copy only on Windows
if(NOT APPLE)
  add_custom_target(VRBindings)
  add_custom_command(TARGET VRBindings PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E
    copy_directory ${CMAKE_SOURCE_DIR}/vrbindings ${CMAKE_BINARY_DIR}/
  )
  add_dependencies(Worksheet6 VRBindings)
  add_custom_command(TARGET Worksheet6 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${OpenVR_BIN_DIR}/openvr_api.dll"
    $<TARGET_FILE_DIR:Worksheet6>
  )
endif()