/**
 * global setting keys
 * @author Yaxing Chen
 * @version 0.0.4
 */

import sbt._

object GlobalSettings extends Build {
  val flatbuffersVersion = settingKey[String]("version of flatbuffer used in this build") // flatbuffers version
}
