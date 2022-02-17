
$NDKPath = Get-Content $PSScriptRoot/ndkpath.txt

$stackScript = "$NDKPath/ndk-stack"
if (-not ($PSVersionTable.PSEdition -eq "Core")) {
    $stackScript += ".cmd"
}
$filename = "tombstone_0" + $args[0]
& adb pull /storage/emulated/0/Android/data/com.beatgames.beatsaber/files/$($filename)
Get-Content $filename | & $stackScript -sym ./build/debug/ > "$($filename)_unstripped.txt"
& code "$($filename)_unstripped.txt"