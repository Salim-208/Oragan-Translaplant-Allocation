param([string]$PgRoot = $env:PGROOT)

$ErrorActionPreference = 'Stop'
$Project = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Project
if (-not $PgRoot) { $PgRoot = 'C:\msys64\ucrt64\opt\pg-16' }
if (-not (Test-Path "$PgRoot\include\libpq-fe.h")) { throw "PostgreSQL headers not found: $PgRoot" }

$Compiler = (Get-Command g++ -ErrorAction SilentlyContinue).Source
if (-not $Compiler -and (Test-Path 'C:\msys64\ucrt64\bin\g++.exe')) { $Compiler = 'C:\msys64\ucrt64\bin\g++.exe' }
if (-not $Compiler) { throw 'Install MSYS2 UCRT64 g++ and try again.' }

$Out = Join-Path $Project 'build-nocmake'
$Deps = Join-Path $Out 'deps'
$Exe = Join-Path $Out 'organ_matching.exe'
foreach ($Name in @('organ_server','os_dbms_lab','organ_matching')) {
    foreach ($Process in (Get-Process $Name -ErrorAction SilentlyContinue)) {
        try {
            $OldExe = Join-Path $Out "$Name.exe"
            if ([IO.Path]::GetFullPath($Process.Path) -ieq [IO.Path]::GetFullPath($OldExe)) { Stop-Process $Process.Id -Force }
        } catch { }
    }
}
New-Item -ItemType Directory -Force -Path "$Deps\nlohmann" | Out-Null
$Headers = @(
    @{ Path = "$Deps\httplib.h"; Url = 'https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.18.3/httplib.h' },
    @{ Path = "$Deps\nlohmann\json.hpp"; Url = 'https://raw.githubusercontent.com/nlohmann/json/v3.11.3/single_include/nlohmann/json.hpp' }
)
foreach ($Header in $Headers) {
    if (-not (Test-Path $Header.Path)) { Invoke-WebRequest -Uri $Header.Url -OutFile $Header.Path }
}

$CompileArgs = @('-std=c++17','-O2','-Wall','-Wextra',"-I$Deps","-I$PgRoot\include",'src/main.cpp',"-L$PgRoot\lib",'-lpq','-lws2_32','-pthread','-o',$Exe)
& $Compiler @CompileArgs
if ($LASTEXITCODE -ne 0) { throw "Build failed (exit code $LASTEXITCODE). Stop any running organ_matching.exe and retry." }
Remove-Item -LiteralPath "$Out\organ_server.exe","$Out\organ_cli.exe","$Out\os_dbms_lab.exe" -Force -ErrorAction SilentlyContinue
Write-Host "Build complete: $Exe"
