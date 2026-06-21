param(
    [string]$MsysPath = "C:\msys64"
)

Write-Host "===================================================" -ForegroundColor Cyan
Write-Host "       Скрипт настройки окружения для проекта      " -ForegroundColor Cyan
Write-Host "===================================================" -ForegroundColor Cyan
Write-Host ""

# Проверка наличия MSYS2
if (-Not (Test-Path "$MsysPath\usr\bin\pacman.exe")) {
    Write-Host "[ОШИБКА] MSYS2 не найден по пути $MsysPath!" -ForegroundColor Red
    Write-Host "Пожалуйста, скачайте и установите MSYS2 с официального сайта (https://www.msys2.org/)." -ForegroundColor Yellow
    Write-Host "Оставьте путь установки по умолчанию (C:\msys64)." -ForegroundColor Yellow
    Write-Host "Если MSYS2 установлен в другое место, запустите скрипт так:" -ForegroundColor Yellow
    Write-Host ".\setup_env.ps1 -MsysPath 'Ваш\Путь\К\msys64'" -ForegroundColor Yellow
    exit 1
}

Write-Host "[1/3] Обновление баз данных MSYS2..." -ForegroundColor Cyan
& "$MsysPath\usr\bin\bash.exe" -lc "pacman -Syu --noconfirm"

Write-Host ""
Write-Host "[2/3] Установка необходимых пакетов для сборки (компилятор, CMake, Ninja, Qt 6, Git)..." -ForegroundColor Cyan
$packages = @(
    "mingw-w64-ucrt-x86_64-gcc",
    "mingw-w64-ucrt-x86_64-cmake",
    "mingw-w64-ucrt-x86_64-ninja",
    "mingw-w64-ucrt-x86_64-qt6-base",
    "mingw-w64-ucrt-x86_64-qt6-websockets",
    "mingw-w64-ucrt-x86_64-qt6-multimedia",
    "git"
)

$pkgString = $packages -join " "
& "$MsysPath\usr\bin\bash.exe" -lc "pacman -S --needed --noconfirm $pkgString"

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ОШИБКА] Произошла ошибка при установке пакетов!" -ForegroundColor Red
    exit 1
}

Write-Host "Пакеты успешно установлены!" -ForegroundColor Green
Write-Host ""
Write-Host "[3/3] Проверка переменных среды PATH..." -ForegroundColor Cyan

$ucrtBin = "$MsysPath\ucrt64\bin"
$usrBin = "$MsysPath\usr\bin"

$currentPath = [Environment]::GetEnvironmentVariable("PATH", "User")
$pathsArray = $currentPath -split ";"

$missingPaths = @()

$ucrtFound = $false
$usrFound = $false

foreach ($p in $pathsArray) {
    if ($p.TrimEnd('\/') -eq $ucrtBin.TrimEnd('\/')) { $ucrtFound = $true }
    if ($p.TrimEnd('\/') -eq $usrBin.TrimEnd('\/')) { $usrFound = $true }
}

if (-not $ucrtFound) { $missingPaths += $ucrtBin }
if (-not $usrFound) { $missingPaths += $usrBin }

if ($missingPaths.Count -gt 0) {
    Write-Host "Для корректной работы VS Code и CMake необходимо добавить следующие пути в переменную среды PATH:" -ForegroundColor Yellow
    foreach ($p in $missingPaths) {
        Write-Host "  -> $p"
    }
    Write-Host ""
    
    $title = "Добавление в PATH"
    $message = "Добавить эти пути в переменную PATH для текущего пользователя?"
    $yes = new-Object System.Management.Automation.Host.ChoiceDescription "&Да","Добавить пути в PATH."
    $no = new-Object System.Management.Automation.Host.ChoiceDescription "&Нет","Не добавлять."
    $options = [System.Management.Automation.Host.ChoiceDescription[]]($yes, $no)
    $result = $host.ui.PromptForChoice($title, $message, $options, 0) 

    if ($result -eq 0) {
        $newPath = $currentPath
        if (-not $newPath.EndsWith(";")) {
            $newPath += ";"
        }
        $newPath += ($missingPaths -join ";")
        [Environment]::SetEnvironmentVariable("PATH", $newPath, "User")
        Write-Host "Пути успешно добавлены в PATH! ВАЖНО: Перезапустите VS Code и другие открытые терминалы, чтобы изменения вступили в силу." -ForegroundColor Green
    } else {
        Write-Host "Вы отказались от добавления путей. Вам нужно будет добавить их вручную, иначе сборка проекта может не работать." -ForegroundColor Yellow
    }
} else {
    Write-Host "Все необходимые пути уже присутствуют в PATH." -ForegroundColor Green
}

Write-Host ""
Write-Host "===================================================" -ForegroundColor Cyan
Write-Host " Настройка завершена! Можно приступать к сборке." -ForegroundColor Cyan
Write-Host "===================================================" -ForegroundColor Cyan
