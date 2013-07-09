@echo off

set FXC="C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Utilities\bin\x86\fxc.exe"
set ASSEMBLER="C:\Code\Myst\Plasma\build\bin\RelWithDebInfo\plShaderAssembler.exe"

IF "%1"=="" GOTO NoShader
set SHADERNAME=%1

IF "%2"=="" GOTO NoEntryPoint
set ENTRYPOINT=%2

IF "%3"=="" GOTO NoShaderType
set SHADERPROFILE=%3

%FXC% /Fc %SHADERNAME%.inl /E %ENTRYPOINT% /T %SHADERPROFILE% %SHADERNAME%.fx
%ASSEMBLER% %SHADERNAME%.inl
move %SHADERNAME%.h ../
GOTO Finish

REM "Built shader %SHADERNAME%"
GOTO Finish

:NoShader
REM "No shader given"
GOTO Finish

:NoEntryPoint
REM "No shader given"
GOTO Finish

:NoShaderType
REM "No shader type given"
GOTO Finish

:Finish