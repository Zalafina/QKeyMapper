param([Parameter(Mandatory)][string]$ExecutablePath,[string]$Arguments='',[string]$WorkingDirectory,[Parameter(Mandatory)][string]$LogPath,[int]$TimeoutSeconds=240)
$ErrorActionPreference='Stop'
if(-not $WorkingDirectory){$WorkingDirectory=Split-Path -Parent $ExecutablePath}
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;
public static class QkmManualPrivateRunner {
 [StructLayout(LayoutKind.Sequential)] struct Security {public int Size;public IntPtr Descriptor;[MarshalAs(UnmanagedType.Bool)]public bool Inherit;}
 [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] struct Startup {public int Size;public string Reserved,Desktop,Title;public int X,Y,Width,Height,XChars,YChars,Fill,Flags;public short Show,Reserved2;public IntPtr ReservedBytes,Input,Output,Error;}
 [StructLayout(LayoutKind.Sequential)] struct ProcessInfo {public IntPtr Process,Thread;public uint Pid,Tid;}
 delegate bool WindowProc(IntPtr hwnd,IntPtr data);
 [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)]static extern IntPtr CreateDesktop(string name,IntPtr device,IntPtr mode,uint flags,uint access,IntPtr security);
 [DllImport("user32.dll")]static extern bool CloseDesktop(IntPtr desktop);
 [DllImport("user32.dll")]static extern bool EnumDesktopWindows(IntPtr desktop,WindowProc proc,IntPtr data);
 [DllImport("user32.dll")]static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint pid);
 [DllImport("user32.dll")]static extern IntPtr GetThreadDesktop(uint tid);
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern bool GetUserObjectInformation(IntPtr handle,int index,StringBuilder value,int size,out int need);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]static extern IntPtr CreateFile(string path,uint access,uint share,ref Security security,uint creation,uint flags,IntPtr template);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)]static extern bool CreateProcess(string exe,StringBuilder command,IntPtr processSecurity,IntPtr threadSecurity,bool inherit,uint flags,IntPtr environment,string directory,ref Startup startup,out ProcessInfo info);
 [DllImport("kernel32.dll")]static extern uint WaitForSingleObject(IntPtr handle,uint timeout);
 [DllImport("kernel32.dll")]static extern bool GetExitCodeProcess(IntPtr process,out uint code);
 [DllImport("kernel32.dll")]static extern bool CloseHandle(IntPtr handle);
 public static int Run(string exe,string args,string directory,string log,int timeout) {
  string name="QkmManual_"+Guid.NewGuid().ToString("N");var desktop=CreateDesktop(name,IntPtr.Zero,IntPtr.Zero,0,0x000F01FF,IntPtr.Zero);
  if(desktop==IntPtr.Zero)throw new Win32Exception();var security=new Security{Size=Marshal.SizeOf(typeof(Security)),Inherit=true};
  var output=CreateFile(log,0x40000000,3,ref security,2,0x80,IntPtr.Zero);var input=CreateFile("NUL",0x80000000,3,ref security,3,0x80,IntPtr.Zero);
  var info=new ProcessInfo();var seen=new HashSet<string>();
  try {
   if(output==new IntPtr(-1)||input==new IntPtr(-1))throw new Win32Exception();
   var startup=new Startup{Size=Marshal.SizeOf(typeof(Startup)),Desktop="WinSta0\\"+name,Flags=0x100,Input=input,Output=output,Error=output};
   if(!CreateProcess(exe,new StringBuilder("\""+exe+"\" "+args),IntPtr.Zero,IntPtr.Zero,true,0x08000000,IntPtr.Zero,directory,ref startup,out info))throw new Win32Exception();
   Console.WriteLine("Private desktop="+name+" launcher PID="+info.Pid);
   var deadline=DateTime.UtcNow.AddSeconds(timeout);
   while(WaitForSingleObject(info.Process,50)==258) {
    EnumDesktopWindows(desktop,(IntPtr hwnd,IntPtr data)=>{uint pid;uint tid=GetWindowThreadProcessId(hwnd,out pid);var b=new StringBuilder(256);int need;bool ok=GetUserObjectInformation(GetThreadDesktop(tid),2,b,512,out need);string row="HWND="+hwnd+" PID="+pid+" desktop="+(ok?b.ToString():"unverified");if(seen.Add(row))Console.WriteLine(row);return true;},IntPtr.Zero);
    if(DateTime.UtcNow>deadline)throw new TimeoutException("Owned private process remains running; PID="+info.Pid);
   }
   uint code;GetExitCodeProcess(info.Process,out code);Console.WriteLine("Native exit="+code);return unchecked((int)code);
  } finally {if(info.Thread!=IntPtr.Zero)CloseHandle(info.Thread);if(info.Process!=IntPtr.Zero)CloseHandle(info.Process);if(input!=new IntPtr(-1))CloseHandle(input);if(output!=new IntPtr(-1))CloseHandle(output);CloseDesktop(desktop);}
 }
}
'@
$exitCode=[QkmManualPrivateRunner]::Run([IO.Path]::GetFullPath($ExecutablePath),$Arguments,[IO.Path]::GetFullPath($WorkingDirectory),[IO.Path]::GetFullPath($LogPath),$TimeoutSeconds)
exit $exitCode
