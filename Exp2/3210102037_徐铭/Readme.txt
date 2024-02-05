运行环境可选择任意能够编译运行C程序的IDE，如VScode，DevC++等均可，编译器为GCC-8.1.0，请确保电脑上已经安装了编译器，具体使用方法为：
1.在VScode中打开一个终端或者打开windows自带的命令行操作系统CMD；
2.在终端或命令行中，cd进入xxx.cpp源程序所在的文件夹；
3.输入命令：gcc xx.cpp header.hpp-o filename，可编译生成可执行文件，可执行文件名为filename；
4.接下来就可以真正的开始使用程序的功能，具体命令格式为：
      filename  operation  srImage_name  diImage_name
   其中，filename指的是3中生成的可执行文件的文件名，operation为使用者想要对原始图片执行的操作，srImage_name为原始图片的名字，diImage_name为操作结束后生成图片的名字，可自定义，但建议根据操作的名称和操作的顺序命名；
可以选择的operation有logarithm（对数化增强）使用时请输入相应的英文；