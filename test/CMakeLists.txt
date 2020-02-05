CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

PROJECT(GrootTest LANGUAGES CXX)

find_package(Qt5Test REQUIRED)
find_package(Qt5Gui REQUIRED)

SET(CMAKE_AUTOMOC ON)
SET(CMAKE_AUTORCC ON)
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_CXX_STANDARD 11)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
ENABLE_TESTING()

function(CompileTest name)
    add_executable(${name} ${name}.cpp groot_test_base.cpp ${RESOURCE_FILES} )
    target_link_libraries(${name} PRIVATE Qt5::Gui Qt5::Test behavior_tree_editor)
    add_test(${name} COMMAND ${name})
endfunction()

set(RESOURCE_FILES
    ../QtNodeEditor/resources/resources.qrc
    ../bt_editor/resources/icons.qrc
    ../bt_editor/resources/style.qrc
    ../test_data/test_files.qrc
    )

CompileTest( editor_test )
CompileTest( replay_test )

