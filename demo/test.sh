# 安装开发工具

# 安装内核
make modules_install
make install
make headers_install

# 安装模块
make
insmod hello_module.ko
modinfo hello_module.ko
rmmod hello_module
