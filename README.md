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
