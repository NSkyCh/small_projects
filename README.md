编译LINUX版本使用 build_linux.sh
编译单板版本使用 build_arm.sh

使用前请安装 jpeg 库 (安装包已在tool目录中)。
安装方式：
1.  tar -zxvf jpegsrc.v8c.tar.gz
2.  cd jpeg-8c
3.  ./configure --enable-shared
4.  make
5.  make install

交叉编译需将jpeg库安装至交叉编译工具中,否则编译错误
安装方式：
1.   tar -zxvf jpegsrc.v8c.tar.gz
2.   ./configure --host=arm-none-linux-gnueabi --prefix=指定路径 (注：检查生成的组件是否是交叉编译式的)
3.   将指定路径中的lib文件夹里动态库和include文件夹放至交叉编译工具的路径中 (具体根据自己所用的交叉编译器来操作)