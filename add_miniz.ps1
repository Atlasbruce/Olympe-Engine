# Script to add miniz.c to vcxproj
$vcxprojPath = "OlympeBlueprintEditor\OlympeBlueprintEditor.vcxproj"
$content = Get-Content $vcxprojPath -Raw

# Find the line with tinyxml2.cpp and add miniz after
$searchPattern = '(\s+<ClCompile Include="\.\.\\Source\\TiledLevelLoader\\third_party\\tinyxml2\\tinyxml2\.cpp" />)'

# Create replacement with miniz
$tiledFiles = @'
    <ClCompile Include="..\Source\TiledLevelLoader\third_party\tinyxml2\tinyxml2.cpp" />
    <ClCompile Include="..\Source\TiledLevelLoader\third_party\miniz\miniz.c" />
'@

# Replace
$newContent = $content -replace $searchPattern, $tiledFiles

# Save
Set-Content $vcxprojPath -Value $newContent -NoNewline -Encoding UTF8

Write-Host "miniz.c added successfully"
