#!/bin/bash

for i in {1..100}; do
	wget -c localhost:8080/ISOs/CentOS-6.3-x86_64-minimal.iso -O imagem${i}.iso &
done

# wget -c localhost:8080/ISOs/CentOS-6.3-x86_64-minimal.iso -O imagem${i}.iso &
# wget -c localhost:8080/buildbot-waterfall.html -O pagina${i}.html &
# wget -c localhost:8080/Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin -O binario${i}.bin &
# wget -c localhost:8080/Arquivos/ProjetoNovoDPIeIPS.pdf -O projeto${i}.pdf &
