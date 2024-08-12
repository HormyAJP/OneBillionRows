#!/usr/bin env bash
mkdir source
sudo yum update
sudo yum -y install git java-21-amazon-corretto.x86_64 gcc zlib-devel
sudo cp /usr/include/linux/sysctl.h /usr/include/sys
sdk install java 21.0.2-graal

git clone https://github.com/gunnarmorling/1brc.git
cd 1brc
./mvnw clean verify
./create_measurements.sh 1000000000
# time ./calculate_average_thomaswue.sh
# time ./1bn ../1brc/measurements.txt



curl -s "https://get.sdkman.io" | bash
source "/home/ec2-user/.sdkman/bin/sdkman-init.sh"
wget -q https://download.oracle.com/graalvm/22/latest/graalvm-jdk-22_linux-x64_bin.tar.gz
tar -xzf graalvm-jdk-22_linux-x64_bin.tar.gz
export JAVA_HOME=/home/ec2-user/graalvm-jdk-22.0.2+9.1
echo '\nJAVA_HOME=/home/ec2-user/graalvm-jdk-22.0.2+9.1' >> ~/.bash_profile
export PATH=/home/ec2-user/graalvm-jdk-22.0.2+9.1/bin:$PATH
echo '\nPATH=/home/ec2-user/graalvm-jdk-22.0.2+9.1/bin:$PATH' >> ~/.bash_profile
sdk install java 21.0.2-graal
./prepare_thomaswue.sh

