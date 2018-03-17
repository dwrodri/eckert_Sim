cmake_minimum_required(VERSION 3.9)
project(eckert_Sim C)

set(CMAKE_C_STANDARD 11)
LINK_LIBRARIES(ncurses)

add_executable(eckert_Sim main.c)