cmake_minimum_required(VERSION 3.10)
project(BankSystem)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows 平台需要链接 gdi32
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS -DUNICODE -D_UNICODE)
endif()

set(SOURCES
    main.cpp
    ui.cpp
    fileio.cpp
    employee.cpp
    customer.cpp
    bankcard.cpp
    transaction.cpp
    bankqueue.cpp
    branchgraph.cpp
    smartmgr.cpp
)

add_executable(BankSystem ${SOURCES})

if(WIN32)
    target_link_libraries(BankSystem)
endif()
