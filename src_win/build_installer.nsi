# NSIS script for creating the Windows bulk_extractor installer file using makensis.
#
# Installs the following:
#   64-bit configuration of bulk_extractor
#   BEViewer Launcher
#   BEViewer
#   uninstaller
#   start menu shortcuts
#   uninstaller shurtcut
#   registry information including uninstaller information

# Assign VERSION externally with -DVERSION=<ver>
# Build from signed files with -DSIGN
!ifndef VERSION
	!echo "VERSION is required."
	!echo "example usage: makensis -DVERSION=1.3 build_installer.nsi"
	!error "Invalid usage"
!endif

!define APPNAME "Bulk Extractor ${VERSION}"
!define REG_SUB_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"
!define COMPANYNAME "Naval Postgraduate School"
!define DESCRIPTION "bulk_extractor Feature extractor and BEViewer User Interface"

# These will be displayed by the "Click here for support information" link in "Add/Remove Programs"
# It is possible to use "mailto:" links here to open the email client
!define HELPURL "https://github.com/simsong/bulk_extractor" # "Support Information" link
!define UPDATEURL "https://github.com/simsong/bulk_extractor" # "Product Updates" link
!define ABOUTURL "https://github.com/simsong/bulk_extractor" # "Publisher" link

#zz!define BULK_EXTRACTOR_64 "bulk_extractor.exe"
#zz!define BE_VIEWER_LAUNCHER "BEViewerLauncher.exe"

SetCompressor lzma
 
RequestExecutionLevel admin
 
InstallDir "$PROGRAMFILES64\${APPNAME}"
 
Name "${APPNAME}"
outFile "bulk_extractor-${VERSION}-windowsinstaller.exe"
 
!include LogicLib.nsh
!include EnvVarUpdate.nsi
!include x64.nsh
 
page components
Page instfiles
UninstPage instfiles
 
!macro VerifyUserIsAdmin
UserInfo::GetAccountType
pop $0
${If} $0 != "admin" ;Require admin rights on NT4+
	messageBox mb_iconstop "Administrator rights required!"
	setErrorLevel 740 ;ERROR_ELEVATION_REQUIRED
	quit
${EndIf}
!macroend

Section "${APPNAME}"

	# establish out path
	setOutPath "$INSTDIR"

	# install Registry information
	WriteRegStr HKLM "${REG_SUB_KEY}" "DisplayName" "${APPNAME}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "UninstallString" "$INSTDIR\uninstall.exe"
	WriteRegStr HKLM "${REG_SUB_KEY}" "QuietUninstallString" "$INSTDIR\uninstall.exe /S"
	WriteRegStr HKLM "${REG_SUB_KEY}" "InstallLocation" "$INSTDIR"
	WriteRegStr HKLM "${REG_SUB_KEY}" "Publisher" "${COMPANYNAME}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "HelpLink" "${HELPURL}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "URLUpdateInfo" "${UPDATEURL}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "URLInfoAbout" "${ABOUTURL}"
	WriteRegStr HKLM "${REG_SUB_KEY}" "DisplayVersion" "${VERSION}"
	# There is no option for modifying or repairing the install
	WriteRegDWORD HKLM "${REG_SUB_KEY}" "NoModify" 1
	WriteRegDWORD HKLM "${REG_SUB_KEY}" "NoRepair" 1

	# install bulk_extractor
	file "bulk_extractor.exe"

	# install BEViewer.jar
	file "BEViewer.jar"

	# install the BEViewer launcher
	file "BEViewerLauncher.exe"

	# create the start menu for bulk_extractor
	createDirectory "$SMPROGRAMS\${APPNAME}"

	# create the shortcut link to the target's start menu
	createShortCut "$SMPROGRAMS\${APPNAME}\BEViewer with ${APPNAME}.lnk" "$OUTDIR\BEViewerLauncher.exe"

        # install PDF docs
        setOutPath "$INSTDIR\pdf"
        file "BEProgrammersManual.pdf"
        file "BEUsersManual.pdf"
        file "BEWorkedExamplesStandalone.pdf"

        # install shortcuts to PDF docs
	createShortCut "$SMPROGRAMS\${APPNAME}\Users Manual.lnk" "$INSTDIR\pdf\BEUsersManual.pdf"
	createShortCut "$SMPROGRAMS\${APPNAME}\Programmers Manual.lnk" "$INSTDIR\pdf\BEProgrammersManual.pdf"
	createShortCut "$SMPROGRAMS\${APPNAME}\Worked Examples.lnk" "$INSTDIR\pdf\BEWorkedExamplesStandalone.pdf"

        # install Python scripts
        setOutPath "$INSTDIR\python"
        file "../python/bulk_diff.py"
        file "../python/bulk_extractor_reader.py"
        file "../python/cda_tool.py"
        file "../python/dfxml.py"
        file "../python/fiwalk.py"
        file "../python/identify_filenames.py"
        file "../python/post_process_exif.py"
        file "../python/report_encodings.py"
        file "../python/statbag.py"
        file "../python/ttable.py"

	# create and install the uninstaller
	writeUninstaller "$INSTDIR\uninstall.exe"
 
	# link the uninstaller to the start menu
	createShortCut "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk" "$INSTDIR\uninstall.exe"
sectionEnd

Section "Add to path"
	setOutPath "$INSTDIR"
        ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR"
        ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR\python"
sectionEnd

function .onInit
        # only win64 is supported
        ${If} ${RunningX64}
            # good
        ${Else}
	    MessageBox MB_ICONSTOP \
		"Error: ${APPNAME} is not supported on 32-bit Windows systems.  Please use Win-64."
	    Abort
        ${EndIf}

        # require admin
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
functionEnd

function un.onInit
	SetShellVarContext all
 
	#Verify the uninstaller - last chance to back out
	MessageBox MB_OKCANCEL "Permanantly remove ${APPNAME}?" IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
functionEnd
 
Function un.FailableDelete
	Start:
	delete "$0"
	IfFileExists "$0" FileStillPresent Continue

	FileStillPresent:
	DetailPrint "Unable to delete file $0, likely because it is in use.  Please close all Bulk Extractor files and try again."
	MessageBox MB_ICONQUESTION|MB_RETRYCANCEL \
		"Unable to delete file $0, \
		likely because it is in use.  \
		Please close all Bulk Extractor files and try again." \
 		/SD IDABORT IDRETRY Start IDABORT InstDirAbort

	# abort
	InstDirAbort:
	DetailPrint "Uninstall started but did not complete."
	Abort

	# continue
	Continue:
FunctionEnd

section "uninstall"
	# manage uninstalling these because they may be open
	StrCpy $0 "$INSTDIR\BEViewerLauncher.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\BEViewer.jar"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\bulk_extractor.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\pdf\BEProgrammersManual.pdf"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\pdf\BEUsersManual.pdf"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\pdf\BEWorkedExamplesStandalone.pdf"
	Call un.FailableDelete

	# uninstall files and links
	delete "$INSTDIR\python\*"
	delete "$INSTDIR\pdf\*"

	# uninstall dir
	rmdir "$INSTDIR\python"
	rmdir "$INSTDIR\pdf"

	# uninstall Start Menu launcher shortcuts
	delete "$SMPROGRAMS\${APPNAME}\BEViewer with ${APPNAME}.lnk"
	delete "$SMPROGRAMS\${APPNAME}\uninstall ${APPNAME}.lnk"
	delete "$SMPROGRAMS\${APPNAME}\Users Manual.lnk"
	delete "$SMPROGRAMS\${APPNAME}\Programmers Manual.lnk"
	delete "$SMPROGRAMS\${APPNAME}\Worked Examples.lnk"
	rmDir "$SMPROGRAMS\${APPNAME}"

	# delete the uninstaller
	delete "$INSTDIR\uninstall.exe"
 
	# Try to remove the install directory
	rmDir "$INSTDIR"
 
	# Remove uninstaller information from the registry
	DeleteRegKey HKLM "${REG_SUB_KEY}"

        # remove associated search paths from the PATH environment variable
        # were both installed
        ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR"
        ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR\python"
sectionEnd

