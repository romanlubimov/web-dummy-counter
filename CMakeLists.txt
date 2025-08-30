cmake_minimum_required(VERSION 3.16)

project(web-counter-game LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(web-counter-game main.cpp
    crow_all.h)

include(GNUInstallDirs)
install(TARGETS web-counter-game
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
