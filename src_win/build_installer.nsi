# NSIS script for creating the Windows bulk_extractor installer file using makensis.
#
# Installs the following:
#   32-bit configuration of bulk_extractor
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
!define COMPANYNAME "NPS"
!define DESCRIPTION "bulk_extractor Feature extractor and BEViewer User Interface"

# These will be displayed by the "Click here for support information" link in "Add/Remove Programs"
# It is possible to use "mailto:" links here to open the email client
!define HELPURL "https://github.com/simsong/bulk_extractor" # "Support Information" link
!define UPDATEURL "https://github.com/simsong/bulk_extractor" # "Product Updates" link
!define ABOUTURL "https://github.com/simsong/bulk_extractor" # "Publisher" link

!ifdef SIGN
	!define BULK_EXTRACTOR_32 "signed_bulk_extractor32.exe"
	!define BULK_EXTRACTOR_64 "signed_bulk_extractor64.exe"
	!define BE_VIEWER_LAUNCHER "signed_BEViewerLauncher.exe"
	!define UNINSTALLER_EXE "signed_uninstall.exe"
!else
	!define BULK_EXTRACTOR_32 "bulk_extractor32.exe"
	!define BULK_EXTRACTOR_64 "bulk_extractor64.exe"
	!define BE_VIEWER_LAUNCHER "BEViewerLauncher.exe"
!endif
!define BE_VIEWER_JAR "BEViewer.jar"

SetCompressor lzma
 
RequestExecutionLevel admin
 
InstallDir "$PROGRAMFILES\${APPNAME}"
 
Name "${APPNAME}"
!ifdef SIGN
	outFile "bulk_extractor-${VERSION}-intermediate.exe"
!else
	outFile "bulk_extractor-${VERSION}-windowsinstaller.exe"
!endif
 
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

function InstallOnce
	# don't install twice
	ifFileExists "$INSTDIR\uninstall.exe" AlreadyThere

	# install BEViewer.jar
	setOutPath "$INSTDIR"
	file ${BE_VIEWER_JAR}

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

	# install the uninstaller
	!ifdef SIGN
		# use the pre-built signed uninstaller
		# Note: the signed uninstaller is created by copying the unsigned uninstaller
		# back in from a Windows installation.
		file "/oname=uninstall.exe" "${UNINSTALLER_EXE}"
	!else
		# create the uninstaller
		writeUninstaller "$INSTDIR\uninstall.exe"
	!endif
 
	# create the start menu for BEViewer
	createDirectory "$SMPROGRAMS\${APPNAME}"

	# link the uninstaller to the start menu
	createShortCut "$SMPROGRAMS\${APPNAME}\Uninstall ${APPNAME}.lnk" "$INSTDIR\uninstall.exe"

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

        # install PDF docs
        setOutPath "$INSTDIR\pdf"
        file "BEProgrammersManual.pdf"
        file "BEUsersManual.pdf"
        file "BEWorkedExamplesStandalone.pdf"

        # 
	createShortCut "$SMPROGRAMS\${APPNAME}\Users Manual.lnk" "$INSTDIR\pdf\BEUsersManual.pdf"
	createShortCut "$SMPROGRAMS\${APPNAME}\Programmers Manual.lnk" "$INSTDIR\pdf\BEProgrammersManual.pdf"
	createShortCut "$SMPROGRAMS\${APPNAME}\Worked Examples.lnk" "$INSTDIR\pdf\BEWorkedExamplesStandalone.pdf"

	AlreadyThere:
functionEnd

Section "32-bit configuration" SEC0000

	# install content common to both
	call InstallOnce

	# install bulk_extractor files into the 32-bit configuration
	setOutPath "$INSTDIR\32-bit"
	file "/oname=bulk_extractor.exe" ${BULK_EXTRACTOR_32}

	# install BEViewerLauncher
	file "/oname=BEViewerLauncher.exe" ${BE_VIEWER_LAUNCHER}
	createShortCut "BEViewer.jar" "..\BEViewer.jar"
 
	# create the shortcut link to the target's start menu
	createShortCut "$SMPROGRAMS\${APPNAME}\BEViewer with ${APPNAME} (32-bit).lnk" "$OUTDIR\BEViewerLauncher.exe"
sectionEnd

Section "64-bit configuration" SEC0001

	# install content common to both
	call InstallOnce

	# install the files into the 64-bit configuration
	setOutPath "$INSTDIR\64-bit"
	file "/oname=bulk_extractor.exe" ${BULK_EXTRACTOR_64}
 
	# install BEViewerLauncher
	file "/oname=BEViewerLauncher.exe" ${BE_VIEWER_LAUNCHER}
	createShortCut "BEViewer.jar" "..\BEViewer.jar"
 
	# create the shortcut link to the target's start menu
	createShortCut "$SMPROGRAMS\${APPNAME}\BEViewer with ${APPNAME} (64-bit).lnk" "$OUTDIR\BEViewerLauncher.exe"
sectionEnd

Section "Add to path" SEC0002
	setOutPath "$INSTDIR"
        # note that path includes 32-bit and 64-bit, whether or not they
        # were both installed
        ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR\python"
        ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR\32-bit"
        ${EnvVarUpdate} $0 "PATH" "A" "HKLM" "$INSTDIR\64-bit"
sectionEnd

function .onInit
        #Determine the bitness of the OS and enable the correct section
        ${If} ${RunningX64}
            SectionSetFlags ${SEC0000}  0
            SectionSetFlags ${SEC0001}  ${SF_SELECTED}
        ${Else}
            SectionSetFlags ${SEC0001}  0
            SectionSetFlags ${SEC0000}  ${SF_SELECTED}
        ${EndIf}

        # require admin
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
functionEnd

!ifndef SIGN
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
	StrCpy $0 "$INSTDIR\32-bit\BEViewerLauncher.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\64-bit\BEViewerLauncher.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\BEViewer.jar"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\32-bit\bulk_extractor.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\64-bit\bulk_extractor.exe"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\pdf\BEProgrammersManual.pdf"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\pdf\BEUsersManual.pdf"
	Call un.FailableDelete
	StrCpy $0 "$INSTDIR\pdf\BEWorkedExamplesStandalone.pdf"
	Call un.FailableDelete

	# uninstall files and links
	delete "$INSTDIR\32-bit\*"
	delete "$INSTDIR\64-bit\*"
	delete "$INSTDIR\python\*"
	delete "$INSTDIR\pdf\*"

	# uninstall dir
	rmdir "$INSTDIR\32-bit"
	rmdir "$INSTDIR\64-bit"
	rmdir "$INSTDIR\python"
	rmdir "$INSTDIR\pdf"

	# uninstall Start Menu launcher shortcuts
	delete "$SMPROGRAMS\${APPNAME}\BEViewer with ${APPNAME} (32-bit).lnk"
	delete "$SMPROGRAMS\${APPNAME}\BEViewer with ${APPNAME} (64-bit).lnk"
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
        ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR\python"
        ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR\32-bit"
        ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM" "$INSTDIR\64-bit"
sectionEnd
!endif

