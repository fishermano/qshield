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
**5.** Setting Hadoop development environment
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