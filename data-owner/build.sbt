sparkVersion := "2.4.5"

sparkComponents ++= Seq("core", "sql", "catalyst")

val flatbuffersVersion = "1.7.0"

val flatbuffersGenJavaDir = SettingKey[File]("flatbuffersGenJavaDir",
  "Location of Flatbuffers generated Java files.")

flatbuffersGenJavaDir := sourceManaged.value / "flatbuffers" / "gen-java"

val fetchFlatbuffersLibTask = TaskKey[File]("fetchFlatbuffersLib",
  "Fetches and builds the Flatbuffers library, returning its location.")

unmanagedSources in Compile ++= ((fetchFlatbuffersLibTask.value / "java") ** "*.java").get

val buildFlatbuffersTask = TaskKey[Seq[File]]("buildFlatbuffers",
  "Generates Java sources from Flatbuffers interface files, returning the Java sources.")

sourceGenerators in Compile += buildFlatbuffersTask.taskValue

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

fetchFlatbuffersLibTask := {
  val flatbuffersSource = target.value / "flatbuffers" / s"flatbuffers-$flatbuffersVersion"
  if (!flatbuffersSource.exists) {
    // Fetch flatbuffers from local resource
    streams.value.log.info(s"Fetching Flatbuffers")
    val flatbuffersLoc = baseDirectory.value / "src" / "deps" / s"flatbuffers-$flatbuffersVersion.zip"
    if (!flatbuffersLoc.exists) {
      // Fetch flatbuffers from Github
      val flatbuffersUrl = new java.net.URL(
        s"https://github.com/google/flatbuffers/archive/v$flatbuffersVersion.zip")
      IO.unzipURL(flatbuffersUrl, flatbuffersSource.getParentFile)
    }else{
      IO.unzip(flatbuffersLoc, flatbuffersSource.getParentFile)
    }
  }
  val flatc = flatbuffersSource / "flatc"
  if (!flatc.exists) {
    // Build flatbuffers with cmake
    import sys.process._
    streams.value.log.info(s"Building Flatbuffers")
    val nproc = java.lang.Runtime.getRuntime.availableProcessors
    if (Process(Seq(
      "cmake", "-G", "Unix Makefiles",
      "-DFLATBUFFERS_BUILD_TESTS=OFF",
      "-DFLATBUFFERS_BUILD_FLATLIB=OFF",
      "-DFLATBUFFERS_BUILD_FLATHASH=OFF",
      "-DFLATBUFFERS_BUILD_FLATC=ON"), flatbuffersSource).! != 0
      || Process(Seq("make", "-j" + nproc), flatbuffersSource).! != 0) {
      sys.error("Flatbuffers library build failed.")
    }
  }
  flatbuffersSource
}

buildFlatbuffersTask := {
  import sys.process._

  val flatc = fetchFlatbuffersLibTask.value / "flatc"

  val flatbuffers = ((sourceDirectory.value / "flatbuffers") ** "*.fbs").get
  // Only regenerate Flatbuffers headers if any .fbs file changed, indicated by their latest
  // modification time being newer than all generated headers. We do this because regenerating
  // Flatbuffers headers causes a full enclave rebuild, which is slow.
  val fbsLastMod = flatbuffers.map(_.lastModified).max
  val gen = (flatbuffersGenJavaDir.value ** "*.java").get

  if (gen.isEmpty || fbsLastMod > gen.map(_.lastModified).max) {
    for (fbs <- flatbuffers) {
      streams.value.log.info(s"Generating flatbuffers for ${fbs}")
      if (Seq(flatc.getPath, "--java", "-o", flatbuffersGenJavaDir.value.getPath, fbs.getPath).! != 0) {
        sys.error("Flatbuffers build failed.")
      }
    }
  }

  (flatbuffersGenJavaDir.value ** "*.java").get
}

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
  val cryptoLib = ((baseDirectory.value / "tpl") ** "*.so").get
  val cryptoMappings: Seq[(File, String)] =
    cryptoLib pair rebase(baseDirectory.value / "tpl" , s"/native/${nativePlatform.value}")
  val cryptoResources: Seq[File] = for ((file, path) <- cryptoMappings) yield {
    val resource = resourceManaged.value / path
    IO.copyFile(file, resource)
    resource
  }
  resources ++ cryptoResources
}
