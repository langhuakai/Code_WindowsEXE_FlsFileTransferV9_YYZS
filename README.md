# 开发步骤
1、安装SCENE_API_Standalone_Developer_Package_2.1.4.14969_Setup.exe
2、将安装目录下的include和lib文件夹复制到项目目录
3、注释掉lsscan.h文件下的CORE_API_EXPORT LSPod<std::vector<ref_ptr<LSPanoramaPicture>>> getPanos() const;
4、CMake编译产生exe文件
5、在SCENE_API安装的bin目录下找到SCENE Redistributable Package 2.1.4.14969 x64.msi和vcredist_x64_2022.exe文件并安装到要运行转换程序的windows电脑上

# 转换fls文件使用实例
C:\Users\Administrator\Desktop>Code_WindowsEXE_FlsFileTransferV9_YYZS.exe -i C:\\Users\\Administrator\\Desktop\\V9-OldRock-001.fls -o E:\\temp\\output\\test2.ply
参数说明
Code_WindowsEXE_FlsFileTransferV9_YYZS.exe：编译后产生的exe程序
-i：后面跟要转换的fls文件路径
-o：后面跟转换后输出的文件路径

# 支持e57点云输入使用实例
C:\Users\Administrator\Desktop>Code_WindowsEXE_FlsFileTransferV9_YYZS.exe -i D:\\data\\scan.e57 -o E:\\temp\\output\\test.ply -f ply
说明
-i：后面跟要转换的输入文件路径，支持 fls / scn / e57（以 .fls 结尾的文件夹当成单个 fls 文件处理）
-o：后面跟转换后输出的文件路径；给目录时为批处理模式，配合 -f 指定输出格式
-f：输出格式，默认 ply。e57 转 ply 就是 -i xxx.e57 -o xxx.ply -f ply

# 输出点云带反射强度
转换时同时加载颜色与反射强度，输出点云在带颜色的同时也带反射强度。各格式实测结果：
ply：顶点属性 x y z red green blue intensity，intensity 为 0~1 的 float（二进制小端，每点 19 字节）
e57：点原型含 intensity 与 colorRed/Green/Blue
las：pointFormat=2，intensity 为 uint16（归一化值乘以 65535），同时带 RGB
pts：7 列 x y z i r g b，i 为 SDK 原始整数值
ptx：7 列 x y z i r g b，i 为 0~1
xyz：9 列 row col x y z r g b i，i 为 0~255
off：OFF 格式规范没有反射强度通道，仍只写 x y z r g b
注意
1、ply 顶点属性比旧版多了 intensity，下游解析程序需按新属性表读取
2、SCENE 的 e57 导入器不读取 intensity 通道，所以 e57 输入转 ply 时只有颜色、没有反射强度
3、ply 的颜色与反射强度分两趟读取后合并；若两趟点数或坐标不一致，自动回退为只带颜色的 ply
