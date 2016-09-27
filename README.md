CTP后台服务程序
=========


操盘大哥v2.0的ctp后台

安装
===

推荐使用最新版gcc编译本程序(gcc >= 6.2)

* 如果新版gcc装到了/usr/local/bin下面，记得修改CC和CXX变量，指向新版位置:
    
    export CC=/usr/local/bin/gcc
    export CXX=/usr/local/bin/g++

* 安装依赖：sudo yum install cmake hiredis-devel libev-devel boost-devel

* 安装[redox](https://github.com/hmartiro/redox)

* 将/usr/local/lib, /usr/local/lib64/ 加入 etc/ld.so.conf

* 将api目录中的so文件复制到/usr/lib64/, 执行sudo ldconfig刷新一下

* 进去源码目录，执行标准cmake安装:

    mkdir build && cd build && cmake .. 
    
    make && sudo make install
    
