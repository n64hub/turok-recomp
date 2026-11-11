# tools/icon.ps1 -OutDir icons: the launcher's thumbnail (512.png) and the program's icon (app.ico, 16 to 256 px as PNG
# entries): "TUROK 2 Recompiled" on a rounded square, drawn from plain text and shapes, nothing from the game.
param([string]$OutDir)
Add-Type -AssemblyName System.Drawing
function Draw([int]$n) {
  $bmp = New-Object System.Drawing.Bitmap $n, $n
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.SmoothingMode = 'AntiAlias'; $g.TextRenderingHint = 'AntiAliasGridFit'
  $g.Clear([System.Drawing.Color]::Transparent)
  $r = $n * 0.14; $path = New-Object System.Drawing.Drawing2D.GraphicsPath
  $path.AddArc(0, 0, 2*$r, 2*$r, 180, 90); $path.AddArc($n-2*$r-1, 0, 2*$r, 2*$r, 270, 90)
  $path.AddArc($n-2*$r-1, $n-2*$r-1, 2*$r, 2*$r, 0, 90); $path.AddArc(0, $n-2*$r-1, 2*$r, 2*$r, 90, 90); $path.CloseFigure()
  $bg = New-Object System.Drawing.Drawing2D.LinearGradientBrush (New-Object System.Drawing.Point 0,0), (New-Object System.Drawing.Point 0,$n), ([System.Drawing.Color]::FromArgb(255,38,64,30)), ([System.Drawing.Color]::FromArgb(255,20,24,14))
  $g.FillPath($bg, $path)
  $sf = New-Object System.Drawing.StringFormat; $sf.Alignment = 'Center'; $sf.LineAlignment = 'Center'
  $gold = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255,232,150,40))
  $white = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::White)
  $pale = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255,200,220,180))
  if ($n -ge 48) {
    $f1 = New-Object System.Drawing.Font 'Georgia', ($n*0.15), ([System.Drawing.FontStyle]::Bold), ([System.Drawing.GraphicsUnit]::Pixel)
    $f2 = New-Object System.Drawing.Font 'Georgia', ($n*0.30), ([System.Drawing.FontStyle]::Bold), ([System.Drawing.GraphicsUnit]::Pixel)
    $f3 = New-Object System.Drawing.Font 'Georgia', ($n*0.075), ([System.Drawing.FontStyle]::Regular), ([System.Drawing.GraphicsUnit]::Pixel)
    $g.DrawString('TUROK', $f1, $gold, (New-Object System.Drawing.RectangleF 0, ($n*0.17), $n, ($n*0.2)), $sf)
    $g.DrawString('2', $f2, $white, (New-Object System.Drawing.RectangleF 0, ($n*0.38), $n, ($n*0.32)), $sf)
    $g.DrawString('Recompiled', $f3, $pale, (New-Object System.Drawing.RectangleF 0, ($n*0.72), $n, ($n*0.12)), $sf)
  } else {
    $f = New-Object System.Drawing.Font 'Georgia', ($n*0.62), ([System.Drawing.FontStyle]::Bold), ([System.Drawing.GraphicsUnit]::Pixel)
    $g.DrawString('T2', $f, $gold, (New-Object System.Drawing.RectangleF (-$n*0.1), 0, ($n*1.2), $n), $sf)
  }
  $g.Dispose(); return $bmp
}
$b = Draw 512; $b.Save("$OutDir\512.png", [System.Drawing.Imaging.ImageFormat]::Png); $b.Dispose()
$sizes = 16, 24, 32, 48, 64, 128, 256; $pngs = @()
foreach ($s in $sizes) { $m = New-Object System.IO.MemoryStream; $b = Draw $s; $b.Save($m, [System.Drawing.Imaging.ImageFormat]::Png); $b.Dispose(); $pngs += ,$m.ToArray() }
$out = New-Object System.IO.MemoryStream; $w = New-Object System.IO.BinaryWriter $out
$w.Write([uint16]0); $w.Write([uint16]1); $w.Write([uint16]$sizes.Count)
$off = 6 + 16 * $sizes.Count
for ($i = 0; $i -lt $sizes.Count; $i++) {
  $s = $sizes[$i]; $d = if ($s -ge 256) { 0 } else { $s }
  $w.Write([byte]$d); $w.Write([byte]$d); $w.Write([byte]0); $w.Write([byte]0); $w.Write([uint16]1); $w.Write([uint16]32)
  $w.Write([uint32]$pngs[$i].Length); $w.Write([uint32]$off); $off += $pngs[$i].Length
}
foreach ($p in $pngs) { $w.Write($p) }
$w.Flush(); [System.IO.File]::WriteAllBytes("$OutDir\app.ico", $out.ToArray())
