#!/bin/bash

for i in {1..40}; do
	curl -T diretorio/CentOS-6.3-x86_64-minimal.iso localhost:8080/resultado-put/imagem${i}.iso &
done
