set(SOL_NO_WIDE_STRING 1)

FILE(GLOB SOURCES src/cpp/*.cpp) #src/cpp/*.h
add_library(effil STATIC ${SOURCES})

target_include_directories(effil PUBLIC ${LUA_INCLUDE_DIR})
#target_link_libraries(effil chacha)
target_link_libraries(effil PRIVATE "-Wl,--no-as-needed" "-ldl" "-lpthread")

set(EFFIL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/cpp/ PARENT_SCOPE)



