#!/bin/sh

# 改变当前目录下所有 .sh 文件的权限为 777
chmod 777 *.sh

# 改变 CarClean 程序的权限为 777
chmod 777 CarClean

# 将 S44_StaticIP.sh 和 S97_Car.sh 移动到 /etc/init.d/ 目录下
mv S44_StaticIP.sh /etc/init.d/
mv S97_Car.sh /etc/init.d/

echo "Permissions updated and files moved successfully."