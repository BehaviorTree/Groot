cmake_minimum_required(VERSION 3.2)

project(behavior_tree_editor)
set(CMAKE_CXX_STANDARD 11)
include_directories(QtNodeEditor/include/nodes/internal)

find_package(Qt5 COMPONENTS  Core Widgets Gui OpenGL Xml Svg)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")


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

add_definitions(${Qt5Widgets_DEFINITIONS} -DNODE_EDITOR_STATIC)
set(CMAKE_CXX_FLAGS "${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS} -Werror=return-type")

if (UNIX)
  option(do_enable_valgrind "Enable running of valgrind." OFF)
  if (do_enable_valgrind)
    set(valgrind_log_file ${CMAKE_CURRENT_BINARY_DIR}/valgrind.out)
    add_custom_target(run_valgrind
      COMMAND valgrind
        --leak-check=full
        -v
        --log-file=${valgrind_log_file}
        --track-origins=yes
        --show-leak-kinds=all
        $<TARGET_FILE:behavior_tree_editor>
      COMMENT "Valgrind log file: ${valgrind_log_file}"
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      VERBATIM)
  else()
    # Enable GCC address memory sanitizer. This setting seems to be incompatible
    # with Valgrind.
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
  endif()
endif()


INCLUDE_DIRECTORIES( ${catkin_INCLUDE_DIRS} ./QtNodeEditor/include ./3rdparty )

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
    ./bt_editor/bt_editor_base.cpp
    ./bt_editor/graphic_container.cpp
    ./bt_editor/startup_dialog.cpp

    ./bt_editor/sidepanel_editor.cpp
    ./bt_editor/sidepanel_replay.cpp

    ./3rdparty/tinyXML2/tinyxml2.cpp
    ./bt_editor/XML_utilities.cpp
    )

qt5_add_resources(RESOURCES
    ./bt_editor/resources/icons.qrc
    )

set(FORMS_UI
  ./bt_editor/mainwindow.ui
  ./bt_editor/action_form.ui
  ./bt_editor/sidepanel_editor.ui
  ./bt_editor/sidepanel_replay.ui
  ./bt_editor/startup_dialog.ui
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

add_executable(behavior_tree_editor
    ${APP_CPPS}
    ${FORMS_HEADERS}
    ${RESOURCES})

if( ZMQ_FOUND )
    target_link_libraries(behavior_tree_editor QtNodeEditor zmq )
else()
    target_link_libraries(behavior_tree_editor QtNodeEditor )
endif()


