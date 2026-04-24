Option Explicit
Dim WshShell, exeName, rc
Set WshShell = CreateObject("WScript.Shell")

If WScript.Arguments.Count = 0 Then
    WScript.Quit 1
End If

exeName = WScript.Arguments(0)
rc = WshShell.Run("taskkill /f /t /im """ & exeName & """", 0, True)