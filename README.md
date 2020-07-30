# QShield

#### INTRODUCTION
This project implements a prototype for QShield, a secure query system introduced by the TPDS paper entitled "QShield: Protecting Outsourced Cloud Data Queries with Multi-user Access Control Based on SGX".

**Abstract:** Due to the concern on cloud security, digital encryption is applied before outsourcing data to the cloud for utilization. This introduces a challenge about how to efficiently perform queries over ciphertexts. Crypto-based solutions currently suffer limited operation support, high computational complexity, weak generality, and poor verifiability. An alternative method that utilizes hardware-assisted Trusted Execution Environment (TEE), i.e., Intel SGX, has emerged to offer high computational efficiency, generality, and flexibility. However, SGX based solutions lack support on multi-user query control and suffer from security compromises caused by untrustworthy TEE function invocation, e.g., key revocation failure, incorrect query results, and sensitive information leakage. In this paper, we leverage SGX and propose a secure and efficient SQL-style query framework named QShield. Notably, we propose a novel lightweight secret sharing scheme in QShield to enable multi-user query control; it effectively circumvents key revocation and avoids cumbersome remote attestation for authentication. We further embed a trust-proof mechanism into QShield to guarantee the trustworthiness of TEE function invocation; it ensures the correctness of query results and alleviates side-channel attacks. Through formal security analysis, proof-of-concept implementation, and performance evaluation, we show that QShield can securely query over outsourced data with high efficiency and scalable multi-user support.

#### SETUP
The following steps show how to build a developing environment for QShield.

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

