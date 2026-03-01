param(
    [string]$ComPort,
    [switch]$IncludeDebug,
    [switch]$DryRun
)

$ErrorActionPreference = 'Stop'

function Write-Info {
    param([string]$Message)
    Write-Host "[info] $Message"
}

function Write-WarnMsg {
    param([string]$Message)
    Write-Host "[warn] $Message" -ForegroundColor Yellow
}

function Get-CandidateProcesses {
    $all = Get-CimInstance Win32_Process

    $patterns = @(
        'platformio\.exe\s+device\s+monitor',
        'platformio\.exe\s+monitor',
        'python\.exe.+platformio\.exe.+device\s+monitor'
    )

    if ($IncludeDebug) {
        $patterns += @(
            'platformio\.exe\s+debug',
            'python\.exe.+platformio\.exe.+debug'
        )
    }

    $candidates = foreach ($proc in $all) {
        $cmd = [string]$proc.CommandLine
        if ([string]::IsNullOrWhiteSpace($cmd)) {
            continue
        }

        $isMatch = $false
        foreach ($pattern in $patterns) {
            if ($cmd -match $pattern) {
                $isMatch = $true
                break
            }
        }

        if (-not $isMatch) {
            continue
        }

        if ($ComPort) {
            $normalizedPort = $ComPort.ToUpperInvariant()
            if ($cmd.ToUpperInvariant() -notmatch [regex]::Escape($normalizedPort)) {
                continue
            }
        }

        [PSCustomObject]@{
            ProcessId   = $proc.ProcessId
            Name        = $proc.Name
            CommandLine = $cmd
        }
    }

    $candidates | Sort-Object ProcessId -Unique
}

Write-Info "Scanning for serial-port lock candidates..."
if ($ComPort) {
    Write-Info "Filtering by port hint: $ComPort"
}
if ($IncludeDebug) {
    Write-Info "Including debug sessions in candidate list"
}

$targets = Get-CandidateProcesses

if (-not $targets -or $targets.Count -eq 0) {
    Write-Info "No matching processes found."
    exit 0
}

Write-Info "Found $($targets.Count) candidate process(es):"
$targets | Format-Table ProcessId, Name -AutoSize | Out-Host

foreach ($target in $targets) {
    $summary = "PID $($target.ProcessId) $($target.Name)"
    if ($DryRun) {
        Write-Info "[dry-run] Would stop $summary"
        continue
    }

    try {
        Stop-Process -Id $target.ProcessId -Force
        Write-Info "Stopped $summary"
    }
    catch {
        Write-WarnMsg "Failed to stop $summary :: $($_.Exception.Message)"
    }
}

if ($DryRun) {
    Write-Info "Dry run completed. Re-run without -DryRun to stop processes."
} else {
    Write-Info "Done. Retry upload/debug now."
}
