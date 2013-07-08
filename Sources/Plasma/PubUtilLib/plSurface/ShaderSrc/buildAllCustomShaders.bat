@echo off

call buildShader.bat ps_Passthrough ps_main ps_3_0
call buildShader.bat vs_Passthrough vs_main vs_3_0
call buildShader.bat ps_RiftDistortAssembly main vs_3_0
call buildShader.bat vs_RiftDistortAssembly main ps_3_0