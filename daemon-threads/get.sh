#!/bin/bash

for i in {1..5}; do
	wget -c localhost:8080/diretorio/CentOS-6.3-x86_64-minimal.iso -O resultado-get/imagem${i}.iso &
done
