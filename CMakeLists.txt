cmake_minimum_required(VERSION 3.10)

set(PATCH_VERSION "0" CACHE INTERNAL "Patch version")
set(PROJECT_VESRION 0.0.${PATCH_VERSION})

project(JoinServer VERSION ${PROJECT_VESRION})


# set(Boost_USE_STATIC_LIBS ON)

# FIND_PACKAGE( Boost COMPONENTS program_options filesystem regex REQUIRED )
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

include_directories(${CMAKE_BINARY_DIR})

# Local plant
link_directories(build)
include_directories(include)
add_executable(join_server src/join_server.cpp)
add_executable(client src/client.cpp) 

# a dir where sub'CmakeLists.txt resides
add_subdirectory(DBServer)

# where to look for lib binary
target_link_libraries(join_server PRIVATE  db_server)

# where to search library's header file
target_include_directories(join_server PRIVATE 
                            "${CMAKE_CURRENT_SOURCE_DIR}/include" 
                            "${CMAKE_CURRENT_SOURCE_DIR}/DBServer/include"
)

target_include_directories(client PRIVATE 
                            "${CMAKE_CURRENT_SOURCE_DIR}/include" 
                            "${CMAKE_CURRENT_SOURCE_DIR}/DBServer/include"
)


set_target_properties(join_server client PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
)



if (MSVC)
    target_compile_options(join_server 
        /W4
    )
    target_compile_options(client 
    /W4 )

else ()
    target_compile_options(join_server PRIVATE
        -Wall -Wextra -pedantic -Werror #-fsanitize=address
    )
    target_compile_options(client PRIVATE
        -Wall -Wextra -pedantic -Werror #-fsanitize=address
    )
    
endif()



# install(TARGETS join_server RUNTIME DESTINATION bin)

# set(CPACK_GENERATOR DEB)

# set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
# set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
# set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

# set(CPACK_PACKAGE_CONTACT alex-guerchoig@yandex.ru)

# include(CPack)

# enable_testing()
# include(GoogleTest)
# gtest_discover_tests(test_main_control)
# add_test(test_main_control  test_main_control)



