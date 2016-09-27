CTP后台服务程序
=========


操盘大哥v2.0的ctp后台

安装
===

要求gcc >= 5.4，推荐最新版

* 安装依赖：sudo yum install cmake hiredis-devel libev-devel

* 安装redox, 

* 将/usr/local/lib, /usr/local/lib64/ 加入 etc/ld.so.conf

* 将api目录中的so文件复制到/usr/lib64/

* sudo ldconfig
