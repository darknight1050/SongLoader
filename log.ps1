if (Test-Path ./log.log)
{
    remove-item log.log -Force
}
& adb logcat -c
& adb logcat > log.log