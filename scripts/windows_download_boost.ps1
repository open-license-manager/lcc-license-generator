#!/usr/bin/env pwsh

# This script downloads boost. It must be invoked with 3 arguments
# windows_download_boost.ps1 <boost_version> <msvc version> <architecture>
# eg. windows_download_boost.ps1 1.91.0 14.3 64

if ($args.Count -ne 3) {
    Write-Host "Error: This script requires 3 parameters.\n windows_download_boost.ps1 <boost_version> <msvc version> <architecture>"
    exit
}
#set Invoke-WebRequest to not show progress bar, as it can cause issues in some CI environments
$ProgressPreference = 'SilentlyContinue'

[string] $version = $args[0]
[string] $msvc_version = $args[1]
[string] $architecture = $args[2]
[int]$maxRetries = 3
[int]$retryCount = 0
[int]$StatusCode = 200
$response = $null


$version_und = $version.Replace('.', '_')
$uri = "https://github.com/userdocs/boost/releases/download/boost-"+$version+"/boost_" + $version_und + "-msvc-" + $msvc_version + "-" + $architecture + ".exe"

if (-not (Test-Path 'C:/local/boost/libs')) {
	echo "Boost not cached, downloading it: $uri"
    do {
        try {
                $response = Invoke-WebRequest -Verbose -Debug -Uri "$uri" -OutFile ".\boost.exe"
                break
            } catch {
                $StatusCode = $_.Exception.Response.StatusCode
                $errorMessage = $_.Exception.Message
                $retryCount++
                echo "Attempt $retryCount failed: $StatusCode $errorMessage. Retrying $uri ..."
                Start-Sleep -Seconds 2  # Wait before retrying
            }
        } until ($retryCount -ge $maxRetries)

        if ($response) {
            echo "Boost downloaded"
        } else {
            echo "Request failed after $retryCount attempts."
            #exit 1
        }
} else { echo "Boost already installed" }

if (Test-Path './boost.exe') {
    echo "Boost executable found."
    dir
} else {
    echo "Error: Boost executable not found."
}