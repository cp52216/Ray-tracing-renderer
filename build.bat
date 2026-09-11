@echo off
rem ============================================================
rem  Fortune Renderer 一键构建脚本
rem  为什么用 Release：路径追踪是纯计算密集型代码（每秒数千万次
rem  三角形求交 + 矩阵运算），Debug 配置是 /Od（完全不优化）+
rem  /RTC1（运行时内存检查），比 Release 的 /O2（循环展开、内联、
rem  向量化、寄存器分配优化）慢 10~30 倍。切换 Release 只是让编译器
rem  生成更快的机器码，浮点语义和计算逻辑不变，渲染结果与 Debug 一致。
rem ============================================================
if not exist .\build mkdir build
if exist .\build\CMakeCache.txt del .\build\CMakeCache.txt

rem 配置（生成 VS 解决方案）
cmake -S . -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if %errorlevel% neq 0 goto :error

rem 以 Release 编译：--parallel 让 MSVC 并行编译多个 .cpp（缩短编译等待，
rem 不影响运行性能）；产物带 /O2 优化，位置在 build\Release\
cmake --build build --config Release --parallel
if %errorlevel% neq 0 goto :error

echo.
echo ============================================
echo  编译成功！可执行文件：
echo    build\Release\FortuneRenderer.exe
echo ============================================
goto :eof

:error
echo.
echo 编译失败，请检查上方错误信息。
exit /b 1
