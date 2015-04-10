; Java Launcher with automatic JRE installation
;-----------------------------------------------
 
Name "Java Launcher for BEViewer"
Caption "Java Launcher for BEViewer"
;Icon "Java Launcher.ico"
OutFile "BEViewerLauncher.exe"
 
; use javaw.exe to avoid dosbox or use java.exe to keep stdout/stderr
!define JAVAEXE "javaw.exe"
!define JAVA_URL "http://www.java.com"
 
RequestExecutionLevel user
SilentInstall silent
AutoCloseWindow true
ShowInstDetails nevershow

!include "FileFunc.nsh"
!insertmacro GetParameters
 
var JAVA_VER
var JAVA_HOME

Section ""
  Call GetJRE
  ${GetParameters} $1

  StrCpy $0 '"$JAVA_HOME\bin\${JAVAEXE}" -Xmx1g -jar BEViewer.jar" $1'
 
  SetOutPath $EXEDIR
  ExecWait $0
SectionEnd
 
; put path to Java 1.6+ into $JAVA_HOME else fail
Function GetJRE
  Push $0
  Push $1

  ;TryJRE64:
  SetRegView 64
  ReadRegStr $JAVA_VER HKLM "SOFTWARE\JavaSoft\Java Runtime Environment" "CurrentVersion"
  StrCmp "" "$JAVA_VER" TryJDK64 0
  ReadRegStr $JAVA_HOME HKLM "SOFTWARE\JavaSoft\Java Runtime Environment\$JAVA_VER" "JavaHome"
  goto CheckJavaVer

  TryJDK64:
  ClearErrors
  ReadRegStr $JAVA_VER HKLM "SOFTWARE\JavaSoft\Java Development Kit" "CurrentVersion"
  StrCmp "" "$JAVA_VER" TryJRE32 0
  ReadRegStr $JAVA_HOME HKLM "SOFTWARE\JavaSoft\Java Development Kit\$JAVA_VER" "JavaHome"
  goto CheckJavaVer

  TryJRE32:
  SetRegView 32
  ReadRegStr $JAVA_VER HKLM "SOFTWARE\JavaSoft\Java Runtime Environment" "CurrentVersion"
  StrCmp "" "$JAVA_VER" TryJDK32 0
  ReadRegStr $JAVA_HOME HKLM "SOFTWARE\JavaSoft\Java Runtime Environment\$JAVA_VER" "JavaHome"
  goto CheckJavaVer

  TryJDK32:
  ClearErrors
  ReadRegStr $JAVA_VER HKLM "SOFTWARE\JavaSoft\Java Development Kit" "CurrentVersion"
  StrCmp "" "$JAVA_VER" JavaBad 0
  ReadRegStr $JAVA_HOME HKLM "SOFTWARE\JavaSoft\Java Development Kit\$JAVA_VER" "JavaHome"

  CheckJavaVer:
  StrCpy $0 $JAVA_VER 1 0
  StrCpy $1 $JAVA_VER 1 2
  StrCpy $JAVA_VER "$0$1"
  IntCmp $JAVA_VER 16 CheckJavaPresent JavaBad CheckJavaPresent

  CheckJavaPresent:
  IfFileExists "$JAVA_HOME\bin\${JAVAEXE}" JavaGood JavaBad

  JavaBad:
  MessageBox MB_ICONSTOP "Unable to find Java 6+.  Please install Java 6 or newer in order to run BEViewer.  Java is available from ${JAVA_URL}.  Aborting."
  Abort
 
  JavaGood:
  Pop $1
  Pop $0
FunctionEnd

