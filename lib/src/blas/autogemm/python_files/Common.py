################################################################################
# Auto-Gemm
################################################################################

outputPath = ""

def setOutputPath(path):
  global outputPath
  outputPath = path + "/"

def getOutputPath():
  global outputPath
  return outputPath

def getRelativeKernelSourcePath():
  return "AutoGemmKernelSources/"

def getRelativeIncludePath():
  return "AutoGemmIncludes/"

def getKernelSourcePath():
  return getOutputPath() + getRelativeKernelSourcePath()

def getIncludePath():
  return getOutputPath() + getRelativeIncludePath()

def getAutoGemmHeader():
  return (
      "/*******************************************************************************\n"
      " * This file was auto-generated using the AutoGemm.py python script.\n"
      " * DO NOT modify this file! Instead, make changes to scripts in\n"
      " *   hcblas/lib/src/blas/autoGemm/ then re-generate files\n"
      " *   (otherwise local changes will be lost after re-generation).\n"
      " ******************************************************************************/\n\n"
      )

hostDataChar = { "s":"s", "d":"d", "c":"c", "z":"z" }
hostDataType = { "s":"float", "d":"double", "c":"float2", "z":"double2" }
openclDataType = { "s":"float", "d":"double", "c":"float2", "z":"double2" }

precisionInt = { "s":0, "d":1, "c":2, "z":3 }
orderInt = { "RowMajor":0, "ColMajor":1 }
transposeInt = { "N":0, "T":1, "C":2 }

