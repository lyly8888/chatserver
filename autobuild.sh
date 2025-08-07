
#!/bin/bash

# set -x

# rm -rf 'pwd'/build/*

# cd 'pwd'/build &&
#	cmake .. &&
#	make

#!/bin/bash

set -x

# 获取当前目录的绝对路径
BUILD_DIR="$(pwd)/build"

# 清空 build 目录
rm -rf "${BUILD_DIR}"/*

# 创建并进入 build 目录
mkdir -p "${BUILD_DIR}" && cd "${BUILD_DIR}" &&
	cmake .. &&
        make
