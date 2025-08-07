
# 定义了一个 SRC_LIST 变量，里面存放的是该目录下的源文件, 不包括子文件

aux_source_directory(. SRC_LIST)

# 指定生成的可执行文件的名字 以及 所依赖的源文件
add_executable(ChatClient ${SRC_LIST} )

# 指定可执行文件链接时所依赖的其它库
target_link_libraries(ChatClient muduo_base pthread)