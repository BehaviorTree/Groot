cmake_minimum_required(VERSION 3.2)

project(behavior_tree_editor)

find_package(Qt5 COMPONENTS  Core Widgets Gui OpenGL Xml)


#############################################################
# http://answers.ros.org/question/230877/optionally-build-a-package-with-catkin/
if( CATKIN_DEVEL_PREFIX OR CATKIN_BUILD_BINARY_PACKAGE)
   set(catkin_FOUND 1)
  # add_definitions( -DUSING_ROS )
endif()

if(catkin_FOUND)
  find_package(catkin REQUIRED COMPONENTS behavior_tree_cpp rosbag rosbag_storage roscpp)

  message(STATUS "------------------------------------------------")
  message(STATUS "BehaviourTreeEditor is being built using CATKIN.")
  message(STATUS "------------------------------------------------")

  catkin_package(
    INCLUDE_DIRS 
    LIBRARIES qt_node_editor
    CATKIN_DEPENDS behavior_tree_cpp
    DEPENDS 
    )
endif(catkin_FOUND)
#############################################################

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

add_definitions(${Qt5Widgets_DEFINITIONS} -DNODE_EDITOR_STATIC -std=c++11)
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")

INCLUDE_DIRECTORIES( ${catkin_INCLUDE_DIRS} ./QtNodeEditor/include )

add_subdirectory( QtNodeEditor )

set(APP_CPPS

    ./bt_editor/models/BehaviorTreeNodeModel.cpp
    ./bt_editor/models/ActionNodeModel.cpp
    ./bt_editor/models/ControlNodeModel.cpp
    ./bt_editor/models/DecoratorNodeModel.cpp
    ./bt_editor/models/RootNodeModel.cpp
    ./bt_editor/models/SubtreeNodeModel.cpp

    ./bt_editor/main.cpp
    ./bt_editor/mainwindow.cpp
    ./bt_editor/editor_flowscene.cpp
    ./bt_editor/utils.cpp

 ##   ./bt_editor/NodeFactory.cpp
 #   ./bt_editor/XmlParsers.cpp
    )


qt5_add_resources(RESOURCES
    ./bt_editor/resources/icons.qrc
    )

QT5_WRAP_UI(FORMS_HEADERS
  ./bt_editor/mainwindow.ui
  ./bt_editor/action_form.ui )

add_executable(behavior_tree_editor
    ${APP_CPPS}
    ${FORMS_HEADERS}
    ${RESOURCES})

target_link_libraries(behavior_tree_editor QtNodeEditor)

