import os
import subprocess
from path import path # pip install path.py

exe7z = path("C:\\") / "Program Files" / "7-zip" / "7z.exe"
signtool = path("C:\\") / "Program Files" / "Microsoft SDKs" / "Windows" / "v7.1" / "Bin" / "signtool.exe"
signopts = ("sign", "/a", "/tr", "http://www.startssl.com/timestamp")
app7z = path("app.7z")

def presign():
  for p in path(".").dirs("FastPreview*"):
    for e in ("*.msi", "*.exe", "*.dll"):
      for f in p.files(e):
        yield f

subprocess.check_call((signtool,) + signopts + tuple(presign()))
          
for p in path(".").dirs("FastPreview*"):
  print p 
  d = p.parent / p.namebase + ".exe"
  print d
  with open(d, "wb") as op:
    with open("7zSD.sfx", "rb") as sp:
      op.write(sp.read())
    op.write(u""";!@Install@!UTF-8!
Title="FastPreview Installer"
ExecuteFile="msiexec.exe"
ExecuteParameters="/i setup.msi"
;!@InstallEnd@!""".encode("utf-8"))
    cmd = list((exe7z,))
    cmd += "a ../{} -t7z -mx -m0=BCJ2 -m1=LZMA:d26 -m2=LZMA:d19 -m3=LZMA:d19 -mb0:1 -mb0s1:2 -mb0s2:3 -x!setup.exe *".format(app7z).split(" ")

    try:
      app7z.unlink()
    except:
      pass

    old = os.getcwd()
    with p:
      subprocess.check_call(cmd)
 
    with open(app7z, "rb") as ap:
      op.write(ap.read())

    try:
      app7z.unlink()
    except:
      pass
subprocess.check_call((signtool,) + signopts + tuple(p.parent / p.namebase + ".exe" for p in path(".").dirs("FastPreview*")))
