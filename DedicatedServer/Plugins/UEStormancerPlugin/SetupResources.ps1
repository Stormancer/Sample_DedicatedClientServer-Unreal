param(
    [string]$DownloadPath = "https://github.com/Stormancer/Sample_DedicatedClientServer-Unreal/releases/download",
    [string]$Version = "1.0", 
    [string]$FileName = "Resources.zip", 
    [string]$InstallPath = "C:\Workspace\Sample_DedicatedClientServer-Unreal\DedicatedServer\Plugins\UEStormancerPlugin\Resources"
)

Write-Host "DownloadPath = $($DownloadPath)"
Write-Host "Version = $($Version)"
Write-Host "FileName = $($FileName)"
Write-Host "InstallPath = $($InstallPath)"

$TempFolder
New-TemporaryFile | %{ rm $_; $TempFolder = $_; mkdir $_ } | Out-Null
$TempDownload = "$($TempFolder)\$($FileName)"

Function DownloadFile 
{
	Write-Host "Downloading $($FileName) ..."
	$client = New-Object System.Net.WebClient
	$client.DownloadFile($FullDownloadName, $TempDownload)
} 

$FullDownloadName = "$($DownloadPath)/$($Version)/$($FileName)"

DownloadFile

Write-Host "Expanding the archive ..."

#The archive is expand in the root so we need to install it in it's directory and then clean the empty directory
#Clean old Resources in InstallPath
if(Test-Path -Path $InstallPath) 
{
	#Maybe dangerous, just delete the old Resources of what we are searching
	Remove-Item $InstallPath -Force -recurse
}
$ExpandFolderName = (Get-Item $TempDownload ).Basename
$expandPath = "$($TempFolder)\$($ExpandFolderName)"
New-Item -ItemType Directory -Force -Path $expandPath | Out-Null
Expand-Archive -Path $TempDownload -DestinationPath $expandPath

# Get expand Content path
if(!(test-path $InstallPath))
{
    New-Item -ItemType Directory -Force -Path $InstallPath | Out-Null
}
$versionFile = (Get-Item $expandPath ).Basename
New-Item -ItemType File -Force -Path "$($InstallPath)\$($versionFile)"  | Out-Null
Get-ChildItem -Path $expandPath | Move-Item -Destination $InstallPath
