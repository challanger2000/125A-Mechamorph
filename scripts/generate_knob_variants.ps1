param(
    [string]$Source = "resource/mechamorph-controls/knob-master-64.png",
    [string]$OutputDir = "resource/mechamorph-controls"
)

$ErrorActionPreference = "Stop"
Add-Type -AssemblyName System.Drawing

$frames = 64
$sourceFrame = 125
$cropX = 17
$cropY = 17
$cropSize = 91
$fillRatio = 0.88

$variants = @(
    @{ Name = "knob-machine-64.png"; Size = 250 },
    @{ Name = "knob-main-64.png";    Size = 125 },
    @{ Name = "knob-scale-64.png";   Size = 220 },
    @{ Name = "knob-utility-64.png"; Size = 104 }
)

$src = [System.Drawing.Bitmap]::FromFile((Resolve-Path $Source))
try {
    if ($src.Width -ne $sourceFrame -or $src.Height -ne ($sourceFrame * $frames)) {
        throw "Unexpected master filmstrip dimensions: $($src.Width)x$($src.Height)"
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
                $drawSize = [int][Math]::Round($size * $fillRatio)
                $margin = [int][Math]::Floor(($size - $drawSize) / 2)
                for ($i = 0; $i -lt $frames; $i++) {
                    $srcRect = New-Object System.Drawing.Rectangle($cropX, ($i * $sourceFrame + $cropY), $cropSize, $cropSize)
                    $dstRect = New-Object System.Drawing.Rectangle($margin, ($i * $size + $margin), $drawSize, $drawSize)
                    $g.DrawImage($src, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
                }
            } finally { $g.Dispose() }
            $dst.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
            Write-Host "$($variant.Name): $size x $($size * $frames), draw=$drawSize"
        } finally { $dst.Dispose() }
    }
} finally { $src.Dispose() }
