@echo off
setlocal enabledelayedexpansion

set "file=C:\Users\Nico\source\repos\Atlasbruce\Olympe-Engine\OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"

for /f "delims=" %%a in ('type "%file%"') do (
    echo %%a
    if "%%a"=="    <ClCompile Include=""..\Source\Editor\Modals\SubGraphFilePickerModal.cpp"" />" (
        echo     ^<ClCompile Include=""..\Source\Editor\Modals\BehaviorTreeFilePickerModal.cpp"" /^>
    )
) > "%file%.tmp"

move /Y "%file%.tmp" "%file%"
