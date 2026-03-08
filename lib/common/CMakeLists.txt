project(common LANGUAGES C)

add_library(${PROJECT_NAME} INTERFACE)
add_library(lib::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} INTERFACE ${PROJECT_SOURCE_DIR}/include)
