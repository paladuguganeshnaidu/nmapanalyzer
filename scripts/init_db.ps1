# PowerShell helper to build and run init_db
Param()

Write-Output "Building init_db.exe..."
gcc -Iinclude -o ..\bin\init_db.exe ..\src\init_db.c ..\src\sqlite3.c -lws2_32
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed"; exit $LASTEXITCODE }

Write-Output "Running init_db.exe..."
..\bin\init_db.exe

Write-Output "Done. Check data\scans.db"
