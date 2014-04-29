import sys
from path import path

try:
  path(sys.argv[1]).makedirs()
except:
  import traceback
  traceback.print_exc()
sys.exit(0)
