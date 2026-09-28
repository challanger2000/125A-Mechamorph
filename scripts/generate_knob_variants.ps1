param(
    [string]$Source = "resource/mechamorph-controls/mechamorph-knob-grip-64.png",
    [string]$FaceplateSource = "resource/mechamorph-faceplate-v2.png",
    [string]$FaceplateOutput = "resource/mechamorph-faceplate-runtime.png",
    [string]$OutputDir = "resource/mechamorph-controls"
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$frames = 64
$sourceFrame = 125
$fillRatio = 0.94

# Normalize the accepted source faceplate to the plugin's fixed 1440x960 logical canvas.
$face = [System.Drawing.Bitmap]::FromFile((Resolve-Path $FaceplateSource))
try {
    if (($face.Width * 2) -ne ($face.Height * 3)) {
        throw "Faceplate source must have 3:2 aspect ratio, got $($face.Width)x$($face.Height)"
    }
    if ($face.Width -lt 1440 -or $face.Height -lt 960) {
        throw "Faceplate source is too small: $($face.Width)x$($face.Height)"
    }
    $faceOut = New-Object System.Drawing.Bitmap(1440, 960, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $g = [System.Drawing.Graphics]::FromImage($faceOut)
        try {
            $g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
            $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
            $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
            $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
            $g.DrawImage($face, (New-Object System.Drawing.Rectangle(0,0,1440,960)))
        } finally { $g.Dispose() }
        $faceOut.Save($FaceplateOutput, [System.Drawing.Imaging.ImageFormat]::Png)
        Write-Host "Faceplate runtime: 1440 x 960"
    } finally { $faceOut.Dispose() }
} finally { $face.Dispose() }

$variants = @(
    @{ Name = "knob-machine-64.png"; Size = 256 },
    @{ Name = "knob-main-64.png";    Size = 168 },
    @{ Name = "knob-scale-64.png";   Size = 198 },
    @{ Name = "knob-utility-64.png"; Size = 90 }
)

$src = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Source))
try {
    if ($src.Width -ne $sourceFrame -or $src.Height -ne ($sourceFrame * $frames)) {
        throw "Unexpected master filmstrip dimensions: $($src.Width)x$($src.Height); expected 125x8000"
    }

    # Find the actual non-transparent knob bounds once. Use one symmetric crop for every frame,
    # so the rotation centre can never move between frames.
    $minX=$sourceFrame; $minY=$sourceFrame; $maxX=-1; $maxY=-1
    for ($y=0; $y -lt $sourceFrame; $y++) {
        for ($x=0; $x -lt $sourceFrame; $x++) {
            if ($src.GetPixel($x,$y).A -gt 8) {
                if ($x -lt $minX) { $minX=$x }
                if ($y -lt $minY) { $minY=$y }
                if ($x -gt $maxX) { $maxX=$x }
                if ($y -gt $maxY) { $maxY=$y }
            }
        }
    }
    if ($maxX -lt 0) { throw "Master knob first frame has no visible alpha pixels" }

    $center = ($sourceFrame - 1) / 2.0
    $half = [Math]::Ceiling([Math]::Max(
        [Math]::Max($center-$minX, $maxX-$center),
        [Math]::Max($center-$minY, $maxY-$center)
    ))
    $cropSize = [int][Math]::Min($sourceFrame, (2*$half)+1)
    $cropX = [int][Math]::Floor(($sourceFrame-$cropSize)/2)
    $cropY = $cropX
    Write-Host "Master alpha bounds: x=$minX..$maxX y=$minY..$maxY; symmetric crop=$cropX,$cropY $cropSize x $cropSize"

    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
    foreach ($variant in $variants) {
        $size = [int]$variant.Size
        $outPath = Join-Path $OutputDir $variant.Name
        $drawSize = [int][Math]::Round($size*$fillRatio)
        $margin = [int][Math]::Floor(($size-$drawSize)/2)
        $dst = New-Object System.Drawing.Bitmap($size, ($size * $frames), [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $g = [System.Drawing.Graphics]::FromImage($dst)
            try {
                $g.Clear([System.Drawing.Color]::Transparent)
                $g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceOver
                $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
                $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                for ($i = 0; $i -lt $frames; $i++) {
                    $srcRect = New-Object System.Drawing.Rectangle($cropX, ($i * $sourceFrame + $cropY), $cropSize, $cropSize)
                    $dstRect = New-Object System.Drawing.Rectangle($margin, ($i * $size + $margin), $drawSize, $drawSize)
                    $g.DrawImage($src, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
                }
            } finally { $g.Dispose() }
            $dst.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
            Write-Host "$($variant.Name): $size x $($size*$frames), visible fill=$([Math]::Round($fillRatio*100))%"
        } finally { $dst.Dispose() }
    }
} finally { $src.Dispose() }
