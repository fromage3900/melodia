Get-ChildItem -Recurse "g:\Melodia\Source\MelodiaMelusina_PROD" -Include *.h,*.cpp | ForEach-Object {
    $lines = (Get-Content $_.FullName | Measure-Object -Line).Lines
    Write-Output "$lines $($_.FullName)"
} | Sort-Object
