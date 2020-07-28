# 查询主设备号 得到 242
cat /proc/devices |grep demo
# 生成 设备点  /dev/demo_cdev
mknod /dev/demo_cdrv c 242 0
# 删除 设备
#rm -rf /dev/demo_cdrv

gcc -o test demo.c 
