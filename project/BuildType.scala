/**
 * seld-defined build type
 * @author Yaxing Chen
 * @version 0.0.4
 */

sealed trait BuildType
case object Release extends BuildType
case object Debug extends BuildType
case object Profile extends BuildType
