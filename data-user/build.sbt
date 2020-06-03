libraryDependencies +=  "org.scalaj" %% "scalaj-http" % "2.4.2"

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
