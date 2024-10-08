# Lab2文件  

这是厦门大学黄炜老师的计算机网络课程的实验二的个人代码文件  

## bin文件夹  

bin文件夹是可执行文件夹  
在使用前请确保电脑安装了ffmpeg库（问就是我直接调用的命令行）  
code.exe文件包含了三个功能： 
1. encode, 编码功能，读取二进制文件并输出为RGB图片，并转换为视频。使用范例：'.\code.exe encode simple 1500 video.mp4 5000'
2. decode,解码功能 ，读取视频帧图片并将图片解码为bin文件，同时生成校准文件，使用范例 ：'.\code.exec decode video.mp4 result'  

因opencv_world4100d.dll文件过大，无法传入githunb，需要自行补充。

## src文件夹  

src为源vs工程文件，主要程序在main.cpp，其他为opencv配置库
