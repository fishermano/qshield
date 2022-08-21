/**
 * sbt plugins for qshield project
 * @author Yaxing Chen
 * @version 0.0.4
 */

/**
 * Scalastyle plugin
 */
addSbtPlugin("org.scalastyle" % "scalastyle-sbt-plugin" % "1.0.0")

/**
 * sbt-spark plugin
 */
resolvers += "Spark Packages repo" at "https://dl.bintray.com/spark-packages/maven/"
addSbtPlugin("org.spark-packages" % "sbt-spark-package" % "0.2.6")

/**
 * sbt-jni (java native interface) plugin
 */
addSbtPlugin("ch.jodersky" % "sbt-jni" % "1.2.6")
