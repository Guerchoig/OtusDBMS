cmake_minimum_required(VERSION 3.10)
# project(DBServer)

add_library(db_server STATIC src/db_server.cpp src/db_command.cpp src/sqlite3.c)

set_target_properties(db_server PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)


target_include_directories(db_server PRIVATE  
                            "${PROJECT_SOURCE_DIR}/include"
                            "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if (MSVC)
        target_compile_options(db_server 
    /W4 )
else ()
       target_compile_options(db_server PRIVATE
        -Wall -Wextra -pedantic -Werror #-fsanitize=address
    )
    
endif()







