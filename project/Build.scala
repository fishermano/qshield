import sbt._

object GlobalSettings extends Build {
  val flatbuffersVersion = settingKey[String]("version of flatbuffer used in this build")
}
