# Installing Make on Windows

## Option 1: Enable WSL2 (Recommended)

You need to run these commands as **Administrator** in PowerShell:

```powershell
# Enable WSL feature
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart

# Enable Virtual Machine Platform
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart

# Restart your computer
Restart-Computer
```

After restart:
```powershell
# Set WSL2 as default
wsl --set-default-version 2

# Install Ubuntu
wsl --install -d Ubuntu
```

## Option 2: Install Make via Chocolatey

```powershell
# Install Chocolatey (as Administrator)
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))

# Install make
choco install make
```

## Option 3: Use MinGW with Make

Install MinGW-w64 which includes make:
1. Download from: https://www.mingw-w64.org/downloads/
2. Or use MSYS2: https://www.msys2.org/

## Option 4: Direct Compilation (Current Workaround)

For now, you can compile directly with g++:

```bash
# In project root
cd optimality/01_layer_mapping
g++ -std=c++17 -Wall -Wextra -g -Iinclude src/Graph.cc main.cc -o graph_demo

cd ../02_layer_planner  
g++ -std=c++17 -Wall -Wextra -g -I../01_layer_mapping/include -Iinclude src/*.cc main.cc ../01_layer_mapping/src/Graph.cc -o planner
```