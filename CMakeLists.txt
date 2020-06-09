cmake_minimum_required(VERSION 3.2)

project(groot)

find_package(Qt5 COMPONENTS  Core Widgets Gui OpenGL Xml Svg)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}  "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(NOT CMAKE_VERSION VERSION_LESS 3.1)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

#############################################################
# http://answers.ros.org/question/230877/optionally-build-a-package-with-catkin/
if( CATKIN_DEVEL_PREFIX OR CATKIN_BUILD_BINARY_PACKAGE)
   set(catkin_FOUND 1)
  # add_definitions( -DUSING_ROS )
endif()

if(catkin_FOUND)
  find_package(catkin REQUIRED COMPONENTS behaviortree_cpp_v3 rosbag rosbag_storage roscpp)

  message(STATUS "------------------------------------------------")
  message(STATUS "BehaviourTreeEditor is being built using CATKIN.")
  message(STATUS "------------------------------------------------")

  catkin_package(
    INCLUDE_DIRS 
    LIBRARIES qt_node_editor
    CATKIN_DEPENDS behaviortree_cpp_v3
    DEPENDS 
    )
endif(catkin_FOUND)
#############################################################

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_definitions(${Qt5Widgets_DEFINITIONS} -DNODE_EDITOR_STATIC )
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

#set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
#set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")

INCLUDE_DIRECTORIES( ${catkin_INCLUDE_DIRS} ./QtNodeEditor/include ./  )

add_subdirectory( QtNodeEditor )

##########################################
# FIX ME in the future....

set(BUILD_EXAMPLES   OFF CACHE BOOL "")
set(BUILD_UNIT_TESTS OFF CACHE BOOL "")
set(BUILD_TOOLS OFF CACHE BOOL "")
set(BUILD_TESTING OFF CACHE BOOL "")
set(CATKIN_ENABLE_TESTING OFF CACHE BOOL "")

if(NOT catkin_FOUND)
    # look for BehaviorTree.CPP on the system
    find_package(BehaviorTree)
    if (NOT BehaviorTree_FOUND)
        # use git submodule only if you are not compiling with catkin
        add_subdirectory( depend/BehaviorTree.CPP )
        include_directories( depend/BehaviorTree.CPP/include )
    endif()
endif()

##########################################

set(APP_CPPS
    ./bt_editor/models/BehaviorTreeNodeModel.cpp
    ./bt_editor/models/SubtreeNodeModel.cpp

    ./bt_editor/mainwindow.cpp
    ./bt_editor/editor_flowscene.cpp
    ./bt_editor/utils.cpp
    ./bt_editor/bt_editor_base.cpp
    ./bt_editor/graphic_container.cpp
    ./bt_editor/startup_dialog.cpp

    ./bt_editor/sidepanel_editor.cpp
    ./bt_editor/sidepanel_replay.cpp
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
  ./bt_editor/sidepanel_replay.ui
  ./bt_editor/startup_dialog.ui
  ./bt_editor/custom_node_dialog.ui
  )

find_package(ZMQ)

if( ZMQ_FOUND )
    message(STATUS "ZeroMQ found.")
    add_definitions( -DZMQ_FOUND )

    set(APP_CPPS ${APP_CPPS} ./bt_editor/sidepanel_monitor.cpp )
    set(FORMS_UI ${FORMS_UI} ./bt_editor/sidepanel_monitor.ui )

else()
    message(WARNING "ZeroMQ NOT found. Real-time monitoring disabled.")
endif()

QT5_WRAP_UI(FORMS_HEADERS ${FORMS_UI})

add_library(behavior_tree_editor
    ${APP_CPPS}
    ${FORMS_HEADERS}
)

SET(GROOT_DEPENDENCIES QtNodeEditor )

if( catkin_FOUND )
    SET(GROOT_DEPENDENCIES ${GROOT_DEPENDENCIES} ${catkin_LIBRARIES} )
else()
    SET(GROOT_DEPENDENCIES ${GROOT_DEPENDENCIES} behaviortree_cpp_v3 )
endif()

if( ZMQ_FOUND )
    SET(GROOT_DEPENDENCIES ${GROOT_DEPENDENCIES} zmq)
endif()

target_link_libraries(behavior_tree_editor ${GROOT_DEPENDENCIES} )


add_executable(Groot ./bt_editor/main.cpp  ${RESOURCE_FILES})
target_link_libraries(Groot behavior_tree_editor )

add_subdirectory(test)

######################################################
# INSTALL
if(catkin_FOUND)
    set( GROOT_LIB_DESTINATION   ${CATKIN_PACKAGE_LIB_DESTINATION} )
    set( GROOT_INC_DESTINATION   ${CATKIN_PACKAGE_INCLUDE_DESTINATION} )
    set( GROOT_BIN_DESTINATION   ${CATKIN_PACKAGE_BIN_DESTINATION} )
else()
    set( GROOT_LIB_DESTINATION   lib )
    set( GROOT_INC_DESTINATION   include )
    set( GROOT_BIN_DESTINATION   bin )
    
    INSTALL( FILES ${CMAKE_CURRENT_SOURCE_DIR}/groot_icon.png
			 DESTINATION share/icons/hicolor/256x256)
			 
	INSTALL( FILES ${CMAKE_CURRENT_SOURCE_DIR}/Groot.desktop
			 DESTINATION share/applications)
    
endif()

INSTALL(TARGETS behavior_tree_editor LIBRARY DESTINATION ${GROOT_LIB_DESTINATION} )
INSTALL(TARGETS Groot RUNTIME DESTINATION ${GROOT_BIN_DESTINATION} )




