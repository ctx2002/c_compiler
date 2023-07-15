#this file must be run inside of developer powershell for visual studio
function createASMFile() {
    param(
        [int]$expected,
        [int]$arg
    )
    Write-Host "Create a ASM file"
    .\c_compiler $arg  > tmp.asm
    nasm -f win64 tmp.ASM
    link .\tmp.obj /ENTRY:main /SUBSYSTEM:console
    .\tmp.exe
    if ($LASTEXITCODE -eq $expected) {
        Write-Host "$LASTEXITCODE ==> $expected"
    } else {
        Write-Host "input is $arg, actual is $LASTEXITCODE,  expected $expected"
    }
}

createASMFile 0 0
createASMFile 12 12

 Write-Host "OK"