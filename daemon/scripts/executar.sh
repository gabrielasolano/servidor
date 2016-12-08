#!/bin/bash

#Baixa 50 arquivos de cada (executar um por vez, para nao demorar muito)

#for i in {1..50}; do
#  wget localhost:8080/buildbot-waterfall.html -O pagina_${i}.html &
#done

#for i in {1..50}; do
#	wget localhost:8080/Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin -O arquivoBin_${i}.bin &
#done

#for i in {1..50}; do
#	wget localhost:8080/ISOs/CentOS-6.3-x86_64-minimal.iso -O centOS_${i}.iso &
#done

for i in {1..50}; do
	wget localhost:8080/ISOs/Fedora-18-i386-DVD.iso -O fedora_${i}.iso &
done

#for i in {1..50}; do
#	wget localhost:8080/Arquivos/ProjetoNovoDPIeIPS.pdf -O arquivoPdf_${i}.pdf &
#done

#for i in {1..50}; do
#	wget localhost:8080/upload/tvcultura/programas/programa-imagem-som.jpg -O imagem_${i}.jpg &
#done

echo "Script finalizado!"
echo "Para remover os arquivos criado execute o script limpar.sh"
