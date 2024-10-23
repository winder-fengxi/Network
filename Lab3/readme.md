# Lab3文件  

这是厦门大学黄炜老师的计算机网络课程的实验三的个人代码文件  

## 文件夹目录说明  

本实验共有四个小实验，其中第三个和第四个涉及到代码部分，第一个和第二个实验使用wiresshark即可完成。  

## Lab3文件说明  

Lab3为用 Libpcap 或 WinPcap 库侦听网络数据；解析侦听到的网络数据。请注意，在win11版本中WinPcap已经不支持，请使用[NPcap](https://npcap.com/),include和lib在其sdk中。  

程序运行之后会出现设备选择,Wi-Fi 连接：如果你通过 Wi-Fi 上网，选择设备 5: Intel(R) Wi-Fi 6E AX211 160MHz。  
有线以太网连接：如果你使用的是有线连接（插入网线），选择设备 10: Realtek PCIe GbE Family Controller。  
虚拟网络（例如 VMware 或 VPN）：如果你想捕获虚拟机或 VPN 相关的流量，可以选择设备 6: VMware Virtual Ethernet Adapter for VMnet8 或 7: VMware Virtual Ethernet Adapter for VMnet1，或 11: Sangfor SSL VPN CS Support System VNIC。  

在选择完设备之后，根据实验的要求打开相应的网站或服务器，监听结果会在终端显示，也会在excel文件中显示

