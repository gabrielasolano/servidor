#!/bin/bash

for i in {1..100}; do
  wget -c localhost:8080/ISOs/CentOS-6.3-x86_64-minimal.iso -O imagem${i}.iso &
done
