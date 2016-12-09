#!/bin/bash
#Move todos os novos arquivos para uma nova pasta 'arquivos'
mkdir -p arquivos/html
mkdir -p arquivos/bin
mkdir -p arquivos/iso/centos
mkdir -p arquivos/iso/fedora
mkdir -p arquivos/pdf
mkdir -p arquivos/jpg

mv pagina_*.html arquivos/html/
mv imagem_*.jpg arquivos/jpg/
mv arquivoPdf_*.pdf arquivos/pdf/
mv centOS_*.iso arquivos/iso/centos/
mv fedora_*.iso arquivos/iso/fedora/
mv arquivoBin_*.bin arquivos/bin/

#Verifica os arquivos novos
echo
md5sum ../diretorio/buildbot-waterfall.html
for i in {1..10}; do
	md5sum arquivos/html/pagina_${i}.html
done
echo

md5sum ../diretorio/Arquivos/openwrt-ramips-mt7620-zbt-we826-squashfs-sysupgrade.bin
for i in {1..10}; do
	md5sum arquivos/bin/arquivoBin_${i}.bin
done
echo

md5sum ../diretorio/ISOs/CentOS-6.3-x86_64-minimal.iso 
for i in {1..10}; do
	md5sum arquivos/iso/centos/centOS_${i}.iso
done
echo

md5sum ../diretorio/ISOs/Fedora-18-i386-DVD.iso
for i in {1..10}; do
	md5sum arquivos/iso/fedora/fedora_${i}.iso
done
echo

md5sum ../diretorio/Arquivos/ProjetoNovoDPIeIPS.pdf 
for i in {1..10}; do
	md5sum arquivos/pdf/arquivoPdf_${i}.pdf
done
echo

md5sum ../diretorio/upload/tvcultura/programas/programa-imagem-som.jpg 
for i in {1..10}; do
	md5sum arquivos/jpg/imagem_${i}.jpg
done
echo
