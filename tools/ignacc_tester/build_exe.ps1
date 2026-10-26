# Build IGNACC_Tester.exe (single file, no console window).
#
#   powershell -ExecutionPolicy Bypass -File tools\ignacc_tester\build_exe.ps1
#   powershell -ExecutionPolicy Bypass -File tools\ignacc_tester\build_exe.ps1 -Python C:\Path\to\python.exe
#
# Needs a Python 3.9+ with Tkinter. Output: tools\ignacc_tester\dist\IGNACC_Tester.exe
param(
    [string]$Python = "C:\Python3_13\python.exe"
)
$ErrorActionPreference = "Stop"
$here = $PSScriptRoot
$venv = Join-Path $here ".venv"
$py = Join-Path $venv "Scripts\python.exe"

if (-not (Test-Path $py)) {
    Write-Host "Creating build venv in $venv"
    & $Python -m venv $venv
    if ($LASTEXITCODE -ne 0) { throw "venv creation failed" }
}
& $py -m pip install --disable-pip-version-check -q -r (Join-Path $here "requirements.txt")
if ($LASTEXITCODE -ne 0) { throw "pip install failed" }

& $py -m PyInstaller --noconfirm --clean --onefile --windowed `
    --name IGNACC_Tester `
    --distpath (Join-Path $here "dist") `
    --workpath (Join-Path $here "build") `
    --specpath (Join-Path $here "build") `
    (Join-Path $here "ignacc_tester_gui.py")
if ($LASTEXITCODE -ne 0) { throw "PyInstaller failed" }

Write-Host "Built: $(Join-Path $here 'dist\IGNACC_Tester.exe')"
