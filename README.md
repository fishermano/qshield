# QShield

#### INTRODUCTION
This project implements a prototype for QShield, a secure query system introduced by the TPDS paper entitled "QShield: Protecting Outsourced Cloud Data Queries with Multi-user Access Control Based on SGX".

**Abstract:** Due to the concern on cloud security, digital encryption is applied before outsourcing data to the cloud for utilization. This introduces a challenge about how to efficiently perform queries over ciphertexts. Crypto-based solutions currently suffer limited operation support, high computational complexity, weak generality, and poor verifiability. An alternative method that utilizes hardware-assisted Trusted Execution Environment (TEE), i.e., Intel SGX, has emerged to offer high computational efficiency, generality, and flexibility. However, SGX based solutions lack support on multi-user query control and suffer from security compromises caused by untrustworthy TEE function invocation, e.g., key revocation failure, incorrect query results, and sensitive information leakage. In this paper, we leverage SGX and propose a secure and efficient SQL-style query framework named QShield. Notably, we propose a novel lightweight secret sharing scheme in QShield to enable multi-user query control; it effectively circumvents key revocation and avoids cumbersome remote attestation for authentication. We further embed a trust-proof mechanism into QShield to guarantee the trustworthiness of TEE function invocation; it ensures the correctness of query results and alleviates side-channel attacks. Through formal security analysis, proof-of-concept implementation, and performance evaluation, we show that QShield can securely query over outsourced data with high efficiency and scalable multi-user support.

#### SETUP
The following steps show how to build a development environment for QShield.

**1.** Creating a hadoop user account
```
~$ sudo adduser hadoop
~$ sudo passwd hadoop
~$ sudo adduser hadoop sudo
```
**2.** Updating apt repository
```
~$ sudo nano /etc/apt/sources.list

    deb http://mirrors.aliyun.com/ubuntu/ bionic main restricted
    deb http://mirrors.aliyun.com/ubuntu/ bionic-updates main restricted
    deb http://mirrors.aliyun.com/ubuntu/ bionic universe
    deb http://mirrors.aliyun.com/ubuntu/ bionic-updates universe
    deb http://mirrors.aliyun.com/ubuntu/ bionic multiverse
    deb http://mirrors.aliyun.com/ubuntu/ bionic-updates multiverse
    deb http://mirrors.aliyun.com/ubuntu/ bionic-backports main restricted universe multiverse
    deb http://mirrors.aliyun.com/ubuntu/ bionic-security main restricted
    deb http://mirrors.aliyun.com/ubuntu/ bionic-security universe
```
**3.** Setting ssh server login without password
```
~$ ssh-keygen -t rsa -C "xxx@xxx"
~$ cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
```
**4.** Setting git global name and email
```
~$ sudo apt install git
~$ git config --global user.name "xxx"
~$ git config --global user.email "xxx@xxx"
```
**5.** Setting SGX development environment
- software retrive
```
~$ mkdir Repoes
~$ cd Repoes/
~/Repoes$ git clone git@gitee.com:fishermano/linux-sgx-driver.git
~/Repoes$ git clone git@gitee.com:fishermano/linux-sgx.git
```
- install sgx driver
```
~/Repoes$ cd linux-sgx-driver
~/Repoes/linux-sgx-driver$ dpkg-query -s linux-headers-$(uname -r)
			   if not execute $ sudo apt-get install linux-headers-$(uname -r)
~/Repoes/linux-sgx-driver$ sudo apt install gcc make
~/Repoes/linux-sgx-driver$ make
~/Repoes/linux-sgx-driver$ sudo mkdir -p "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"
~/Repoes/linux-sgx-driver$ sudo cp isgx.ko "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"
~/Repoes/linux-sgx-driver$ sudo sh -c "cat /etc/modules | grep -Fxq isgx || echo isgx >> /etc/modules"
~/Repoes/linux-sgx-driver$ sudo /sbin/depmod
~/Repoes/linux-sgx-driver$ sudo /sbin/modprobe isgx
~/Repoes/linux-sgx-driver$ cd ~/Repoes
```
- uninstall sgx driver
```
~/Repoes$ cd linux-sgx-driver
~/Repoes/linux-sgx-driver$ sudo /sbin/modprobe -r isgx
~/Repoes/linux-sgx-driver$ sudo rm -rf "/lib/modules/"`uname -r`"/kernel/drivers/intel/sgx"
~/Repoes/linux-sgx-driver$ sudo /sbin/depmod
~/Repoes/linux-sgx-driver$ sudo /bin/sed -i '/^isgx$/d' /etc/modules
```
- install sgx sdk
```
~/Repoes$ cd linux-sgx
// install required tools
~/Repoes/linux-sgx$ sudo apt-get install build-essential ocaml ocamlbuild automake autoconf libtool wget python libssl-dev git cmake perl
~/Repoes/linux-sgx$ sudo apt-get install libssl-dev libcurl4-openssl-dev protobuf-compiler libprotobuf-dev debhelper cmake reprepro

// install required prebuilt binaries
~/Repoes/linux-sgx$ ./download_prebuilt.sh
~/Repoes/linux-sgx$ sudo cp external/toolset/{as,ld,ld.gold,objdump} /usr/local/bin

// build sgx sdk installer
~/Repoes/linux-sgx$ make sdk_install_pkg

// install sgx sdk
// suggest: set the install directory as /opt/
~/Repoes/linux-sgx$ sudo ./linux/installer/bin/sgx_linux_x64_sdk_2.9.101.2.bin
~/Repoes/linux-sgx$ sudo chown -R hadoop.root /opt/sgxsdk
~/Repoes/linux-sgx$ sudo nano /etc/profile

    export SGX_SDK=/opt/sgxsdk
    export PATH=$PATH:$SGX_SDK/bin:$SGX_SDK/bin/x64
    export PKG_CONFIG_PATH=$PKG_CONFIG_PATH:$SGX_SDK/pkgconfig

~/Repoes/linux-sgx$ sudo touch /etc/ld.so.conf.d/sgx.conf
~/Repoes/linux-sgx$ sudo nano /etc/ld.so.conf.d/sgx.conf

    # sgx libs configuration
    /opt/sgxsdk/sdk_libs

~/Repoes/linux-sgx$ source /etc/profile
```
- install sgx psw
```
// install required tools
~/Repoes/linux-sgx$ sudo apt-get install libssl-dev libcurl4-openssl-dev protobuf-compiler libprotobuf-dev debhelper cmake reprepro

//build sgx psw local Debian package repository
~/Repoes/linux-sgx$ make deb_local_repo
~/Repoes/linux-sgx$ sudo nano /etc/apt/sources.list

    deb [trusted=yes arch=amd64] file:/ABSOLUTE_PATH_TO_LOCAL_REPO bionic main

~/Repoes/linux-sgx$ sudo apt update
~/Repoes/linux-sgx$ cd ~/Repoes

// install sgx psw [launch service]
~/Repoes$ sudo apt-get install libsgx-launch libsgx-urts

// install sgx psw [EPID-based attestation service]:
~/Repoes$ sudo apt-get install libsgx-epid libsgx-urts

// install sgx psw [algorithm agnostic attestation service]:
~/Repoes$ sudo apt-get install libsgx-quote-ex libsgx-urts
```
- uninstall sgx psw:
```
~/Repoes$ sudo apt-get remove libsgx-launch libsgx-epid libsgx-quote-ex libsgx-urts
```
**6.** Setting Hadoop development environment
- install java sdk
```
~$ cd Repoes
// download jdk 1.8 (jdk-8u151-linux-x64.tar.gz) in the current directory
~/Repoes$ sudo tar -xzvf jdk-8u151-linux-x64.tar.gz -C /opt/
~/Repoes$ sudo nano /etc/profile

    export JAVA_HOME=/opt/jdk1.8.0_151
    export JRE_HOME=$JAVA_HOME/jre
    export CLASSPATH=.:$JAVA_HOME/lib:$JAVA_HOME/jre/lib:$CLASSPATH
    export PATH=$JAVA_HOME/bin:$JAVA_HOME/jre/bin:$PATH

~/Repoes$ source /etc/profile
```
- install Hadoop sdk
```
~$ cd Repoes
~/Repoes$ git clone git@gitee.com:fishermano/hadoop-3.1.0.git
~/Repoes$ sudo tar -xzvf hadoop-3.1.0.tar.gz -C /opt/ or run 'sudo cp -R hadoop-3.1.0/ /opt'
~/Repoes$ sudo chown -R hadoop.root /opt/hadoop-3.1.0

// cofigure hadoop environment
~/Repoes$ sudo nano /etc/profile

    export HADOOP_HOME=/opt/hadoop-3.1.0
    export HADOOP_MAPRED_HOME=$HADOOP_HOME
    export HADOOP_COMMON_HOME=$HADOOP_HOME
    export HADOOP_HDFS_HOME=$HADOOP_HOME
    export YARN_HOME=$HADOOP_HOME
    export HADOOP_COMMON_LIB_NATIVE_DIR=$HADOOP_HOME/lib/native
    export PATH=$HADOOP_HOME/bin:$HADOOP_HOME/sbin:$PATH
    export HADOOP_install=$HADOOP_HOME

~/Repoes$ source /etc/profile
```
- run hadoop
```
// configure core-site.xml
~/Repoes$ sudo nano /opt/hadoop-3.1.0/etc/hadoop/core-site.xml

    <configuration>
　　    <property>
　　　　　　<name>hadoop.tmp.dir</name>
　　　　　　<value>file:/opt/hadoop-3.1.0/tmp</value>
　　　　　　<description>Abase for other temporary directories.
　　　　　　</description>
　　    </property>
　　    <property>
　　　　　　<name>fs.defaultFS</name>
　　　　　  <value>hdfs://localhost:9000</value>
　　    </property>
    </configuration>

// configure hadoop-env.sh
~/Repoes$ sudo nano /opt/hadoop-3.1.0/etc/hadoop/hadoop-env.sh

    export JAVA_HOME=/opt/jdk1.8.0_151
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HADOOP_COMMON_LIB_NATIVE_DIR

// configure hdfs-site.xml
~/Repoes$ sudo nano /opt/hadoop-3.1.0/etc/hadoop/hdfs-site.xml

    <configuration>
       <property>
          <name>dfs.replication</name>
          <value>1</value>
       </property>
       <property>
          <name>dfs.namenode.name.dir</name>
          <value>file:/opt/hadoop-3.1.0/tmp/dfs/name</value>
       </property>
       <property>
          <name>dfs.datanode.data.dir</name>
          <value>file:/opt/hadoop-3.1.0/tmp/dfs/data</value>
       </property>
       <property>
          <name>dfs.http.address</name>
          <value>0.0.0.0:9870</value>
       </property>
    </configuration>

// configure yarn-site.xml
~/Repoes$ sudo nano /opt/hadoop-3.1.0/etc/hadoop/yarn-site.xml

    <configuration>
       <property>
         <name>yarn.nodemanager.aux-services</name>
         <value>mapreduce_shuffle</value>
       </property>
    </configuration>

// configure mapred-site.xml
~/Repoes$ sudo nano /opt/hadoop-3.1.0/etc/hadoop/mapred-site.xml

    <configuration>
      <property>
         <name>mapreduce.framework.name</name>
         <value>yarn</value>
      </property>
    </configuration>

// format namenode
~/Repoes$ hdfs namenode -format

// start hdfs:
~/Repoes$ start-dfs.sh

// stop hdfs:
~/Repoes$ stop-all.sh
```
**7.** Setting sbt development environment
```
~$ cd Repoes
~/Repoes$ git clone git@gitee.com:fishermano/sbt-0.13.17.git
~/Repoes$ sudo cp -R sbt-0.13.17/ /opt

// configure sbt
~/Repoes$ sudo nano /etc/profile

    export SBT_HOME=/opt/sbt-0.13.17
    export PATH=$SBT_HOME/bin:$PATH

~/Repoes$ source /etc/profile

// configure sbt console to show current project module
~$ touch ~/.sbt/0.13/global.sbt
~$ nano ~/.sbt/0.13/global.sbt

    import scala.Console.{BLUE, RESET, UNDERLINED}
    shellPrompt := { state =>
       val projectId = Project.extract(state).currentProject.id
       s"$BLUE sbt ($projectId)>$RESET "
    }

// configure sbt repository
~$ touch ~/.sbt/repositories
~$ nano ~/.sbt/repositories

     [repositories]
     local
     aliyun-central: https://maven.aliyun.com/repository/central
     aliyun-google: https://maven.aliyun.com/repository/google
     aliyun-gradle-plugin: https://maven.aliyun.com/repository/gradle-plugin
     aliyun-jcenter: https://maven.aliyun.com/repository/jcenter
     aliyun-spring: https://maven.aliyun.com/repository/spring
     aliyun-spring-plugin: https://maven.aliyun.com/repository/spring-plugin
     aliyun-public: https://maven.aliyun.com/repository/public
     aliyun-releases: https://maven.aliyun.com/repository/releases
     aliyun-grails-core: https://maven.aliyun.com/repository/grails-core
     huaweicloud-maven: https://repo.huaweicloud.com/repository/maven/
     maven-central: https://repo1.maven.org/maven2/
     sbt-plugin-repo: https://repo.scala-sbt.org/scalasbt/sbt-plugin-releases, [organization]/[module]/(scala_[scalaVersion]/)(sbt_[sbtVersion]/)[revision]/[type]s/[artifact](-[classifier]).[ext]

$ sudo nano /opt/sbt-0.13.17/conf/sbtopts

     -Dsbt.override.build.repos=true

```
**8.** Setting maven development environment
```
~$ sudo apt-get install maven

// configure maven
~/Repoes$ sudo nano /etc/profile

     export MAVEN_HOME=/usr/share/maven
     export MAVEN_OPTS="-Xmx2048m"

~/Repoes$ source /etc/profile
```
**9.** Setting scala development environment
```
~$ cd Repoes
~/Repoes$ git clone git@gitee.com:fishermano/scala-2.11.12.git
~/Repoes$ sudo cp -R scala-2.11.12/ /opt

// configure scala:
~/Repoes$ sudo nano /etc/profile

     export SCALA_HOME=/opt/scala-2.11.12
     export PATH=$SCALA_HOME/bin:$PATH

~/Repoes$ source /etc/profile
```
**10.** Setting Spark development environment
```
~$ git clone git@gitee.com:fishermano/spark-2.4.5.git
```
- compile spark core module
```
~$ cd spark-2.4.5
~/spark-2.4.5$ mvn package -Pyarn -Phadoop-3.1 -DskipTests -pl core
```
- compile spark sql/core sql/catalyst modules
```
~/spark-2.4.5$ mvn package -Pyarn -Phadoop-3.1 -DskipTests -pl sql/catalyst,sql/core
```
- compile spark and produce a distribution
```
~/spark-2.4.5$ ./dev/make-distribution.sh --name custom-spark --tgz -Phadoop-3.1 -DskipTests
```
- install spark
```
~/spark-2.4.5$ sudo tar -zxvf spark-2.4.5-bin-custom-spark.tgz -C /opt/
~/spark-2.4.5$ cd /opt
/opt$ sudo chown -R hadoop.root spark-2.4.5-bin-custom-spark/

// configure spark-env.sh
/opt$ cd spark-2.4.5-bin-custom-spark/conf
/opt/spark-2.4.5-bin-custom-spark/conf$ cp spark-env.sh.template spark-env.sh
/opt/spark-2.4.5-bin-custom-spark/conf$ nano spark-env.sh

    export JAVA_HOME=/opt/jdk1.8.0_151
    export HADOOP_HOME=/opt/hadoop-3.1.0
    export HADOOP_CONF_DIR=/opt/hadoop-3.1.0/etc/hadoop
    export YARN_CONF_DIR=/opt/hadoop-3.1.0/etc/hadoop
    export SPARK_MASTER_IP={the name of your host}
    export SPARK_LOCAL_IP=127.0.1.1
    export SPARK_WORKER_MEMORY=8g
    export SPARK_HOME=/opt/spark-2.4.5-bin-custom-spark
    export SPARK_LOCAL_DIRS=/opt/spark-2.4.5-bin-custom-spark/tmp
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$HADOOP_HOME/lib/native

// configure /etc/hosts
~$ sudo nano /etc/hosts

    127.0.1.1 {the name of your host}

// configure slaves (stand-alone)
/opt/spark-2.4.5-bin-custom-spark/conf$ cp slaves.template slaves
/opt/spark-2.4.5-bin-custom-spark/conf$ nano slaves

    localhost

// configure log4j.properties (hide inessential information output to console)
/opt/spark-2.4.5-bin-custom-spark/conf$ cp log4j.properties.template log4j.properties
/opt/spark-2.4.5-bin-custom-spark/conf$ nano log4j.properties

    [replacing all 'INFO' with 'WARN']

// configure /etc/profile
~$ sudo nano /etc/profile

    export SPARK_HOME=/opt/spark-2.4.5-bin-custom-spark
    export PATH=$SPARK_HOME/bin:$SPARK_HOME/sbin:$PATH

~$ source /etc/profile
``` 
**11.** Setting pyspark development environement
- update pip3 source mirror
```
~$ mkdir .pip
~$ touch .pip/pip.conf
~$ nano .pip/pip.conf
    [global]
    timeout = 6000
    index-url = https://pypi.tuna.tsinghua.edu.cn/simple
    trusted-host = pypi.tuna.tsinghua.edu.cn
~$ pip3 install --upgrade pip
```
- install python3-pip and relevant dependencies
```
~$ sudo apt install python3-pip
~$ pip3 install aiohttp
~$ pip3 install jinja2
// copy pysaprk py4j source code from spark protject to /home/hadoop/.local/lib/python3.6/site-packages
~$ cp -r /home/hadoop/spark-2.4.5/python/lib/pyspark/ /home/hadoop/.local/lib/python3.6/site-packages/
~$ cp -r /home/hadoop/spark-2.4.5/python/lib/py4j/ /home/hadoop/.local/lib/python3.6/site-packages/
~$ sudo nano /etc/profile

   export PYSPARK_PYTHON=/usr/bin/python3.6
   export PYSPARK_DRIVER_PYTHON=/usr/bin/python3.6

~$ pip3 install pyspark-asyncactions
~$ pip3 install numpy
~$ pip3 install sqlparse-0.3.1
~$ pip3 install pyinstaller
**12.** QShield Compilation
- retrieve QShield source code
```
~$ git clone git@gitee.com:fishermano/qshield.git
```
- install QShield dependencies
```
// install sgx-enabled GMP library
~$ cp ./qshield/tpl/sgx-gmp/sgx_tgmp.h /opt/sgxsdk/include
~$ cp ./qshield/tpl/sgx-gmp/libsgx_tgmp.a /opt/sgxsdk/lib64

// install sgx-enabled PBC library
~$ cd ./qshield/tpl/sgx-pbc
~/qshield/tpl/sgx-pbc$ ./bootstrap
~/qshield/tpl/sgx-pbc$ ./configure prefix=/opt/sgxsdk
~/qshield/tpl/sgx-pbc$ make
~/qshield/tpl/sgx-pbc$ make install

// install sgx-enabled e-scheme library
~$ cd ./qshield/tpl/e-scheme
~/qshield/tpl/e-scheme$ ./bootstrap
~/qshield/tpl/e-scheme$ ./configure prefix=/opt/sgxsdk
~/qshield/tpl/e-scheme$ make
~/qshield/tpl/e-scheme$ make install
```
- check whether SETUP_DIR in the specification of pyinstaller has the correct absolute path of rest-apis

```
~$ nano ./qshield/rest-apis/app.spec
```
- modify QShield configuration
```
~$ nano ./qshield/rest-apis/conf/config_override.py

    'master': 'spark://{the name of your host}:7077'
    'jars': '/home/hadoop/qshield/opaque-ext/target/scala-2.11/opaque-ext_2.11-0.1.jar,/home/hadoop/qshield/data-owner/target/scala-2.11/data-owner_2.11-0.1.jar'
```
- generate data owner's private key
```
~$ openssl ecparam -name prime256v1 -genkey -noout -out private_key.pem
```
- configure QShield profile
```
~$ sudo nano /etc/profile

    export QSHIELD_HOME=/home/hadoop/qshield
    export SGX_PERF=1
    export PRIVATE_KEY_PATH=/home/hadoop/private_key.pem
    export SGX_MODE=HW
    export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH

~$ source /etc/profile
```
- compile QShield project
```
~$ cd qshield
~/qshield$ ./build/sbt
sbt (root)> package
```