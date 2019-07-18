[![Build Status](https://travis-ci.org/BehaviorTree/Groot.svg?branch=master)](https://travis-ci.org/BehaviorTree/Groot)

# Groot 

**Groot** is a Graphical Editor, written in C++ and Qt, to create [BehaviorTrees](https://en.wikipedia.org/wiki/Behavior_tree).

It is compliant with the the library [BehaviorTree.CPP](https://github.com/BehaviorTree/BehaviorTree.CPP).

[![Groot Editor](groot-screenshot.png)](https://vimeo.com/275066768)


In the following video you can see how the C++ library and
the graphic user interface are used to design and monitor a Behavior Tree.

[![MOOD2Be](video_MOOD2Be.png)](https://vimeo.com/304651183)


# Dependencies, Installation, and Usage

To compile the project you need:

- [CMake 3.2](https://cmake.org/download)
- Qt5 (tested with version 5.5.1), including the SVG module.

On Ubuntu Xenial or later, you can install the dependencies with:

       sudo apt-get install qtbase5-dev libqt5svg5-dev
       sudo apt install libdw-dev
      
Some functionalities of the code related to ROS will work __only__ if the
project is compiled with with _catkin_.

# Compilation instructions (Linux)

       git clone https://github.com/BehaviorTree/Groot.git
       cd Groot
       git submodule update --init --recursive
       mkdir build; cd build
       cmake ..
       make
       
 Note compiling "in-source" is not allowed.      

# Licence       

Copyright (c) 2018-2019 FUNDACIO EURECAT 

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

  
# Acknowledgment

This project is one of the main components of [MOOD2Be](https://eurecat.org/es/portfolio-items/mood2be/),
and it is developed at [Eurecat](https://eurecat.org) by Davide Faconti.

MOOD2Be is one of the six **Integrated Technical Projects (ITPs)** selected from the [RobMoSys first open call](https://robmosys.eu/itp/). 

It received funding from the European Union’s Horizon 2020 Research and Innovation Programme
under the RobMoSys project.

