cmake_minimum_required( VERSION 3.0)
project(main)

# 配置编译选项  -g可调试
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g ")

# 配置最终的可执行文件输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 配置头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)

# # 配置库文件搜索路径
# link_directories(${PROJECT_SOURCE_DIR}/lib)

# 指定搜索的子目录 
add_subdirectory(src)

# CMakeLists.txt