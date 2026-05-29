# CMakeList.txt: Code_WindowsEXE_FlsFileTransferV9_YYZS 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(Code_WindowsEXE_FlsFileTransferV9_YYZS
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "FARO SCENE File Transfer Tool V9"
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows特定设置
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE)
    add_compile_options(/W3 /EHsc /Zc:wchar_t /Zc:inline /permissive-)
    
    # 关闭增量链接，减少部署问题
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /INCREMENTAL:NO")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:YES")
endif()

# 搜索源文件
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST)

# 添加可执行文件
add_executable(${PROJECT_NAME} ${SRC_LIST} "src/ArgumentParser.h" "src/ArgumentParser.cpp" "src/StringConverter.cpp" "src/StringConverter.h" "src/FlsToPly.h" "src/FlsToPly.cpp" "src/FlsToE57.h" "src/FlsToE57.cpp" "src/FlsToOff.h" "src/FlsToOff.cpp")

# 头文件包含
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 导入库
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/Core_API.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/x64/Standalone_API.lib
)


# 添加Windows系统库
if(WIN32)
    link_libraries(
        kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib
        advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib
        odbc32.lib odbccp32.lib shell32.lib
    )
endif()

# FARO SCENE Redistributable Manifest
# ----------------------------------------------------------------
set(FARO_MANIFEST_CONTENT
"<?xml version='1.0' encoding='UTF-8' standalone='yes'?>
<assembly xmlns='urn:schemas-microsoft-com:asm.v1' manifestVersion='1.0'>
  <dependency>
    <dependentAssembly>
      <assemblyIdentity
        type='win32'
        name='FARO.SCENE.Redistributable'
        version='2.1.0.0'
        processorArchitecture='amd64'
        publicKeyToken='1d23f5635ba800ab'
        language='*'
      />
    </dependentAssembly>
  </dependency>
</assembly>"
)

# 创建manifest文件
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.manifest"
    "${FARO_MANIFEST_CONTENT}")

# 附加manifest到可执行文件
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_ProjectApplicationManifest
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.manifest"
)

# 在构建时嵌入manifest
if(MSVC)
    add_compile_options(/WX-)  # 禁用将警告视为错误
    target_sources(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.manifest"
    )
endif()

# ----------------------------------------------------------------
# 部署配置
# ----------------------------------------------------------------
# 创建部署目标
set(DEPLOY_DIR "${CMAKE_CURRENT_BINARY_DIR}/Deploy")
set(REDIST_DIR "${CMAKE_CURRENT_BINARY_DIR}/Redist")

# 创建部署目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${REDIST_DIR}"
    COMMENT "创建部署目录"
)

# 复制可执行文件到部署目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:${PROJECT_NAME}>"
        "${DEPLOY_DIR}/"
    COMMENT "复制可执行文件到部署目录"
)

# 复制manifest到部署目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.manifest"
        "${DEPLOY_DIR}/"
    COMMENT "复制manifest文件"
)














