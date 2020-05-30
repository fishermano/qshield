val buildType = SettingKey[BuildType]("buildType",
  "Release, Debug, or Profile.")

buildType := Release

val nativeBuildTask = TaskKey[File]("nativeBuild", "Build native C++ code, returning the directory containing the resulting shared libraries.")

baseDirectory in nativeBuildTask := (baseDirectory in ThisBuild).value

compile in Compile := { (compile in Compile).dependsOn(nativeBuildTask).value }

val copyNativeLibrariesToResourcesTask = TaskKey[Seq[File]]("copyNativeLibrariesToResources",
  "Copies the native libraries to the managed resources directory, returning the copied files.")

resourceGenerators in Compile += copyNativeLibrariesToResourcesTask.taskValue

managedResourceDirectories in Compile += resourceManaged.value

nativePlatform := {
  try {
    val lines = Process("uname -sm").lines
    if (lines.length == 0) {
      sys.error("Error occured trying to run `uname`")
    }
    // uname -sm returns "<kernel> <hardware name>"
    val parts = lines.head.split(" ")
    if (parts.length != 2) {
      sys.error("'uname -sm' returned unexpected string: " + lines.head)
    } else {
      val arch = parts(1).toLowerCase.replaceAll("\\s", "")
      val kernel = parts(0).toLowerCase.replaceAll("\\s", "")
      arch + "-" + kernel
    }
  } catch {
    case ex: Exception =>
      sLog.value.error("Error trying to determine platform.")
      sLog.value.warn("Cannot determine platform! It will be set to 'unknown'.")
      "unknown-unknown"
  }
}

nativeBuildTask :={
  import sys.process._
  val nativeSourceDir = baseDirectory.value / "src" / "native"
  val nativeBuildDir = target.value / "native"
  nativeBuildDir.mkdirs()
  val cmakeResult =
    Process(Seq(
      "cmake",
      s"-DCMAKE_INSTALL_PREFIX:PATH=${nativeBuildDir.getPath}",
      s"-DCMAKE_BUILD_TYPE=${buildType.value}",
      nativeSourceDir.getPath), nativeBuildDir).!
  if (cmakeResult != 0) sys.error("native build failed.")
  val nproc = java.lang.Runtime.getRuntime.availableProcessors
  val buildResult = Process(Seq("make", "-j" + nproc), nativeBuildDir).!
  if (buildResult != 0) sys.error("C++ build failed.")
  val installResult = Process(Seq("make", "install"), nativeBuildDir).!
  if (installResult != 0) sys.error("C++ build failed.")
  nativeBuildDir / "lib"
}

copyNativeLibrariesToResourcesTask :={
  val libraries = (nativeBuildTask.value ** "*.so").get
  val mappings: Seq[(File, String)] =
    libraries pair rebase(nativeBuildTask.value , s"/native/${nativePlatform.value}")
  val resources: Seq[File] = for ((file, path) <- mappings) yield {
    val resource = resourceManaged.value / path
    IO.copyFile(file, resource)
    resource
  }
  resources
}
