# 安装开发工具
yum -y groupinstall "development tools" && yum install openssl-devel elfutils-libelf-devel bc lz4 

#
yum -y libblkid-devel

# 安装内核
make bzImage
make modules
make modules_install
make install
make headers_install
reboot

# 安装模块
make
insmod hello_module.ko
modinfo hello_module.ko
rmmod hello_module
