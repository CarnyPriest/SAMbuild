copy /V /-Y "PinMAME_VC2008.sln" "PinMAME_VC2010.sln"
copy /V /-Y "InstallVPinMAME_VC2008.vcproj" "InstallVPinMAME_VC2010.vcxproj"
copy /V /-Y "PinMAME_VC2008.vcproj" "PinMAME_VC2010.vcxproj"
copy /V /-Y "PinMAME32_VC2008.vcproj" "PinMAME32_VC2010.vcxproj"
copy /V /-Y "VPinMAME_VC2008.vcproj" "VPinMAME_VC2010.vcxproj"

@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"PinMAME_VC2010.sln" /out:"PinMAME_VC2010.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2010.vcproj" /replace:"VC2010.vcxproj" /in:"PinMAME_VC2010.sln" /out:"PinMAME_VC2010.sln"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"InstallVPinMAME_VC2010.vcxproj" /out:"InstallVPinMAME_VC2010.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"PinMAME_VC2010.vcxproj" /out:"PinMAME_VC2010.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"PinMAME32_VC2010.vcxproj" /out:"PinMAME32_VC2010.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"VC2008" /replace:"VC2010" /in:"VPinMAME_VC2010.vcxproj" /out:"VPinMAME_VC2010.vcxproj"
@if errorlevel 1 goto manual

@cscript "simplereplace.wsf" //nologo /search:"PreprocessorDefinitions=``WIN32;" /replace:"PreprocessorDefinitions=``_CRT_SECURE_NO_WARNINGS;WIN32;" /in:"InstallVPinMAME_VC2010.vcxproj" /out:"InstallVPinMAME_VC2010.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"PreprocessorDefinitions=``WIN32;" /replace:"PreprocessorDefinitions=``_CRT_SECURE_NO_WARNINGS;WIN32;" /in:"PinMAME_VC2010.vcxproj" /out:"PinMAME_VC2010.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"PreprocessorDefinitions=``WIN32;" /replace:"PreprocessorDefinitions=``_CRT_SECURE_NO_WARNINGS;WIN32;" /in:"PinMAME32_VC2010.vcxproj" /out:"PinMAME32_VC2010.vcxproj"
@if errorlevel 1 goto manual
@cscript "simplereplace.wsf" //nologo /search:"PreprocessorDefinitions=``WIN32;" /replace:"PreprocessorDefinitions=``_CRT_SECURE_NO_WARNINGS;WIN32;" /in:"VPinMAME_VC2010.vcxproj" /out:"VPinMAME_VC2010.vcxproj"
@if errorlevel 1 goto manual

@goto end

:manual
@echo.
@echo Replace all occurrences of VC2008 in the project files with VC2010.

:end
@echo Convert the project files with VC2010 and compile.
@pause
