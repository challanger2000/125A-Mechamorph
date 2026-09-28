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
    @{ Name = "knob-machine-64.png"; Size = 250 },
    @{ Name = "knob-main-64.png";    Size = 125 },
    @{ Name = "knob-scale-64.png";   Size = 220 },
    @{ Name = "knob-utility-64.png"; Size = 104 }
)

$src = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Source))
try {
    if ($src.Width -ne $sourceFrame -or $src.Height -ne ($sourceFrame * $frames)) {
        throw "Unexpected master filmstrip dimensions: $($src.Width)x$($src.Height); expected 125x8000"
    }
    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
    foreach ($variant in $variants) {
        $size = [int]$variant.Size
        $outPath = Join-Path $OutputDir $variant.Name
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
                    $srcRect = New-Object System.Drawing.Rectangle(0, ($i * $sourceFrame), $sourceFrame, $sourceFrame)
                    $dstRect = New-Object System.Drawing.Rectangle(0, ($i * $size), $size, $size)
                    $g.DrawImage($src, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
                }
            } finally { $g.Dispose() }
            $dst.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
            Write-Host "$($variant.Name): $size x $($size * $frames)"
        } finally { $dst.Dispose() }
    }
} finally { $src.Dispose() }
