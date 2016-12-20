#!/bin/bash

for i in {1..10}; do
	curl -T diretorio/Fedora-15-i686-Live-Desktop.iso localhost:8080/imagem.iso &
done
