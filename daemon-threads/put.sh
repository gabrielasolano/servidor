#!/bin/bash

for i in {1..1}; do
	curl -T diretorio/CentOS-6.3-x86_64-minimal.iso localhost:8080/resultado-put/centos.iso &
	curl -T diretorio/Fedora-15-i686-Live-Desktop.iso localhost:8080/resultado-put/fedora.iso
done
