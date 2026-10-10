# Windows PowerShell 5.1+. Does not modify firewall rules or system settings.
[CmdletBinding()]
param(
    [ValidateRange(0,65535)][int]$Port = 5514,
    [string]$BindAddress = '0.0.0.0',
    [string]$OutputDirectory = '',
    [string]$AmigaIP = '',
    [switch]$SelfTest,
    [ValidateRange(3,4096)][int]$TestPackets = 3
)
$ErrorActionPreference = 'Stop'
# Resolve after parameter binding: Windows PowerShell 5.1 can evaluate a
# parameter default using an empty $PSScriptRoot.
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    $OutputDirectory = $PSScriptRoot
    if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
        $OutputDirectory = Split-Path -Parent $MyInvocation.MyCommand.Path
    }
}
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
    throw 'Cannot determine the log directory. Supply -OutputDirectory explicitly.'
}
$udp = $null
$sender = $null
$writer = $null
$stream = $null
$received = 0
if ($SelfTest) { $BindAddress = '127.0.0.1'; $Port = 0; $AmigaIP = '127.0.0.1' }
$bindIP = [Net.IPAddress]::Parse($BindAddress)
if ($bindIP.AddressFamily -ne [Net.Sockets.AddressFamily]::InterNetwork) {
    throw 'Use an IPv4 bind address.'
}
if ($AmigaIP) { $AmigaIP = [Net.IPAddress]::Parse($AmigaIP).ToString() }
$null = [IO.Directory]::CreateDirectory($OutputDirectory)
$name = 'fpse-udp-{0}-{1}.log' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), ([Guid]::NewGuid().ToString('N').Substring(0,8))
$path = [IO.Path]::GetFullPath((Join-Path $OutputDirectory $name))
try {
    $udp = [Net.Sockets.UdpClient]::new([Net.Sockets.AddressFamily]::InterNetwork)
    $udp.ExclusiveAddressUse = $true
    $udp.Client.ReceiveBufferSize = 8388608
    $udp.Client.ReceiveTimeout = 500
    $udp.Client.Bind([Net.IPEndPoint]::new($bindIP, $Port))
    $stream = [IO.FileStream]::new($path, [IO.FileMode]::CreateNew,
        [IO.FileAccess]::Write, [IO.FileShare]::Read, 4096, [IO.FileOptions]::None)
    $writer = [IO.StreamWriter]::new($stream, [Text.UTF8Encoding]::new($false))
    $writer.AutoFlush = $true
    Write-Host ('Listening on UDP {0}. Start FPSE now; Ctrl+C stops the receiver.' -f $udp.Client.LocalEndPoint)
    Write-Host ('Log: {0}' -f $path)
    Write-Host 'Console is a preview only; ALL accepted packets go to the log file.'
    Write-Host ('Receive buffer: {0} bytes. Each line is flushed to Windows; no disk write-through per packet.' -f $udp.Client.ReceiveBufferSize)
    Write-Host 'No firewall rules changed. If required, allow inbound UDP 5514 from the Amiga on your trusted LAN only.'
    if ($SelfTest) {
        $sender = [Net.Sockets.UdpClient]::new([Net.Sockets.AddressFamily]::InterNetwork)
        foreach ($message in @('1 START loopback', '2 ENTER test', '3 LEAVE test')) {
            $data = [Text.Encoding]::ASCII.GetBytes($message)
            $null = $sender.Send($data, $data.Length, $udp.Client.LocalEndPoint)
        }
        for ($testIndex = 4; $testIndex -le $TestPackets; ++$testIndex) {
            $data = [Text.Encoding]::ASCII.GetBytes(('{0} BURST {1}' -f $testIndex, ('x' * 700)))
            $null = $sender.Send($data, $data.Length, $udp.Client.LocalEndPoint)
        }
    }
    $deadline = [DateTime]::UtcNow.AddSeconds(5)
    while ($true) {
        $remote = [Net.IPEndPoint]::new([Net.IPAddress]::Any, 0)
        try { $data = $udp.Receive([ref]$remote) }
        catch [Net.Sockets.SocketException] {
            if ($_.Exception.SocketErrorCode -ne [Net.Sockets.SocketError]::TimedOut) { throw }
            if ($SelfTest -and [DateTime]::UtcNow -gt $deadline) { throw 'UDP loopback test timed out.' }
            continue
        }
        if ($AmigaIP -and $remote.Address.ToString() -ne $AmigaIP) { continue }
        if ($data.Length -gt 1024) { continue }
        # Display/log text only, never interpret received content as commands.
        $message = [Text.Encoding]::ASCII.GetString($data).TrimEnd([char[]]"`r`n")
        $message = $message -replace '[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]', '?'
        $record = '{0:o} [{1}] {2}' -f [DateTime]::UtcNow, $remote, $message
        $writer.WriteLine($record)
        # AutoFlush already makes each line visible in the PC file. Forcing
        # a physical disk sync per packet stalls receiving during bursts.
        # An Amiga hang does not discard the Windows filesystem cache.
        ++$received
        if (($received -le 12 -and (-not $SelfTest -or $received -le 3)) -or ($received % 1000) -eq 0 -or
            $message -match 'START |OpenLibrary LEAVE|FAIL|TRACE LIMIT|END GPU session') { Write-Host $record }
        if ($SelfTest -and $received -eq $TestPackets) {
            # Read while the writer is STILL OPEN, before graceful cleanup.
            $reader = [IO.FileStream]::new($path, [IO.FileMode]::Open,
                [IO.FileAccess]::Read, [IO.FileShare]::ReadWrite)
            $textReader = [IO.StreamReader]::new($reader)
            try { $saved = $textReader.ReadToEnd() } finally { $textReader.Dispose() }
            foreach ($expected in @('1 START loopback', '2 ENTER test', '3 LEAVE test')) {
                if (-not $saved.Contains($expected)) { throw "Missing persisted packet: $expected" }
            }
            if ($TestPackets -gt 3 -and -not $saved.Contains("$TestPackets BURST")) {
                throw 'Missing final burst packet in the file.'
            }
            Write-Host ('PASS: {0} real UDP loopback packets saved before receiver shutdown.' -f $received)
            break
        }
        if ($stream.Length -ge 64MB) {
            Write-Warning 'Receiver log reached 64 MiB; stopping. Start a new capture if needed.'
            break
        }
    }
} finally {
    if ($writer) { $writer.Dispose() } elseif ($stream) { $stream.Dispose() }
    if ($udp) { $udp.Dispose() }
    if ($sender) { $sender.Dispose() }
    Write-Host ('Stopped. Received {0} packets. Log: {1}' -f $received, $path)
}
