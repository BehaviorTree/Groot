cmake_minimum_required(VERSION 3.2)

project(groot)

find_package(Qt5 COMPONENTS  Core Widgets Gui OpenGL Xml Svg)

if(NOT CMAKE_VERSION VERSION_LESS 3.1)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

if (WIN32)
    add_definitions("/EHsc")
endif()

#############################################################

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_definitions(${Qt5Widgets_DEFINITIONS} -DNODE_EDITOR_STATIC )
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

include_directories( ./QtNodeEditor/include ./  )

add_subdirectory( QtNodeEditor )

##########################################
# FIX ME in the future....

set(BUILD_EXAMPLES   OFF CACHE BOOL "")
set(BUILD_UNIT_TESTS OFF CACHE BOOL "")
set(BUILD_TOOLS OFF CACHE BOOL "")
set(BUILD_TESTING OFF CACHE BOOL "")


if(NOT EXISTS depend/BehaviorTree.CPP)
    message("Can't find the submodule BehaviorTree.CPP")
    message("Run [git submodule update --init --recursive]")
endif()

set(BUILD_EXAMPLES OFF CACHE BOOL "Build tutorials and examples" FORCE)
set(BUILD_UNIT_TESTS OFF CACHE BOOL "Build the unit tests" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

# look for BehaviorTree.CPP as submodule
# use git submodule only if you are not compiling with catkin
add_subdirectory( depend/BehaviorTree.CPP )
include_directories( depend/BehaviorTree.CPP/include )

message(STATUS "----------------------------------")
message(STATUS "BehaviourTree.CPP submodule found.")
message(STATUS "----------------------------------")


##########################################

set(APP_CPPS
    ./bt_editor/models/BehaviorTreeNodeModel.cpp
    ./bt_editor/models/SubtreeNodeModel.cpp

    ./bt_editor/mainwindow.cpp
    ./bt_editor/editor_flowscene.cpp
    ./bt_editor/utils.cpp
    ./bt_editor/bt_editor_base.cpp
    ./bt_editor/graphic_container.cpp

    ./bt_editor/sidepanel_editor.cpp
    ./bt_editor/custom_node_dialog.cpp

    ./bt_editor/XML_utilities.cpp
    )

set(RESOURCE_FILES
    ./bt_editor/resources/icons.qrc
    ./bt_editor/resources/style.qrc
    ./test_data/test_files.qrc
    )

set(FORMS_UI
  ./bt_editor/about_dialog.ui
  ./bt_editor/mainwindow.ui
  ./bt_editor/action_form.ui
  ./bt_editor/sidepanel_editor.ui
  ./bt_editor/custom_node_dialog.ui
  )

QT5_WRAP_UI(FORMS_HEADERS ${FORMS_UI})

add_library(behavior_tree_editor SHARED
    ${APP_CPPS}
    ${FORMS_HEADERS}
)

set_target_properties(behavior_tree_editor PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
include(GenerateExportHeader)
generate_export_header(behavior_tree_editor)
target_include_directories(behavior_tree_editor PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

set(GROOT_DEPENDENCIES QtNodeEditor behaviortree_cpp_v3)

target_link_libraries(behavior_tree_editor ${GROOT_DEPENDENCIES} )

if (WIN32)
    add_executable(Groot WIN32 ./bt_editor/main.cpp  ${RESOURCE_FILES})
else()
    add_executable(Groot ./bt_editor/main.cpp  ${RESOURCE_FILES})
endif()

target_link_libraries(Groot behavior_tree_editor )

add_subdirectory(test)

######################################################
# INSTALL

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/groot_install)

set(GROOT_LIB_DESTINATION   lib)
set(GROOT_INC_DESTINATION   include)
set(GROOT_BIN_DESTINATION   bin)

install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/groot_icon.png
		 DESTINATION share/icons/hicolor/256x256)
		 
install( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Groot.desktop
		 DESTINATION share/applications)
    

install(TARGETS behavior_tree_editor CONFIGURATIONS Release LIBRARY DESTINATION ${GROOT_LIB_DESTINATION} )
install(TARGETS Groot CONFIGURATIONS Release RUNTIME DESTINATION ${GROOT_BIN_DESTINATION} )
