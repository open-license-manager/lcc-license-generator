#!/usr/bin/env pwsh

# This script downloads boost. It must be invoked with 3 arguments
# windows_download_boost.ps1 <boost_version> <msvc version> <architecture>
# eg. windows_download_boost.ps1 1.91.0 14.3 64

if ($args.Count -ne 3) {
    Write-Host "Error: This script requires 3 parameters.\n windows_download_boost.ps1 <boost_version> <msvc version> <architecture>"
    exit
}
[string] $version = $args[0]
[string] $msvc_version = $args[1]
[string] $architecture = $args[2]
$version_und = $version.Replace('.', '_')
$uri = "https://github.com/userdocs/boost/releases/download/boost-"+$version+"/boost_" + $version_und + "-msvc-" + $msvc_version + "-" + $architecture + ".exe"

if (-not (Test-Path 'C:/local/boost/libs')) {
	echo "Boost not cached, downloading it"
	$maxRetries = 3
    $retryCount = 0
    $response = $null

    do {
        try {
                $response = Invoke-WebRequest -Uri $uri -UseBasicParsing -OutFile boost.exe
                # If the request is successful, exit the loop
                break
            } catch {
                [int]$StatusCode = $_.Exception.Response.StatusCode
                $errorMessage = $_.Exception.Message
                $retryCount++
                echo "Attempt $retryCount failed: $StatusCode $errorMessage. Retrying $uri ..."
                Start-Sleep -Seconds 2  # Wait before retrying
            }
        } until ($retryCount -ge $maxRetries)

        if ($response) {
            echo "Boost downloaded"
        } else {
            echo "Request failed after $maxRetries attempts."
            exit 1
        }
} else { echo "Boost already installed" }