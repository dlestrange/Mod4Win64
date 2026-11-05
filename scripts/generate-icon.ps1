param(
    [Parameter(Mandatory=$true)][string]$Output
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

# No third-party art or fonts: draw a sharp retro 64, green on a dark LCD.
# Each ICO layer is generated independently to remain legible at 16px.
$glyph6 = @('01110','10000','10000','11110','10001','10001','01110')
$glyph4 = @('10010','10010','10010','11111','00010','00010','00010')
$sizes = @(16,24,32,48,64,128,256)
$entries = New-Object 'System.Collections.Generic.List[byte[]]'
foreach ($size in $sizes) {
    $bmp = [System.Drawing.Bitmap]::new($size,$size,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    try {
        $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
        $g.Clear([System.Drawing.Color]::FromArgb(255,192,192,192))
        $dark = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255,3,12,6))
        $border = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255,27,110,45))
        $lit = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255,72,245,94))
        $dim = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255,15,61,24))
        try {
            $edge = [Math]::Max(1,[int][Math]::Round($size*0.06))
            $g.FillRectangle($border,0,0,$size,$size)
            $g.FillRectangle($dark,$edge,$edge,$size-2*$edge,$size-2*$edge)
            # A familiar grid of unlit green LCD dots behind the digits.
            if($size -ge 32) {
                $step = [Math]::Max(3,[int][Math]::Floor($size/18))
                for($y=$edge+2;$y -lt $size-$edge-1;$y+=$step) {
                    for($x=$edge+2;$x -lt $size-$edge-1;$x+=$step) {
                        $g.FillRectangle($dim,$x,$y,1,1)
                    }
                }
            }
            # Eleven columns (5+1+5), seven rows: luminous pixel-font 64.
            $cell = [Math]::Max(1,[int][Math]::Floor(($size-2*$edge-3)/12))
            $width = 11*$cell
            $height = 7*$cell
            $startX = [int][Math]::Floor(($size-$width)/2)
            $startY = [int][Math]::Floor(($size-$height)/2)
            for($row=0;$row -lt 7;$row++) {
                for($col=0;$col -lt 11;$col++) {
                    if($col -eq 5) { continue }
                    $charCol = if($col -gt 5) {$col-6} else {$col}
                    $bits = if($col -gt 5) {$glyph4[$row]} else {$glyph6[$row]}
                    if($bits[$charCol] -eq '1') {
                        $x=$startX+$col*$cell
                        $y=$startY+$row*$cell
                        $g.FillRectangle($lit,$x,$y,[Math]::Max(1,$cell-1),[Math]::Max(1,$cell-1))
                    }
                }
            }
        }
        finally { $dark.Dispose();$border.Dispose();$lit.Dispose();$dim.Dispose() }
        $ms = [System.IO.MemoryStream]::new()
        try {
            $bmp.Save($ms,[System.Drawing.Imaging.ImageFormat]::Png)
            $entries.Add($ms.ToArray())
        } finally { $ms.Dispose() }
    } finally { $g.Dispose();$bmp.Dispose() }
}

# ICO container of PNG-compressed entries (Vista+ / Windows 11 compatible).
$parent = [System.IO.Path]::GetDirectoryName($Output)
if($parent -and -not (Test-Path $parent)) {
    New-Item -ItemType Directory -Path $parent -Force | Out-Null
}
$fs=[System.IO.File]::Create($Output)
$writer=[System.IO.BinaryWriter]::new($fs)
try {
    $writer.Write([uint16]0)   # reserved
    $writer.Write([uint16]1)   # icon
    $writer.Write([uint16]$sizes.Count)
    $offset=6+16*$sizes.Count
    for($i=0;$i -lt $sizes.Count;$i++) {
        $n=$sizes[$i]
        $writer.Write([byte]$(if($n -eq 256){0}else{$n}))
        $writer.Write([byte]$(if($n -eq 256){0}else{$n}))
        $writer.Write([byte]0) # palette
        $writer.Write([byte]0)
        $writer.Write([uint16]1)
        $writer.Write([uint16]32)
        $writer.Write([uint32]$entries[$i].Length)
        $writer.Write([uint32]$offset)
        $offset+=$entries[$i].Length
    }
    foreach($png in $entries) { $writer.Write($png) }
} finally { $writer.Dispose();$fs.Dispose() }
Write-Host "Generated icon: $Output"
