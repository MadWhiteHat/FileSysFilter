param([string]$serv_name = $(throw "Enter service name"))

if ($serv_name) {
    $serv = Get-Service -Name $serv_name -ErrorAction SilentlyContinue
    if ($serv -eq $null) { Write-Host $serv_name "does not exist."}
    else { Write-Host $serv "exists." }
} else {
    Write-Host "Empty service name"
}